#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesSlim.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/Half.h"
#include "CodeM/core/Complex.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/OpsInternal.h"
#include "CodeM/native/cpu/IndexingExtras.h"
#include "MSharp/RangeView.h"

#include <vector>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <unordered_set>

namespace pluss::codem::native::cpu {

static void lin_to_idx(std::int64_t lin, std::span<const std::int64_t> shape, std::vector<std::int64_t>& idx);
static Series bool_dim_to_positions(const Series& mask);
static std::vector<Series> bool_mask_to_multi_indices(const Series& mask);
Series index(const Series& a, pluss::msharp::RangeView::Of<Series> indices);
Series index_put(const Series& a, pluss::msharp::RangeView::Of<Series> indices, const Series& values, bool accumulate);

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

  // Generic strided copy over dynamically sized metadata.
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
  while ((std::int64_t)out.size() < nd) out.push_back(pluss::series::undefined());
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
  // Torch-like behavior:
  // - multiple advanced index tensors are broadcast together by standard
  //   broadcasting rules.
  // - 1D + 1D does pairwise selection when shapes match (no forced outer
  //   product expansion).
  // Outer-product semantics are still achievable by explicitly reshaping/
  // unsqueezing indices at the call-site.
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
  const auto nnz = nz.sizes()[0];
  const auto nd = nz.sizes()[1];
  std::vector<Series> outs;
  outs.reserve((std::size_t)nd);
  for (std::int64_t d=0; d<nd; ++d) {
    outs.push_back(nz.select(1, d).reshape({nnz}));
  }
  return outs;
}

static Series bool_dim_to_positions(const Series& mask) {
  // mask is 1D
  auto nz = pluss::codem::ops_internal::nonzero(mask); // [nnz,1]
  return nz.select(1, 0).reshape({nz.sizes()[0]});
}

static std::vector<std::int64_t> broadcast_shape_many(const std::vector<std::vector<std::int64_t>>& shapes) {
  std::vector<std::int64_t> out;
  for (const auto& s : shapes) out = SeriesIterator::broadcast_shape(out, s);
  return out;
}

static void lin_to_idx(std::int64_t lin, std::span<const std::int64_t> shape, std::vector<std::int64_t>& idx) {
  for (std::int64_t d = (std::int64_t)shape.size() - 1; d >= 0; --d) {
    const auto s = shape[(std::size_t)d];
    idx[(std::size_t)d] = (s == 0) ? 0 : (lin % s);
    lin = (s == 0) ? 0 : (lin / s);
  }
}

