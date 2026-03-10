#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesSlim.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/Complex.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/OpsInternal.h"
#include "CodeM/native/cpu/IndexingExtras.h"

#include <vector>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <unordered_set>

namespace pluss::codem::native::cpu {

static void lin_to_idx(std::int64_t lin, std::span<const std::int64_t> shape, std::vector<std::int64_t>& idx);
static Series bool_dim_to_positions(const Series& mask);
static std::vector<Series> bool_mask_to_multi_indices(const Series& mask);
Series index(const Series& a, const std::vector<Series>& indices);
Series index_put(const Series& a, const std::vector<Series>& indices, const Series& values, bool accumulate);

// Advanced indexing engine (CPU-only, v2).
//
// Supported:
// - Int64 index tensors for the first k dimensions (0 < k <= a.dim()).
//   Indices are broadcast together. Negative indices supported.
//   Output shape = broadcast(indices) + remaining dimensions (k..nd-1).
// - Single Bool mask tensor as indices[0] (boolean indexing):
//   output = masked_select(a, mask) (1D)
// - index_put supports overwrite or accumulate.
//
// Intentionally excluded (for now):
// - ellipsis / None / slice objects
// - non-leading advanced indices: supported
// - mixing bool masks with int indices: supported
// - multi-dimensional bool masks: supported (converted via nonzero)
// - contiguous advanced dims keep selection layout in-place (Torch parity for full values broadcasting)

static bool storage_aliases(const Series& a, const Series& b) {
  if (!a.defined() || !b.defined()) return false;
  return a.storage().data() == b.storage().data();
}

static void copy_into(Series& dst, const Series& src) {
  PLUSS_CHECK(dst.defined() && src.defined(), "copy_into: undefined");
  PLUSS_CHECK(dst.scalar_type() == src.scalar_type(), "copy_into: dtype mismatch");
  PLUSS_CHECK(dst.numel() == src.numel(), "copy_into: numel mismatch");
  // Fast path: identical contiguous layout
  if (dst.is_contiguous() && src.is_contiguous() && dst.itemsize() == src.itemsize()) {
    std::memcpy(const_cast<std::byte*>(dst.raw_data()), src.raw_data(), (std::size_t)dst.numel() * (std::size_t)dst.itemsize());
    return;
  }

  // Generic strided copy (nd <= 7)
  const auto nd = (std::int64_t)dst.dim();
  PLUSS_CHECK(nd == (std::int64_t)src.dim(), "copy_into: dim mismatch");
  for (std::int64_t d=0; d<nd; ++d) {
    PLUSS_CHECK(dst.sizes()[(std::size_t)d] == src.sizes()[(std::size_t)d], "copy_into: shape mismatch");
  }
  const auto item = dst.itemsize();
  const auto* sp = src.raw_data();
  auto* dp = const_cast<std::byte*>(dst.raw_data());

  std::vector<std::int64_t> idx((std::size_t)nd, 0);
  const auto total = dst.numel();
  for (std::int64_t lin=0; lin<total; ++lin) {
    lin_to_idx(lin, std::vector<std::int64_t>(dst.sizes().begin(), dst.sizes().end()), idx);
    std::int64_t so=0, doff=0;
    for (std::int64_t d=0; d<nd; ++d) {
      so   += idx[(std::size_t)d] * src.strides()[(std::size_t)d];
      doff += idx[(std::size_t)d] * dst.strides()[(std::size_t)d];
    }
    std::memcpy(dp + (std::ptrdiff_t)doff * (std::ptrdiff_t)item,
                sp + (std::ptrdiff_t)so   * (std::ptrdiff_t)item,
                item);
  }
}



static bool is_undefined_index(const Series& s) noexcept {
  return !s.defined() || s.numel() == 0;
}
static bool is_bool_index(const Series& s) noexcept {
  return s.defined() && s.scalar_type() == ScalarType::Bool;
}
static bool is_int_index(const Series& s) noexcept {
  return s.defined() && s.scalar_type() == ScalarType::Int64;
}

static std::vector<Series> normalize_indices_to_nd(const std::vector<Series>& indices, std::int64_t nd) {
  std::vector<Series> out;
  out.reserve((std::size_t)nd);
  for (auto& s: indices) out.push_back(s);
  while ((std::int64_t)out.size() < nd) out.push_back(Series::undefined());
  return out;
}



// Turn a 1D bool mask (length = dim size) into positions (int64).
static Series bool_1d_to_positions(const Series& mask) {
  PLUSS_CHECK(mask.dim() == 1, "bool_1d_to_positions: expects 1D mask");
  return bool_dim_to_positions(mask);
}

// Build permutation that moves all advanced indexed dims to the front, preserving order.
static std::vector<std::int64_t> adv_to_front_perm(std::int64_t nd, const std::vector<int>& is_adv) {
  std::vector<std::int64_t> perm;
  perm.reserve((std::size_t)nd);
  for (std::int64_t i=0;i<nd;++i) if (is_adv[(std::size_t)i]) perm.push_back(i);
  for (std::int64_t i=0;i<nd;++i) if (!is_adv[(std::size_t)i]) perm.push_back(i);
  return perm;
}

static std::vector<std::int64_t> invert_perm(const std::vector<std::int64_t>& p) {
  std::vector<std::int64_t> inv(p.size());
  for (std::size_t i=0;i<p.size();++i) inv[(std::size_t)p[i]] = (std::int64_t)i;
  return inv;
}

// Convert a potentially mixed list of indices into a normalized nd-length list,
// expanding multi-d bool masks into multiple int64 index tensors.
// Rules implemented (Torch-ish subset):
// - undefined => basic ":" for that dim
// - int64 index tensor => advanced for that dim
// - bool 1D mask => converted to positions (int64) for that dim
// - bool ND mask => only allowed if it spans a contiguous block of dims (same shape as input on those dims),
//                   converted via nonzero into multiple int64 index tensors, replacing that block.

static std::vector<std::int64_t> broadcast_shape(const std::vector<std::vector<std::int64_t>>& shapes) {
  std::size_t maxd = 0;
  for (auto& s : shapes) maxd = std::max(maxd, s.size());
  std::vector<std::int64_t> out(maxd, 1);
  // align from right
  for (auto& s : shapes) {
    for (std::size_t i=0;i<s.size();++i) {
      const std::size_t oi = maxd - s.size() + i;
      const std::int64_t a = out[oi];
      const std::int64_t b = s[i];
      if (a == 1) out[oi] = b;
      else if (b == 1) { /*keep*/ }
      else PLUSS_CHECK(a == b, "index: advanced index tensors are not broadcastable");
    }
  }
  return out;
}

static Series expand_int64_to_shape_contig(const Series& idx, const std::vector<std::int64_t>& shape) {
  // idx must be contiguous int64
  PLUSS_CHECK(idx.scalar_type() == ScalarType::Int64, "expand_int64_to_shape_contig: expects int64");
  auto ic = idx.contiguous();
  // if already same shape, return
  if ((std::int64_t)ic.dim() == (std::int64_t)shape.size()) {
    bool same = true;
    for (std::size_t i=0;i<shape.size();++i) if (ic.sizes()[i] != shape[i]) { same=false; break; }
    if (same) return ic;
  }

  // Use generic broadcast from ScatterKernels approach but specialized for int64.
  Series out = Series::empty(shape, ScalarType::Int64);
  const auto nd = (std::int64_t)shape.size();
  std::vector<std::int64_t> sizes_src(ic.sizes().begin(), ic.sizes().end());
  std::vector<std::int64_t> strides_src(ic.strides().begin(), ic.strides().end());

  // left-pad src sizes/strides with ones/0 to match nd
  std::vector<std::int64_t> sizes_pad((std::size_t)nd, 1);
  std::vector<std::int64_t> strides_pad((std::size_t)nd, 0);
  {
    const auto sd = (std::int64_t)sizes_src.size();
    for (std::int64_t i=0;i<sd;++i) {
      sizes_pad[(std::size_t)(nd - sd + i)] = sizes_src[(std::size_t)i];
      strides_pad[(std::size_t)(nd - sd + i)] = strides_src[(std::size_t)i];
    }
  }
  std::vector<std::int64_t> bstride((std::size_t)nd);
  for (std::int64_t k=0;k<nd;++k) bstride[(std::size_t)k] = (sizes_pad[(std::size_t)k] == 1) ? 0 : strides_pad[(std::size_t)k];

  const std::int64_t numel = out.numel();
  const std::int64_t* sp = reinterpret_cast<const std::int64_t*>(ic.raw_data());
  std::int64_t* op = reinterpret_cast<std::int64_t*>(out.raw_data());
  std::vector<std::int64_t> mi((std::size_t)nd, 0);

  for (std::int64_t lin=0; lin<numel; ++lin) {
    std::int64_t off = 0;
    for (std::int64_t k=0;k<nd;++k) off += mi[(std::size_t)k] * bstride[(std::size_t)k];
    op[lin] = sp[off];

    for (std::int64_t k=nd-1; k>=0; --k) {
      mi[(std::size_t)k] += 1;
      if (mi[(std::size_t)k] < shape[(std::size_t)k]) break;
      mi[(std::size_t)k] = 0;
    }
  }
  return out;
}

static Series expand_bool_to_shape_contig(const Series& mask, const std::vector<std::int64_t>& shape) {
  PLUSS_CHECK(mask.scalar_type() == ScalarType::Bool, "expand_bool_to_shape_contig: expects bool");
  auto mc = mask.contiguous();
  if ((std::int64_t)mc.dim() == (std::int64_t)shape.size()) {
    bool same = true;
    for (std::size_t i = 0; i < shape.size(); ++i) {
      if (mc.sizes()[i] != shape[i]) {
        same = false;
        break;
      }
    }
    if (same) return mc;
  }

  Series out = Series::empty(shape, ScalarType::Bool);
  const auto nd = (std::int64_t)shape.size();
  std::vector<std::int64_t> sizes_src(mc.sizes().begin(), mc.sizes().end());
  std::vector<std::int64_t> strides_src(mc.strides().begin(), mc.strides().end());

  std::vector<std::int64_t> sizes_pad((std::size_t)nd, 1);
  std::vector<std::int64_t> strides_pad((std::size_t)nd, 0);
  const auto sd = (std::int64_t)sizes_src.size();
  for (std::int64_t i = 0; i < sd; ++i) {
    sizes_pad[(std::size_t)(nd - sd + i)] = sizes_src[(std::size_t)i];
    strides_pad[(std::size_t)(nd - sd + i)] = strides_src[(std::size_t)i];
  }
  std::vector<std::int64_t> bstride((std::size_t)nd);
  for (std::int64_t k = 0; k < nd; ++k) {
    bstride[(std::size_t)k] = (sizes_pad[(std::size_t)k] == 1) ? 0 : strides_pad[(std::size_t)k];
  }

  const auto numel = out.numel();
  const auto* sp = reinterpret_cast<const std::uint8_t*>(mc.raw_data());
  auto* op = reinterpret_cast<std::uint8_t*>(out.raw_data());
  std::vector<std::int64_t> idx((std::size_t)nd, 0);

  for (std::int64_t lin = 0; lin < numel; ++lin) {
    std::int64_t off = 0;
    for (std::int64_t k = 0; k < nd; ++k) off += idx[(std::size_t)k] * bstride[(std::size_t)k];
    op[lin] = sp[off];

    for (std::int64_t k = nd - 1; k >= 0; --k) {
      idx[(std::size_t)k] += 1;
      if (idx[(std::size_t)k] < shape[(std::size_t)k]) break;
      idx[(std::size_t)k] = 0;
    }
  }
  return out;
}

static std::vector<Series> broadcast_advanced_indices(const std::vector<Series>& adv) {
  if (adv.size() > 1) {
    bool all_1d = true;
    for (const auto& t : adv) {
      if (t.dim() != 1) {
        all_1d = false;
        break;
      }
    }
    if (all_1d) {
      // Treat independent 1D advanced indices as an outer-product selection:
      // idx0:[N], idx1:[M] => broadcast to [N,M] instead of elementwise [max(N,M)].
      std::vector<Series> reshaped;
      reshaped.reserve(adv.size());
      for (std::size_t i = 0; i < adv.size(); ++i) {
        std::vector<std::int64_t> shape(adv.size(), 1);
        shape[i] = adv[i].sizes()[0];
        reshaped.push_back(adv[i].reshape(shape).contiguous());
      }
      std::vector<std::vector<std::int64_t>> shapes;
      shapes.reserve(reshaped.size());
      for (auto& t : reshaped) shapes.push_back(std::vector<std::int64_t>(t.sizes().begin(), t.sizes().end()));
      auto out_shape = broadcast_shape(shapes);
      std::vector<Series> out;
      out.reserve(reshaped.size());
      for (auto& t : reshaped) out.push_back(expand_int64_to_shape_contig(t, out_shape));
      return out;
    }
  }
  std::vector<std::vector<std::int64_t>> shapes;
  shapes.reserve(adv.size());
  for (auto& t : adv) shapes.push_back(std::vector<std::int64_t>(t.sizes().begin(), t.sizes().end()));
  auto out_shape = broadcast_shape(shapes);
  std::vector<Series> out;
  out.reserve(adv.size());
  for (auto& t : adv) out.push_back(expand_int64_to_shape_contig(t, out_shape));
  return out;
}
static std::vector<Series> normalize_mixed_indices(const Series& a, const std::vector<Series>& indices_in) {
  const auto nd = (std::int64_t)a.dim();
  auto idx = normalize_indices_to_nd(indices_in, nd);

  // First pass: detect multi-d bool mask
  for (std::int64_t d=0; d<nd; ++d) {
    if (!idx[(std::size_t)d].defined()) continue;
    if (is_bool_index(idx[(std::size_t)d]) && idx[(std::size_t)d].dim() > 1) {
      auto mask = idx[(std::size_t)d];
      // Ensure mask shape matches a over a block starting at d
      const auto md = (std::int64_t)mask.dim();
      PLUSS_CHECK(d + md <= nd, "index: bool mask dims exceed input dims");
      for (std::int64_t k=0;k<md;++k) {
        PLUSS_CHECK(mask.sizes()[(std::size_t)k] == a.sizes()[(std::size_t)(d+k)],
                    "index: multi-d bool mask must match input sizes for the masked block");
      }
      // Block must not have any other indices specified
      for (std::int64_t k=0;k<md;++k) {
        if (k == 0) continue;
        PLUSS_CHECK(!idx[(std::size_t)(d+k)].defined(), "index: multi-d bool mask overlaps with other indices");
      }

      // Convert to multi-indices and splice in
      auto multi = bool_mask_to_multi_indices(mask);
      PLUSS_CHECK((std::int64_t)multi.size() == md, "index: internal mask conversion error");

      // Replace block [d, d+md) with int indices
      for (std::int64_t k=0;k<md;++k) idx[(std::size_t)(d+k)] = multi[(std::size_t)k];
    }
  }

  // Second pass: convert 1D bool to positions
  for (std::int64_t d=0; d<nd; ++d) {
    if (!idx[(std::size_t)d].defined()) continue;
    if (is_bool_index(idx[(std::size_t)d])) {
      PLUSS_CHECK(idx[(std::size_t)d].dim() == 1, "index: bool index must be 1D or ND mask handled earlier");
      PLUSS_CHECK(idx[(std::size_t)d].sizes()[0] == a.sizes()[(std::size_t)d], "index: bool mask length mismatch");
      idx[(std::size_t)d] = bool_1d_to_positions(idx[(std::size_t)d]);
    }
    PLUSS_CHECK(is_int_index(idx[(std::size_t)d]), "index: indices must be int64 or bool");
  }


  // Torch-like: if there are multiple advanced index tensors, broadcast them to a common shape.
  {
    std::vector<Series> adv;
    std::vector<std::int64_t> adv_dims;
    for (std::int64_t d=0; d<nd; ++d) {
      if (idx[(std::size_t)d].defined()) {
        adv.push_back(idx[(std::size_t)d].to(ScalarType::Int64).contiguous());
        adv_dims.push_back(d);
      }
    }
    if (adv.size() > 1) {
      auto b = broadcast_advanced_indices(adv);
      for (std::size_t i=0;i<b.size();++i) idx[(std::size_t)adv_dims[i]] = b[i];
    }
  }

return idx;
}

static Series index_torch_like(const Series& a, const std::vector<Series>& indices_in);
static Series index_put_torch_like(const Series& a, const std::vector<Series>& indices_in, const Series& values, bool accumulate);

static std::vector<Series> bool_mask_to_multi_indices(const Series& mask) {
  auto nz = pluss::codem::ops_internal::nonzero(mask); // [nnz, nd]
  auto nzc = nz.contiguous();
  const auto nnz = nzc.sizes()[0];
  const auto nd = nzc.sizes()[1];
  const auto* p = reinterpret_cast<const std::int64_t*>(nzc.raw_data());
  std::vector<Series> outs;
  outs.reserve((std::size_t)nd);
  for (std::int64_t d=0; d<nd; ++d) {
    Series col = Series::empty({nnz}, ScalarType::Int64);
    auto* cp = reinterpret_cast<std::int64_t*>(col.raw_data());
    for (std::int64_t i=0;i<nnz;++i) cp[i] = p[i*nd + d];
    outs.push_back(col);
  }
  return outs;
}

static Series bool_dim_to_positions(const Series& mask) {
  // mask is 1D
  auto nz = pluss::codem::ops_internal::nonzero(mask); // [nnz,1]
  Series pos = Series::empty({nz.sizes()[0]}, ScalarType::Int64);
  auto* pp = reinterpret_cast<std::int64_t*>(pos.raw_data());
  auto* np = reinterpret_cast<const std::int64_t*>(nz.raw_data());
  for (std::int64_t i=0;i<nz.sizes()[0];++i) pp[i]=np[i];
  return pos;
}

static std::vector<std::int64_t> broadcast_shape_many(const std::vector<std::vector<std::int64_t>>& shapes) {
  std::vector<std::int64_t> out;
  for (const auto& s : shapes) out = SeriesIterator::broadcast_shape(out, s);
  return out;
}

static std::vector<std::int64_t> expand_strides(std::span<const std::int64_t> sizes,
                                                std::span<const std::int64_t> strides,
                                                std::span<const std::int64_t> out_shape) {
  const std::size_t n = out_shape.size();
  const std::size_t m = sizes.size();
  std::vector<std::int64_t> out_strides(n, 0);
  for (std::size_t i = 0; i < n; ++i) {
    const std::size_t oi = n - 1 - i;
    const std::size_t si = (i < m) ? (m - 1 - i) : static_cast<std::size_t>(-1);

    const std::int64_t osz = out_shape[oi];
    const std::int64_t ssz = (i < m) ? sizes[si] : 1;

    if (ssz == 1 && osz != 1) out_strides[oi] = 0;
    else if (ssz == osz) out_strides[oi] = (i < m) ? strides[si] : 0;
    else throw_error("advanced_index: incompatible index broadcast");
  }
  return out_strides;
}

static void lin_to_idx(std::int64_t lin, std::span<const std::int64_t> shape, std::vector<std::int64_t>& idx) {
  for (std::int64_t d = (std::int64_t)shape.size() - 1; d >= 0; --d) {
    const auto s = shape[(std::size_t)d];
    idx[(std::size_t)d] = (s == 0) ? 0 : (lin % s);
    lin = (s == 0) ? 0 : (lin / s);
  }
}

static std::int64_t off(const std::vector<std::int64_t>& idx, const std::vector<std::int64_t>& strides) {
  std::int64_t o = 0;
  for (std::size_t i=0;i<idx.size();++i) o += idx[i] * strides[i];
  return o;
}



static Series index_torch_like(const Series& a, const std::vector<Series>& indices_in) {
  const auto nd = (std::int64_t)a.dim();
  auto idx = normalize_mixed_indices(a, indices_in);

  // Identify advanced dims (where an index tensor is defined)
  std::vector<std::int64_t> adv_dims;
  adv_dims.reserve((std::size_t)nd);
  for (std::int64_t d=0; d<nd; ++d) if (idx[(std::size_t)d].defined()) adv_dims.push_back(d);

  if (adv_dims.empty()) {
    // Basic indexing only (":" everywhere) -> return copy (Torch returns view-like; we return copy for safety)
    return a.clone();
  }

  // Check whether advanced dims form a contiguous block in the original dimension order.
  bool contiguous_block = true;
  for (std::size_t i=1;i<adv_dims.size();++i) {
    if (adv_dims[i] != adv_dims[i-1] + 1) { contiguous_block = false; break; }
  }

  const std::int64_t block_begin = adv_dims.front();
  const std::int64_t block_end_excl = adv_dims.back() + 1; // [begin,end)
  const std::int64_t k = (std::int64_t)adv_dims.size();

  // Collect advanced indices in block order (already broadcasted in normalize_mixed_indices)
  std::vector<Series> adv;
  adv.reserve((std::size_t)k);
  for (std::int64_t i=0;i<k;++i) adv.push_back(idx[(std::size_t)(block_begin+i)]);

  // Fast path: if advanced block is leading, existing engine already yields correct "adv dims in place" order.
  if (contiguous_block && block_begin == 0) {
    return index(a, adv);
  }

  if (contiguous_block) {
    // Torch/Numpy rule: if advanced indices are contiguous, the broadcasted advanced shape appears
    // in-place where the block occurred (not necessarily moved to front).
    //
    // Implementation:
    // - permute input so adv-block dims are moved to the front, preserving relative order of all dims
    // - run leading-advanced index engine
    // - permute output so adv broadcasted dims move back into the original block position
    std::vector<std::int64_t> perm;
    perm.reserve((std::size_t)nd);
    for (std::int64_t d=block_begin; d<block_end_excl; ++d) perm.push_back(d);
    for (std::int64_t d=0; d<block_begin; ++d) perm.push_back(d);
    for (std::int64_t d=block_end_excl; d<nd; ++d) perm.push_back(d);

    Series ap = a.permute(perm);
    Series out_front = index(ap, adv); // out dims: [adv_shape, dims_before, dims_after]

    // Now move adv_shape dims back after dims_before.
    const std::int64_t out_nd = out_front.dim();
    const std::int64_t before = block_begin;
    const std::int64_t after = nd - block_end_excl;

    // out_front layout: [0..k-1]=adv, [k..k+before-1]=before, [k+before..]=after
    std::vector<std::int64_t> out_perm;
    out_perm.reserve((std::size_t)out_nd);
    // before dims first
    for (std::int64_t i=0;i<before;++i) out_perm.push_back(k + i);
    // then adv dims
    for (std::int64_t i=0;i<k;++i) out_perm.push_back(i);
    // then after dims
    for (std::int64_t i=0;i<after;++i) out_perm.push_back(k + before + i);

    return out_front.permute(out_perm);
  }

  // Non-contiguous advanced dims: Torch moves advanced dims to the front.
  // We implement by permuting all advanced dims to the front, then applying leading engine.
  std::vector<int> is_adv((std::size_t)nd, 0);
  for (auto d : adv_dims) is_adv[(std::size_t)d] = 1;
  const auto perm = adv_to_front_perm(nd, is_adv);

  Series ap = a.permute(perm);
  // permute indices as well
  std::vector<Series> idxp;
  idxp.reserve((std::size_t)nd);
  for (std::int64_t d=0; d<nd; ++d) {
    const auto od = perm[(std::size_t)d];
    if (!idx[(std::size_t)od].defined()) { idxp.push_back(Series::undefined()); continue; }
    idxp.push_back(idx[(std::size_t)od]);
  }

  // collect leading advanced in permuted space
  std::int64_t kk = 0;
  for (std::int64_t d=0; d<nd; ++d) if (is_adv[(std::size_t)d]) ++kk;
  std::vector<Series> leading;
  leading.reserve((std::size_t)kk);
  for (std::int64_t d=0; d<kk; ++d) leading.push_back(idxp[(std::size_t)d]);

  return index(ap, leading); // adv dims moved to front
}



static Series index_put_torch_like(const Series& a, const std::vector<Series>& indices_in, const Series& values, bool accumulate) {
  const auto nd = (std::int64_t)a.dim();
  auto idx = normalize_mixed_indices(a, indices_in);

  std::vector<std::int64_t> adv_dims;
  adv_dims.reserve((std::size_t)nd);
  for (std::int64_t d=0; d<nd; ++d) if (idx[(std::size_t)d].defined()) adv_dims.push_back(d);

  if (adv_dims.empty()) {
    // No indexing => replace entire tensor. Torch would broadcast values; we require same numel unless scalar.
    if (values.numel() == 1) {
      Series out = a.clone();
      Series vv = values.to(out.scalar_type()).contiguous();
      const auto item = out.itemsize();
      const auto* vp = vv.raw_data();
      auto* op = const_cast<std::byte*>(out.raw_data());
      for (std::int64_t i = 0; i < out.numel(); ++i) {
        std::memcpy(op + (std::ptrdiff_t)i * (std::ptrdiff_t)item, vp, (std::size_t)item);
      }
      return out;
    }
    PLUSS_CHECK(values.numel() == a.numel(), "index_put: values must match tensor size when no indices");
    return values.reshape(a.sizes());
  }

  bool contiguous_block = true;
  for (std::size_t i=1;i<adv_dims.size();++i) {
    if (adv_dims[i] != adv_dims[i-1] + 1) { contiguous_block = false; break; }
  }
  const std::int64_t block_begin = adv_dims.front();
  const std::int64_t block_end_excl = adv_dims.back() + 1;
  const std::int64_t k = (std::int64_t)adv_dims.size();

  std::vector<Series> adv;
  adv.reserve((std::size_t)k);
  for (std::int64_t i=0;i<k;++i) adv.push_back(idx[(std::size_t)(block_begin+i)]);

  if (contiguous_block && block_begin == 0) {
    return index_put(a, adv, values, accumulate);
  }

  if (contiguous_block) {
    // Keep advanced dims in-place (Torch rule) by permuting adv-block to the front, updating, then permuting back.
    std::vector<std::int64_t> perm;
    perm.reserve((std::size_t)nd);
    for (std::int64_t d=block_begin; d<block_end_excl; ++d) perm.push_back(d);
    for (std::int64_t d=0; d<block_begin; ++d) perm.push_back(d);
    for (std::int64_t d=block_end_excl; d<nd; ++d) perm.push_back(d);

    Series ap = a.permute(perm);
    Series out_front = index_put(ap, adv, values, accumulate); // same shape as ap

    // permute back to original
    // inverse of perm:
    std::vector<std::int64_t> inv(perm.size());
    for (std::size_t i=0;i<perm.size();++i) inv[(std::size_t)perm[i]] = (std::int64_t)i;
    return out_front.permute(inv);
  }

  // Non-contiguous advanced dims => move advanced dims to front (Torch rule),
  // then update, then permute back.
  std::vector<int> is_adv((std::size_t)nd, 0);
  for (auto d : adv_dims) is_adv[(std::size_t)d] = 1;
  const auto perm = adv_to_front_perm(nd, is_adv);
  const auto invp = invert_perm(perm);

  Series ap = a.permute(perm);

  std::vector<Series> idxp;
  idxp.reserve((std::size_t)nd);
  for (std::int64_t d=0; d<nd; ++d) {
    const auto od = perm[(std::size_t)d];
    if (!idx[(std::size_t)od].defined()) { idxp.push_back(Series::undefined()); continue; }
    idxp.push_back(idx[(std::size_t)od]);
  }

  std::int64_t kk = 0;
  for (std::int64_t d=0; d<nd; ++d) if (is_adv[(std::size_t)d]) ++kk;

  std::vector<Series> leading;
  leading.reserve((std::size_t)kk);
  for (std::int64_t d=0; d<kk; ++d) leading.push_back(idxp[(std::size_t)d]);

  Series outp = index_put(ap, leading, values, accumulate);
  return outp.permute(invp);
}


Series index(const Series& a, const std::vector<Series>& indices) {
  if (!a.defined()) throw_error("index: undefined input");
  if (indices.empty()) throw_error("index: empty indices");


  // General Torch-like indexing path:
  // - supports non-leading advanced indices by permuting advanced dims to the front
  // - supports mixing bool masks with int indices (bool -> positions, or multi-d mask -> nonzero -> multi indices)
  // - supports indices list length == a.dim() with undefined entries (":" dimensions)
  if ((std::int64_t)indices.size() == a.dim()) {
    // if any bool or any undefined index present, or any non-leading advanced likely -> use torch-like path
    bool need = false;
    for (const auto& t : indices) {
      if (!t.defined()) { need = true; break; }
      if (t.scalar_type() == ScalarType::Bool) { need = true; break; }
    }
    if (need) return index_torch_like(a, indices);
  }
if (indices.size() == 1 && indices[0].scalar_type() == ScalarType::Bool) {
    return ops_internal::masked_select(a, indices[0]);
  }

  const auto nd = a.dim();
  const auto k = (std::int64_t)indices.size();
  if (k <= 0 || k > nd) throw_error("index: invalid number of indices");

  // Indices must be int64, broadcastable together
  std::vector<std::vector<std::int64_t>> ishapes;
  ishapes.reserve(indices.size());
  std::vector<Series> idxc;
  idxc.reserve(indices.size());
  for (const auto& t : indices) {
    if (!t.defined()) throw_error("index: undefined index");
    if (t.scalar_type() != ScalarType::Int64) throw_error("index: indices must be int64 (or single bool mask)");
    idxc.push_back(t.contiguous());
    ishapes.push_back(std::vector<std::int64_t>(t.sizes().begin(), t.sizes().end()));
  }
  auto bshape = broadcast_shape_many(ishapes);

  // Output shape = bshape + remaining dims (k..nd-1)
  std::vector<std::int64_t> out_shape = bshape;
  for (std::int64_t d=k; d<nd; ++d) out_shape.push_back(a.sizes()[(std::size_t)d]);

  Series out = Series::empty(out_shape, a.scalar_type());

  auto aa = a.contiguous();
  const auto dt = aa.dtype();
  const auto* ap = aa.raw_data();
  auto* op = const_cast<std::byte*>(out.raw_data());

  // expanded strides for each index into bshape
  std::vector<std::vector<std::int64_t>> idx_es;
  idx_es.reserve(k);
  for (std::int64_t d=0; d<k; ++d) idx_es.push_back(expand_strides(idxc[(std::size_t)d].sizes(), idxc[(std::size_t)d].strides(), bshape));

  const std::int64_t head_total = compute_numel(bshape);

  // tail is contiguous block of remaining dims
  std::int64_t tail = 1;
  for (std::int64_t d=k; d<nd; ++d) tail *= aa.sizes()[(std::size_t)d];
  const std::size_t tail_bytes = (std::size_t)tail * dt.itemsize;

  std::vector<std::int64_t> idx(bshape.size(), 0);

  for (std::int64_t lin=0; lin<head_total; ++lin) {
    lin_to_idx(lin, bshape, idx);

    // compute base offset in a for indexed dims (leading)
    std::int64_t base_off = 0;
    for (std::int64_t d=0; d<k; ++d) {
      const auto io = off(idx, idx_es[(std::size_t)d]);
      auto kidx = reinterpret_cast<const std::int64_t*>(idxc[(std::size_t)d].raw_data())[io];
      const auto dim_size = aa.sizes()[(std::size_t)d];
      if (kidx < 0) kidx += dim_size;
      if (kidx < 0 || kidx >= dim_size) throw_error("index: index out of range");
      base_off += kidx * aa.strides()[(std::size_t)d];
    }

    // copy tail block
    const std::int64_t out_base = lin * tail;
    std::memcpy(op + (std::ptrdiff_t)out_base * (std::ptrdiff_t)dt.itemsize,
                ap + (std::ptrdiff_t)base_off * (std::ptrdiff_t)dt.itemsize,
                tail_bytes);
  }

  return out;
}

Series index_put(const Series& a, const std::vector<Series>& indices, const Series& values, bool accumulate) {
  if (!a.defined() || !values.defined()) throw_error("index_put: undefined");
  if (indices.empty()) throw_error("index_put: empty indices");


  // General Torch-like mixed indexing path (non-leading advanced, mixed bool/int, multi-d masks).
  if ((std::int64_t)indices.size() == a.dim()) {
    bool need = false;
    for (const auto& t : indices) {
      if (!t.defined()) { need = true; break; }
      if (t.scalar_type() == ScalarType::Bool) { need = true; break; }
    }
    if (need) return index_put_torch_like(a, indices, values, accumulate);
  }
  // Bool mask path: allow mask broadcast to a (torch-y), but we require final broadcast to exactly a.shape.
  if (indices.size() == 1 && indices[0].scalar_type() == ScalarType::Bool) {
    auto mask = indices[0];
    auto shape = SeriesIterator::broadcast_shape(a.sizes(), mask.sizes());
    if (shape != std::vector<std::int64_t>(a.sizes().begin(), a.sizes().end()))
      throw_error("index_put: mask must broadcast to a.shape");
    if (mask.sizes().size() != a.sizes().size() ||
        !std::equal(mask.sizes().begin(), mask.sizes().end(), a.sizes().begin())) {
      mask = expand_bool_to_shape_contig(mask, shape);
    }

    auto aa = a.contiguous();
    auto mm = mask.contiguous();

    auto nz = ops_internal::nonzero(mm); // [nnz, nd]
    const auto nd = aa.dim();
    const auto nnz = nz.sizes()[0];

    // multi -> flat
    auto strides = contiguous_strides(std::vector<std::int64_t>(aa.sizes().begin(), aa.sizes().end()));
    Series flat = Series::empty({nnz}, ScalarType::Int64);
    auto* fp = reinterpret_cast<std::int64_t*>(flat.raw_data());
    auto* mip = reinterpret_cast<const std::int64_t*>(nz.raw_data());
    for (std::int64_t i=0;i<nnz;++i){
      std::int64_t o=0;
      for (std::int64_t d=0; d<nd; ++d) o += mip[i*nd + d] * strides[(std::size_t)d];
      fp[i]=o;
    }

    if (values.scalar_type() != aa.scalar_type()) throw_error("index_put: values dtype mismatch");
    if (!(values.numel()==1 || values.numel()==nnz)) throw_error("index_put: values must be scalar or match nnz");
    return put(aa, flat, values, accumulate);
  }


  const auto nd = a.dim();

  auto idx = normalize_indices_to_nd(indices, nd);

  std::vector<std::int64_t> adv_dims;
  std::vector<Series> adv_tensors;
  std::vector<std::vector<std::int64_t>> adv_shapes;


  for (std::int64_t d=0; d<nd; ++d) {
    if (is_undefined_index(idx[(std::size_t)d])) continue;

    if (is_bool_index(idx[(std::size_t)d])) {
      auto mask = idx[(std::size_t)d].contiguous();

      if (mask.dim() == 1) {
        PLUSS_CHECK(mask.sizes()[0] == a.sizes()[(std::size_t)d], "index_put: bool index size mismatch for dimension");
        auto pos = bool_dim_to_positions(mask);
        adv_dims.push_back(d);
        adv_tensors.push_back(pos.contiguous());
        adv_shapes.push_back({pos.sizes()[0]});
        continue;
      }

      // Broadcastable full-tensor bool mask (Torch-style):
      // if mask has the same rank as `a`, and is broadcastable to `a.shape`, we treat it as a full-tensor mask.
      if (mask.dim() == nd) {
        // verify broadcastability (allow 1 or exact)
        bool ok = true;
        for (std::int64_t k=0; k<nd; ++k) {
          const auto ms = mask.sizes()[(std::size_t)k];
          const auto as = a.sizes()[(std::size_t)k];
          if (!(ms == 1 || ms == as)) { ok = false; break; }
        }
        if (ok) {
          auto em = mask;
          if (mask.sizes().size() != a.sizes().size() ||
              !std::equal(mask.sizes().begin(), mask.sizes().end(), a.sizes().begin())) {
            em = expand_bool_to_shape_contig(mask, std::vector<std::int64_t>(a.sizes().begin(), a.sizes().end()));
          }
          em = em.contiguous();
          auto cols = bool_mask_to_multi_indices(em);
          for (std::int64_t k=0; k<nd; ++k) {
            adv_dims.push_back(k);
            adv_tensors.push_back(cols[(std::size_t)k].contiguous());
            adv_shapes.push_back({cols[(std::size_t)k].sizes()[0]});
          }
          // Full mask consumes all dims
          break;
        }
      }

      // Multi-dim bool mask: must match a contiguous block of dimensions starting at d.
      const auto md = (std::int64_t)mask.sizes().size();
      PLUSS_CHECK(d + md <= nd, "index_put: multi-dim bool mask exceeds tensor rank");
      for (std::int64_t k=0; k<md; ++k) {
        PLUSS_CHECK(mask.sizes()[(std::size_t)k] == a.sizes()[(std::size_t)(d+k)], "index_put: multi-dim bool mask shape mismatch");
      }

      auto cols = bool_mask_to_multi_indices(mask);
      for (std::int64_t k=0; k<md; ++k) {
        adv_dims.push_back(d+k);
        adv_tensors.push_back(cols[(std::size_t)k].contiguous());
        adv_shapes.push_back({cols[(std::size_t)k].sizes()[0]});
      }
      d += (md - 1);
      continue;
    }

    PLUSS_CHECK(is_int_index(idx[(std::size_t)d]), "index_put: indices must be Int64, Bool, or undefined(slice)");
    auto t = idx[(std::size_t)d].contiguous();
    adv_dims.push_back(d);
    adv_tensors.push_back(t);
    adv_shapes.push_back(std::vector<std::int64_t>(t.sizes().begin(), t.sizes().end()));
  }
if (adv_dims.empty()) throw_error("index_put: no indices");
  auto bshape = broadcast_shape_many(adv_shapes);
  const std::int64_t head_total = compute_numel(bshape);

  const bool adv_contiguous = [&](){
    if (adv_dims.empty()) return true;
    for (std::size_t i=1;i<adv_dims.size();++i) if (adv_dims[i] != adv_dims[i-1] + 1) return false;
    return true;
  }();


  // tail dims: all dims not in adv_dims are sliced through
  std::vector<char> is_adv((std::size_t)nd, 0);
  for (auto d : adv_dims) is_adv[(std::size_t)d]=1;

  std::vector<std::int64_t> tail_sizes;
  std::vector<std::int64_t> tail_strides;
  for (std::int64_t d=0; d<nd; ++d) {
    if (!is_adv[(std::size_t)d]) {
      tail_sizes.push_back(a.sizes()[(std::size_t)d]);
      tail_strides.push_back(a.strides()[(std::size_t)d]);
    }
  }
  const std::int64_t tail_total = compute_numel(tail_sizes);

  auto out = a.contiguous().clone();
  if (values.scalar_type() != out.scalar_type()) throw_error("index_put: values dtype mismatch");

  const auto vnum = values.numel();
  const bool v_scalar = (vnum == 1);
  const bool v_head   = (vnum == head_total);
  const bool v_tail   = (vnum == tail_total);
  const bool v_full   = (vnum == head_total * tail_total);
  // Compatibility mode for legacy callers that pass an extra packed suffix
  // (e.g. [head, tail, ...]). We consume the first packed block.
  const bool v_packed = (head_total > 0 && tail_total > 0 &&
                         vnum > (head_total * tail_total) &&
                         (vnum % (head_total * tail_total) == 0));
  if (!(v_scalar || v_head || v_tail || v_full || v_packed)) {
    throw_error("index_put: values must be scalar, head_total, tail_total, or full output match");
  }


  std::vector<std::vector<std::int64_t>> idx_es;
  idx_es.reserve(adv_tensors.size());
  for (std::size_t i=0; i<adv_tensors.size(); ++i) {
    idx_es.push_back(expand_strides(adv_tensors[i].sizes(), adv_tensors[i].strides(), bshape));
  }

  Series vv = values.contiguous();
  if (storage_aliases(out, vv)) vv = vv.clone().contiguous();

  const std::int64_t total_updates = head_total * tail_total;
  const bool track_overlap = (!accumulate &&
                              indices.size() == 1 &&
                              nd == 1 &&
                              tail_total == 1 &&
                              total_updates <= 1'000'000);
  std::unordered_set<std::int64_t> touched;
  if (track_overlap) {
    touched.reserve((std::size_t)total_updates);
  }

  const auto dt = out.dtype();
  auto* op = const_cast<std::byte*>(out.raw_data());
  const auto* vp = vv.raw_data();

  std::vector<std::int64_t> bidx(bshape.size(), 0);
  std::vector<std::int64_t> tidx(tail_sizes.size(), 0);

  auto add_inplace = [&](std::int64_t out_off, std::int64_t v_off){
    if (out.scalar_type() == ScalarType::Float64) { reinterpret_cast<double*>(op)[out_off] += reinterpret_cast<const double*>(vp)[v_off]; return; }
    if (out.scalar_type() == ScalarType::Float32) { reinterpret_cast<float*>(op)[out_off] += reinterpret_cast<const float*>(vp)[v_off]; return; }
    if (out.scalar_type() == ScalarType::Float16) {
      auto* outp = reinterpret_cast<Float16*>(op);
      const auto* srcp = reinterpret_cast<const Float16*>(vp);
      outp[out_off] = float16_from_float(float16_to_float(outp[out_off]) + float16_to_float(srcp[v_off]));
      return;
    }
    if (out.scalar_type() == ScalarType::BFloat16) {
      auto* outp = reinterpret_cast<BFloat16*>(op);
      const auto* srcp = reinterpret_cast<const BFloat16*>(vp);
      outp[out_off] = bfloat16_from_float(bfloat16_to_float(outp[out_off]) + bfloat16_to_float(srcp[v_off]));
      return;
    }
    if (out.scalar_type() == ScalarType::Int64)   { reinterpret_cast<std::int64_t*>(op)[out_off] += reinterpret_cast<const std::int64_t*>(vp)[v_off]; return; }
    if (out.scalar_type() == ScalarType::Int32)   { reinterpret_cast<std::int32_t*>(op)[out_off] += reinterpret_cast<const std::int32_t*>(vp)[v_off]; return; }
    if (out.scalar_type() == ScalarType::Int16)   { reinterpret_cast<std::int16_t*>(op)[out_off] += reinterpret_cast<const std::int16_t*>(vp)[v_off]; return; }
    if (out.scalar_type() == ScalarType::Int8)    { reinterpret_cast<std::int8_t*>(op)[out_off] += reinterpret_cast<const std::int8_t*>(vp)[v_off]; return; }
    if (out.scalar_type() == ScalarType::UInt64)  { reinterpret_cast<std::uint64_t*>(op)[out_off] += reinterpret_cast<const std::uint64_t*>(vp)[v_off]; return; }
    if (out.scalar_type() == ScalarType::UInt32)  { reinterpret_cast<std::uint32_t*>(op)[out_off] += reinterpret_cast<const std::uint32_t*>(vp)[v_off]; return; }
    if (out.scalar_type() == ScalarType::UInt16)  { reinterpret_cast<std::uint16_t*>(op)[out_off] += reinterpret_cast<const std::uint16_t*>(vp)[v_off]; return; }
    if (out.scalar_type() == ScalarType::UInt8)   { reinterpret_cast<std::uint8_t*>(op)[out_off] += reinterpret_cast<const std::uint8_t*>(vp)[v_off]; return; }
    if (out.scalar_type() == ScalarType::Complex64) {
      reinterpret_cast<Complex64*>(op)[out_off] += reinterpret_cast<const Complex64*>(vp)[v_off];
      return;
    }
    if (out.scalar_type() == ScalarType::Complex128) {
      reinterpret_cast<Complex128*>(op)[out_off] += reinterpret_cast<const Complex128*>(vp)[v_off];
      return;
    }
    throw_error("index_put: accumulate dtype not implemented");
  };

  for (std::int64_t lin=0; lin<head_total; ++lin) {
    lin_to_idx(lin, bshape, bidx);

    std::int64_t base_off = 0;
    for (std::size_t i=0; i<adv_tensors.size(); ++i) {
      const auto io = off(bidx, idx_es[i]);
      auto kidx = reinterpret_cast<const std::int64_t*>(adv_tensors[i].raw_data())[io];
      const auto dim = adv_dims[i];
      const auto dim_size = out.sizes()[(std::size_t)dim];
      if (kidx < 0) kidx += dim_size;
      if (kidx < 0 || kidx >= dim_size) throw_error("index_put: index out of range");
      base_off += kidx * out.strides()[(std::size_t)dim];
    }

    for (std::int64_t tj=0; tj<tail_total; ++tj) {
      lin_to_idx(tj, tail_sizes, tidx);
      std::int64_t tail_off = 0;
      for (std::size_t q=0; q<tail_sizes.size(); ++q) tail_off += tidx[q] * tail_strides[q];

      const std::int64_t out_off = base_off + tail_off;
      if (track_overlap) {
        const auto inserted = touched.insert(out_off);
        if (!inserted.second) {
          throw_error("index_put: overlapping indices with accumulate=false");
        }
      }
      const std::int64_t v_off = [&](){
        if (v_scalar) return (std::int64_t)0;
        if (v_head) return lin;
        if (v_tail) return tj;
        if (v_packed) return lin*tail_total + tj;
        if (!adv_contiguous) return lin*tail_total + tj;

        const std::int64_t first_adv = adv_dims.front();
        std::vector<std::int64_t> sel_sizes;
        sel_sizes.reserve((std::size_t)nd);
        for (std::int64_t d=0; d<nd; ++d) {
          const bool isA = (std::find(adv_dims.begin(), adv_dims.end(), d) != adv_dims.end());
          if (d < first_adv && !isA) sel_sizes.push_back(a.sizes()[(std::size_t)d]);
        }
        for (auto s : bshape) sel_sizes.push_back(s);
        for (std::int64_t d=0; d<nd; ++d) {
          const bool isA = (std::find(adv_dims.begin(), adv_dims.end(), d) != adv_dims.end());
          if (d > adv_dims.back() && !isA) sel_sizes.push_back(a.sizes()[(std::size_t)d]);
        }

        std::vector<std::int64_t> sel_idx;
        sel_idx.reserve(sel_sizes.size());

        std::size_t tpos = 0;
        for (std::int64_t d=0; d<nd; ++d) {
          const bool isA = (std::find(adv_dims.begin(), adv_dims.end(), d) != adv_dims.end());
          if (!isA) {
            if (d < first_adv) sel_idx.push_back(tidx[tpos]);
            ++tpos;
          }
        }
        for (auto v : bidx) sel_idx.push_back(v);

        tpos = 0;
        for (std::int64_t d=0; d<nd; ++d) {
          const bool isA = (std::find(adv_dims.begin(), adv_dims.end(), d) != adv_dims.end());
          if (!isA) {
            if (d > adv_dims.back()) sel_idx.push_back(tidx[tpos]);
            ++tpos;
          }
        }

        std::int64_t off=0; std::int64_t stride=1;
        for (std::size_t i=sel_sizes.size(); i-- > 0;) { off += sel_idx[i]*stride; stride*=sel_sizes[i]; }
        return off;
      }();
      if (accumulate) add_inplace(out_off, v_off);
      else {
        std::memcpy(op + (std::ptrdiff_t)out_off * (std::ptrdiff_t)dt.itemsize,
                    vp + (std::ptrdiff_t)v_off  * (std::ptrdiff_t)dt.itemsize,
                    dt.itemsize);
      }
    }
  }

  return out;
}

void index_put_inplace(Series& a, const std::vector<Series>& indices, const Series& values, bool accumulate) {
  Series out = index_put(a, indices, values, accumulate);
  copy_into(a, out);
}

} // namespace pluss::codem::native::cpu