void add_inplace_at(
    ScalarType st,
    std::byte* dst_raw,
    const std::byte* src_raw,
    std::int64_t dst_off,
    std::int64_t src_off) {
  if (st == ScalarType::Float64) {
    reinterpret_cast<double*>(dst_raw)[dst_off] += reinterpret_cast<const double*>(src_raw)[src_off];
    return;
  }
  if (st == ScalarType::Float32) {
    reinterpret_cast<float*>(dst_raw)[dst_off] += reinterpret_cast<const float*>(src_raw)[src_off];
    return;
  }
  if (st == ScalarType::Float16) {
    auto* outp = reinterpret_cast<Float16*>(dst_raw);
    const auto* srcp = reinterpret_cast<const Float16*>(src_raw);
    outp[dst_off] =
        float16_from_float(float16_to_float(outp[dst_off]) + float16_to_float(srcp[src_off]));
    return;
  }
  if (st == ScalarType::BFloat16) {
    auto* outp = reinterpret_cast<BFloat16*>(dst_raw);
    const auto* srcp = reinterpret_cast<const BFloat16*>(src_raw);
    outp[dst_off] =
        bfloat16_from_float(bfloat16_to_float(outp[dst_off]) + bfloat16_to_float(srcp[src_off]));
    return;
  }
  if (st == ScalarType::Int64) {
    reinterpret_cast<std::int64_t*>(dst_raw)[dst_off] +=
        reinterpret_cast<const std::int64_t*>(src_raw)[src_off];
    return;
  }
  if (st == ScalarType::Int32) {
    reinterpret_cast<std::int32_t*>(dst_raw)[dst_off] +=
        reinterpret_cast<const std::int32_t*>(src_raw)[src_off];
    return;
  }
  if (st == ScalarType::Int16) {
    reinterpret_cast<std::int16_t*>(dst_raw)[dst_off] +=
        reinterpret_cast<const std::int16_t*>(src_raw)[src_off];
    return;
  }
  if (st == ScalarType::Int8) {
    reinterpret_cast<std::int8_t*>(dst_raw)[dst_off] +=
        reinterpret_cast<const std::int8_t*>(src_raw)[src_off];
    return;
  }
  if (st == ScalarType::UInt64) {
    reinterpret_cast<std::uint64_t*>(dst_raw)[dst_off] +=
        reinterpret_cast<const std::uint64_t*>(src_raw)[src_off];
    return;
  }
  if (st == ScalarType::UInt32) {
    reinterpret_cast<std::uint32_t*>(dst_raw)[dst_off] +=
        reinterpret_cast<const std::uint32_t*>(src_raw)[src_off];
    return;
  }
  if (st == ScalarType::UInt16) {
    reinterpret_cast<std::uint16_t*>(dst_raw)[dst_off] +=
        reinterpret_cast<const std::uint16_t*>(src_raw)[src_off];
    return;
  }
  if (st == ScalarType::UInt8) {
    reinterpret_cast<std::uint8_t*>(dst_raw)[dst_off] +=
        reinterpret_cast<const std::uint8_t*>(src_raw)[src_off];
    return;
  }
  if (st == ScalarType::Complex64) {
    reinterpret_cast<Complex64*>(dst_raw)[dst_off] +=
        reinterpret_cast<const Complex64*>(src_raw)[src_off];
    return;
  }
  if (st == ScalarType::Complex128) {
    reinterpret_cast<Complex128*>(dst_raw)[dst_off] +=
        reinterpret_cast<const Complex128*>(src_raw)[src_off];
    return;
  }
  throw_error("index_put: accumulate dtype not implemented");
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
    if (!idx[(std::size_t)od].defined()) { idxp.push_back(pluss::series::undefined()); continue; }
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
    if (!idx[(std::size_t)od].defined()) { idxp.push_back(pluss::series::undefined()); continue; }
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


Series index(const Series& a, pluss::msharp::RangeView::Of<Series> indices) {
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
    if (need) return index_torch_like(a, std::vector<Series>(indices.begin(), indices.end()));
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
  auto bidx = broadcast_advanced_indices(idxc);
  std::vector<const std::int64_t*> bidx_ptrs;
  bidx_ptrs.reserve((std::size_t)k);
  for (const auto& bi : bidx) {
    PLUSS_CHECK(bi.scalar_type() == ScalarType::Int64, "index: internal broadcast index dtype mismatch");
    bidx_ptrs.push_back(reinterpret_cast<const std::int64_t*>(bi.raw_data()));
  }

  // Output shape = bshape + remaining dims (k..nd-1)
  std::vector<std::int64_t> out_shape = bshape;
  for (std::int64_t d=k; d<nd; ++d) out_shape.push_back(a.sizes()[(std::size_t)d]);

  Series out = Series::empty(out_shape, a.scalar_type());

  // Important: do not force `a.contiguous()` here.
  // For large permuted inputs this can materialize multi-GB temporaries and OOM.
  const auto dt = a.dtype();
  const auto* ap = a.raw_data();
  auto* op = const_cast<std::byte*>(out.raw_data());

  const std::int64_t head_total = compute_numel(bshape);

  // Tail logical size over remaining dims.
  std::int64_t tail = 1;
  for (std::int64_t d=k; d<nd; ++d) tail *= a.sizes()[(std::size_t)d];
  const std::size_t tail_bytes = (std::size_t)tail * dt.itemsize;

  // Determine whether remaining dims are contiguous in source layout.
  bool tail_contiguous = true;
  std::int64_t expect = 1;
  for (std::int64_t d = nd - 1; d >= k; --d) {
    const auto sz = a.sizes()[(std::size_t)d];
    const auto st = a.strides()[(std::size_t)d];
    if (sz > 1 && st != expect) {
      tail_contiguous = false;
      break;
    }
    expect *= sz;
  }

  // For non-contiguous tails, precompute per-tail logical element offsets once.
  std::vector<std::int64_t> tail_offsets;
  if (!tail_contiguous && tail > 0) {
    std::vector<std::int64_t> rem_shape;
    std::vector<std::int64_t> rem_strides;
    rem_shape.reserve((std::size_t)(nd - k));
    rem_strides.reserve((std::size_t)(nd - k));
    for (std::int64_t d = k; d < nd; ++d) {
      rem_shape.push_back(a.sizes()[(std::size_t)d]);
      rem_strides.push_back(a.strides()[(std::size_t)d]);
    }
    tail_offsets.resize((std::size_t)tail, 0);
    std::vector<std::int64_t> ridx(rem_shape.size(), 0);
    for (std::int64_t t = 0; t < tail; ++t) {
      lin_to_idx(t, rem_shape, ridx);
      std::int64_t off_t = 0;
      for (std::size_t j = 0; j < ridx.size(); ++j) {
        off_t += ridx[j] * rem_strides[j];
      }
      tail_offsets[(std::size_t)t] = off_t;
    }
  }

  for (std::int64_t lin=0; lin<head_total; ++lin) {
    // compute base offset in a for indexed dims (leading)
    std::int64_t base_off = 0;
    for (std::int64_t d=0; d<k; ++d) {
      auto kidx = bidx_ptrs[(std::size_t)d][lin];
      const auto dim_size = a.sizes()[(std::size_t)d];
      if (kidx < 0) kidx += dim_size;
      if (kidx < 0 || kidx >= dim_size) throw_error("index: index out of range");
      base_off += kidx * a.strides()[(std::size_t)d];
    }

    const std::int64_t out_base = lin * tail;
    if (tail_contiguous) {
      std::memcpy(op + (std::ptrdiff_t)out_base * (std::ptrdiff_t)dt.itemsize,
                  ap + (std::ptrdiff_t)base_off * (std::ptrdiff_t)dt.itemsize,
                  tail_bytes);
      continue;
    }

    for (std::int64_t t = 0; t < tail; ++t) {
      const std::int64_t src_off = base_off + tail_offsets[(std::size_t)t];
      std::memcpy(
          op + (std::ptrdiff_t)(out_base + t) * (std::ptrdiff_t)dt.itemsize,
          ap + (std::ptrdiff_t)src_off * (std::ptrdiff_t)dt.itemsize,
          dt.itemsize);
    }
  }

  return out;
}

Series index_put(const Series& a, pluss::msharp::RangeView::Of<Series> indices, const Series& values, bool accumulate) {
  if (!a.defined() || !values.defined()) throw_error("index_put: undefined");
  if (indices.empty()) throw_error("index_put: empty indices");


  // General Torch-like mixed indexing path (non-leading advanced, mixed bool/int, multi-d masks).
  if ((std::int64_t)indices.size() == a.dim()) {
    bool need = false;
    for (const auto& t : indices) {
      if (!t.defined()) { need = true; break; }
      if (t.scalar_type() == ScalarType::Bool) { need = true; break; }
    }
    if (need) {
      return index_put_torch_like(
          a,
          std::vector<Series>(indices.begin(), indices.end()),
          values,
          accumulate);
    }
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

    const Series mm = mask;
    if (values.scalar_type() != a.scalar_type()) throw_error("index_put: values dtype mismatch");
    auto out = a.clone();
    Series vv = values.contiguous();
    if (storage_aliases(out, vv)) vv = vv.clone().contiguous();

    const auto* mp = reinterpret_cast<const std::uint8_t*>(mm.raw_data());
    auto* op = const_cast<std::byte*>(out.raw_data());
    const auto* vp = vv.raw_data();
    const auto dt = out.dtype();

    const bool scalar_value = (vv.numel() == 1);
    const auto nnz = [&]() -> std::int64_t {
      if (out.is_contiguous() && mm.is_contiguous()) {
        std::int64_t c = 0;
        const auto n = out.numel();
        for (std::int64_t i = 0; i < n; ++i) {
          if (mp[i]) ++c;
        }
        return c;
      }
      std::int64_t c = 0;
      SeriesIterator count_it = SeriesIterator::unary(mm, out);
      count_it.for_each_index([&](std::int64_t, std::int64_t m_off, std::int64_t, std::int64_t) {
        if (mp[m_off]) ++c;
      });
      return c;
    }();

    if (!(scalar_value || vv.numel() == nnz)) {
      throw_error("index_put: values must be scalar or match nnz");
    }
    if (nnz == 0) {
      return out;
    }

    auto write_value = [&](std::int64_t out_off, std::int64_t src_off) {
      if (accumulate) {
        add_inplace_at(out.scalar_type(), op, vp, out_off, src_off);
      } else {
        std::memcpy(
            op + static_cast<std::ptrdiff_t>(out_off) * static_cast<std::ptrdiff_t>(dt.itemsize),
            vp + static_cast<std::ptrdiff_t>(src_off) * static_cast<std::ptrdiff_t>(dt.itemsize),
            dt.itemsize);
      }
    };

    if (out.is_contiguous() && mm.is_contiguous()) {
      std::int64_t src_pos = 0;
      const auto n = out.numel();
      for (std::int64_t i = 0; i < n; ++i) {
        if (!mp[i]) continue;
        write_value(i, scalar_value ? 0 : src_pos);
        if (!scalar_value) ++src_pos;
      }
      return out;
    }

    std::int64_t src_pos = 0;
    SeriesIterator it = SeriesIterator::unary(mm, out);
    it.for_each_index([&](std::int64_t, std::int64_t m_off, std::int64_t, std::int64_t out_off) {
      if (!mp[m_off]) return;
      write_value(out_off, scalar_value ? 0 : src_pos);
      if (!scalar_value) ++src_pos;
    });
    return out;
  }


  const auto nd = a.dim();

  auto idx = normalize_indices_to_nd(std::vector<Series>(indices.begin(), indices.end()), nd);

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
  const auto badv = broadcast_advanced_indices(adv_tensors);
  std::vector<const std::int64_t*> bidx_ptrs;
  bidx_ptrs.reserve(badv.size());
  for (const auto& bi : badv) {
    PLUSS_CHECK(bi.scalar_type() == ScalarType::Int64, "index_put: internal broadcast index dtype mismatch");
    bidx_ptrs.push_back(reinterpret_cast<const std::int64_t*>(bi.raw_data()));
  }

  const bool adv_contiguous = [&](){
    if (adv_dims.empty()) return true;
    for (std::size_t i=1;i<adv_dims.size();++i) if (adv_dims[i] != adv_dims[i-1] + 1) return false;
    return true;
  }();

  // tail dims: all dims not in adv_dims are sliced through
  std::vector<char> is_adv((std::size_t)nd, 0);
  for (auto d : adv_dims) is_adv[(std::size_t)d]=1;

  auto out = a.clone();
  if (values.scalar_type() != out.scalar_type()) throw_error("index_put: values dtype mismatch");

  std::vector<std::int64_t> tail_sizes;
  std::vector<std::int64_t> tail_strides;
  tail_sizes.reserve((std::size_t)nd);
  tail_strides.reserve((std::size_t)nd);
  for (std::int64_t d=0; d<nd; ++d) {
    if (!is_adv[(std::size_t)d]) {
      tail_sizes.push_back(out.sizes()[(std::size_t)d]);
      tail_strides.push_back(out.strides()[(std::size_t)d]);
    }
  }
  const std::int64_t tail_total = compute_numel(tail_sizes);

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

  // Precompute tail element offsets once.
  std::vector<std::int64_t> tail_offsets;
  tail_offsets.resize((std::size_t)tail_total, 0);
  if (tail_total > 0) {
    std::vector<std::int64_t> tidx(tail_sizes.size(), 0);
    for (std::int64_t tj = 0; tj < tail_total; ++tj) {
      lin_to_idx(tj, tail_sizes, tidx);
      std::int64_t off_t = 0;
      for (std::size_t q = 0; q < tail_sizes.size(); ++q) {
        off_t += tidx[q] * tail_strides[q];
      }
      tail_offsets[(std::size_t)tj] = off_t;
    }
  }

  // If advanced dims are contiguous and source is fully sized, the source
  // logical order is [before_nonadv..., bshape..., after_nonadv...].
  const bool need_adv_contig_full_map = adv_contiguous && v_full;
  std::vector<std::int64_t> bshape_v_strides;
  std::vector<std::int64_t> tail_v_offsets;
  std::vector<std::int64_t> bidx;
  if (need_adv_contig_full_map) {
    const std::int64_t first_adv = adv_dims.front();
    const std::int64_t last_adv = adv_dims.back();

    std::vector<std::int64_t> before_sizes;
    std::vector<std::int64_t> after_sizes;
    before_sizes.reserve((std::size_t)nd);
    after_sizes.reserve((std::size_t)nd);
    for (std::int64_t d = 0; d < nd; ++d) {
      if (is_adv[(std::size_t)d]) continue;
      if (d < first_adv) before_sizes.push_back(out.sizes()[(std::size_t)d]);
      else if (d > last_adv) after_sizes.push_back(out.sizes()[(std::size_t)d]);
    }

    std::vector<std::int64_t> expected_sizes;
    expected_sizes.reserve(before_sizes.size() + bshape.size() + after_sizes.size());
    expected_sizes.insert(expected_sizes.end(), before_sizes.begin(), before_sizes.end());
    expected_sizes.insert(expected_sizes.end(), bshape.begin(), bshape.end());
    expected_sizes.insert(expected_sizes.end(), after_sizes.begin(), after_sizes.end());
    const auto expected_strides = contiguous_strides(expected_sizes);

    bshape_v_strides.assign(
        expected_strides.begin() + static_cast<std::ptrdiff_t>(before_sizes.size()),
        expected_strides.begin() + static_cast<std::ptrdiff_t>(before_sizes.size() + bshape.size()));

    tail_v_offsets.resize((std::size_t)tail_total, 0);
    if (tail_total > 0) {
      const std::size_t nbefore = before_sizes.size();
      const std::size_t nafter = after_sizes.size();
      std::vector<std::int64_t> tidx(tail_sizes.size(), 0);
      for (std::int64_t tj = 0; tj < tail_total; ++tj) {
        lin_to_idx(tj, tail_sizes, tidx);
        std::int64_t off_t = 0;
        for (std::size_t i = 0; i < nbefore; ++i) {
          off_t += tidx[i] * expected_strides[i];
        }
        for (std::size_t i = 0; i < nafter; ++i) {
          off_t += tidx[nbefore + i] * expected_strides[nbefore + bshape.size() + i];
        }
        tail_v_offsets[(std::size_t)tj] = off_t;
      }
    }
    bidx.assign(bshape.size(), 0);
  }

  auto add_inplace = [&](std::int64_t out_off, std::int64_t v_off) {
    add_inplace_at(out.scalar_type(), op, vp, out_off, v_off);
  };

  for (std::int64_t lin=0; lin<head_total; ++lin) {
    std::int64_t base_off = 0;
    for (std::size_t i=0; i<bidx_ptrs.size(); ++i) {
      auto kidx = bidx_ptrs[i][lin];
      const auto dim = adv_dims[i];
      const auto dim_size = out.sizes()[(std::size_t)dim];
      if (kidx < 0) kidx += dim_size;
      if (kidx < 0 || kidx >= dim_size) throw_error("index_put: index out of range");
      base_off += kidx * out.strides()[(std::size_t)dim];
    }

    std::int64_t b_off = 0;
    if (need_adv_contig_full_map) {
      lin_to_idx(lin, bshape, bidx);
      for (std::size_t j = 0; j < bidx.size(); ++j) {
        b_off += bidx[j] * bshape_v_strides[j];
      }
    }

    for (std::int64_t tj=0; tj<tail_total; ++tj) {
      const std::int64_t out_off = base_off + tail_offsets[(std::size_t)tj];
      if (track_overlap) {
        const auto inserted = touched.insert(out_off);
        if (!inserted.second) {
          throw_error("index_put: overlapping indices with accumulate=false");
        }
      }

      std::int64_t v_off = 0;
      if (v_scalar) {
        v_off = 0;
      } else if (v_head) {
        v_off = lin;
      } else if (v_tail) {
        v_off = tj;
      } else if (v_packed || !need_adv_contig_full_map) {
        v_off = lin * tail_total + tj;
      } else {
        v_off = b_off + tail_v_offsets[(std::size_t)tj];
      }

      if (accumulate) {
        add_inplace(out_off, v_off);
      } else {
        std::memcpy(op + (std::ptrdiff_t)out_off * (std::ptrdiff_t)dt.itemsize,
                    vp + (std::ptrdiff_t)v_off  * (std::ptrdiff_t)dt.itemsize,
                    dt.itemsize);
      }
    }
  }

  return out;
}

void index_put_inplace(Series& a, pluss::msharp::RangeView::Of<Series> indices, const Series& values, bool accumulate) {
  Series out = index_put(a, indices, values, accumulate);
  copy_into(a, out);
}

} // namespace pluss::codem::native::cpu
