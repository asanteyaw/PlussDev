#include "CodeM/core/Errors.h"
#include "CodeM/core/Complex.h"
#include "CodeM/core/SeriesSlim.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/TypePromotion.h"
#include "CodeM/native/cpu/IndexingExtras.h"
#include "CodeM/native/cpu/AdvancedIndexingKernels.h"

#include <vector>
#include <cstring>

namespace pluss::codem::native::cpu {

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
    else throw_error("broadcast: incompatible shapes");
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

// where(cond, a, b) : cond bool broadcastable; a,b broadcastable; output shape = broadcast(cond, a, b)
Series where(const Series& cond, const Series& a, const Series& b) {
  if (!cond.defined() || !a.defined() || !b.defined()) throw_error("where: undefined");
  if (cond.scalar_type() != ScalarType::Bool) throw_error("where: cond must be bool");

  auto shape_ab = SeriesIterator::broadcast_shape(a.sizes(), b.sizes());
  auto shape = SeriesIterator::broadcast_shape(shape_ab, cond.sizes());

  auto aa = a.contiguous();
  auto bb = b.contiguous();
  auto cc = cond.contiguous();

  const auto out_dtype = promote_types(aa.scalar_type(), bb.scalar_type());
  if (aa.scalar_type() != out_dtype) aa = aa.to(out_dtype);
  if (bb.scalar_type() != out_dtype) bb = bb.to(out_dtype);

  Series out = Series::empty(shape, out_dtype);

  auto a_es = expand_strides(aa.sizes(), aa.strides(), shape);
  auto b_es = expand_strides(bb.sizes(), bb.strides(), shape);
  auto c_es = expand_strides(cc.sizes(), cc.strides(), shape);

  const auto dt = out.dtype();
  const auto* ap = aa.raw_data();
  const auto* bp = bb.raw_data();
  const auto* cp = reinterpret_cast<const std::uint8_t*>(cc.raw_data());
  auto* op = const_cast<std::byte*>(out.raw_data());

  const std::int64_t total = compute_numel(shape);
  std::vector<std::int64_t> idx(shape.size(), 0);

  for (std::int64_t lin=0; lin<total; ++lin) {
    lin_to_idx(lin, shape, idx);
    const auto co = off(idx, c_es);
    const auto ao = off(idx, a_es);
    const auto bo = off(idx, b_es);
    const auto* src = cp[co] ? (ap + (std::ptrdiff_t)ao * (std::ptrdiff_t)dt.itemsize)
                             : (bp + (std::ptrdiff_t)bo * (std::ptrdiff_t)dt.itemsize);
    std::memcpy(op + (std::ptrdiff_t)lin * (std::ptrdiff_t)dt.itemsize, src, dt.itemsize);
  }
  return out;
}

// masked_fill(a, mask, value): returns new tensor, mask broadcastable to a.
Series masked_fill(const Series& a, const Series& mask, double value) {
  if (!a.defined() || !mask.defined()) throw_error("masked_fill: undefined");
  if (mask.scalar_type() != ScalarType::Bool) throw_error("masked_fill: mask must be bool");

  auto shape = SeriesIterator::broadcast_shape(a.sizes(), mask.sizes());
  // For torch compatibility, require shape == a.shape (mask can broadcast to a).
  if (shape != std::vector<std::int64_t>(a.sizes().begin(), a.sizes().end())) {
    throw_error("masked_fill: mask must broadcast to a.shape");
  }

  auto aa = a.contiguous();
  auto mm = mask.contiguous();

  Series out = aa.clone(); // start copy
  auto m_es = expand_strides(mm.sizes(), mm.strides(), aa.sizes());

  const auto* mp = reinterpret_cast<const std::uint8_t*>(mm.raw_data());
  const auto dt = out.dtype();
  auto* op = const_cast<std::byte*>(out.raw_data());

  // write scalar
  if (out.scalar_type() == ScalarType::Float64) {
    const double v = value;
    std::vector<std::int64_t> idx(aa.sizes().size(), 0);
    const auto total = out.numel();
    for (std::int64_t lin=0; lin<total; ++lin) {
      lin_to_idx(lin, aa.sizes(), idx);
      if (mp[off(idx, m_es)]) {
        std::memcpy(op + (std::ptrdiff_t)lin * (std::ptrdiff_t)dt.itemsize, &v, sizeof(double));
      }
    }
    return out;
  }
  if (out.scalar_type() == ScalarType::Float32) {
    const float v = (float)value;
    std::vector<std::int64_t> idx(aa.sizes().size(), 0);
    const auto total = out.numel();
    for (std::int64_t lin=0; lin<total; ++lin) {
      lin_to_idx(lin, aa.sizes(), idx);
      if (mp[off(idx, m_es)]) {
        std::memcpy(op + (std::ptrdiff_t)lin * (std::ptrdiff_t)dt.itemsize, &v, sizeof(float));
      }
    }
    return out;
  }
  throw_error("masked_fill: dtype not implemented (float32/float64 only)");
}


// masked_fill_(a, mask, value): in-place, mask broadcastable to a (must not require broadcasting a).
void masked_fill_inplace(Series& a, const Series& mask, double value) {
  if (!a.defined() || !mask.defined()) throw_error("masked_fill_: undefined");
  if (mask.scalar_type() != ScalarType::Bool) throw_error("masked_fill_: mask must be bool");

  auto shape = SeriesIterator::broadcast_shape(a.sizes(), mask.sizes());
  if (shape != std::vector<std::int64_t>(a.sizes().begin(), a.sizes().end())) {
    throw_error("masked_fill_: mask must broadcast to a.shape");
  }

  auto aa = a.contiguous(); // we require contiguous for in-place speed and to avoid complex stride writes
  auto mm = mask.contiguous();
  auto m_es = expand_strides(mm.sizes(), mm.strides(), aa.sizes());

  const auto* mp = reinterpret_cast<const std::uint8_t*>(mm.raw_data());
  auto* outp = reinterpret_cast<std::uint8_t*>(aa.raw_data());
  const auto dt = aa.dtype();

  // write scalar bytes once
  std::array<std::uint8_t, 16> buf{};
  PLUSS_CHECK(dt.itemsize <= (std::int64_t)buf.size(), "masked_fill_: dtype too large");
  Series tmp = Series::full({1}, value, aa.scalar_type());
  std::memcpy(buf.data(), tmp.raw_data(), (std::size_t)dt.itemsize);

  const auto n = aa.numel();
  std::vector<std::int64_t> idx(shape.size(), 0);
  for (std::int64_t lin=0; lin<n; ++lin) {
    lin_to_idx(lin, aa.sizes(), idx);
    std::int64_t mo = 0;
    for (std::size_t d=0; d<idx.size(); ++d) mo += idx[d]*m_es[d];
    if (mp[mo]) {
      std::memcpy(outp + (std::ptrdiff_t)lin * (std::ptrdiff_t)dt.itemsize, buf.data(), (std::size_t)dt.itemsize);
    }
  }

  // write back into original a if it was already contiguous, else replace storage (Torch-like for contiguous requirement)
  if (a.is_contiguous()) {
    std::memcpy(a.raw_data(), aa.raw_data(), (std::size_t)(dt.itemsize * aa.numel()));
  } else {
    // enforce policy: non-contiguous in-place masked_fill_ is currently disallowed.
    throw_error("masked_fill_: non-contiguous target not supported yet");
  }
}


// nonzero(mask): returns [nnz, mask.dim()] int64 indices
Series nonzero(const Series& mask) {
  if (!mask.defined()) throw_error("nonzero: undefined");
  if (mask.scalar_type() != ScalarType::Bool) throw_error("nonzero: mask must be bool");
  auto mm = mask.contiguous();
  const auto* mp = reinterpret_cast<const std::uint8_t*>(mm.raw_data());
  const auto total = mm.numel();

  std::int64_t count = 0;
  for (std::int64_t i=0;i<total;++i) if (mp[i]) ++count;

  const std::int64_t nd = mm.dim();
  Series out = Series::empty({count, nd}, ScalarType::Int64);
  auto* op = reinterpret_cast<std::int64_t*>(out.raw_data());

  std::vector<std::int64_t> idx((std::size_t)nd, 0);
  for (std::int64_t lin=0, j=0; lin<total; ++lin) {
    if (!mp[lin]) continue;
    lin_to_idx(lin, mm.sizes(), idx);
    for (std::int64_t d=0; d<nd; ++d) op[j*nd + d] = idx[(std::size_t)d];
    ++j;
  }
  return out;
}

// take(a, index): index int64 into flattened a (supports negative indices),
// preserving index shape in the output.
Series take(const Series& a, const Series& index) {
  if (!a.defined() || !index.defined()) throw_error("take: undefined");
  if (index.scalar_type() != ScalarType::Int64) throw_error("take: index must be int64");
  auto aa = a.contiguous();
  auto idx = index.contiguous();

  const auto* ip = reinterpret_cast<const std::int64_t*>(idx.raw_data());
  const auto n = idx.numel();
  Series out = Series::empty(idx.sizes(), aa.scalar_type());

  const auto dt = aa.dtype();
  const auto* ap = aa.raw_data();
  auto* op = const_cast<std::byte*>(out.raw_data());

  const auto total = aa.numel();
  for (std::int64_t i=0;i<n;++i) {
    std::int64_t k = ip[i];
    if (k < 0) k += total;
    if (k < 0 || k >= total) throw_error("take: index out of range");
    std::memcpy(op + (std::ptrdiff_t)i * (std::ptrdiff_t)dt.itemsize,
                ap + (std::ptrdiff_t)k * (std::ptrdiff_t)dt.itemsize,
                dt.itemsize);
  }
  return out;
}

// put(a, index, src): returns a copy with updates. src can be scalar (numel=1)
// or same numel as index (flattened).
Series put(const Series& a, const Series& index, const Series& src, bool accumulate) {

  if (!a.defined() || !index.defined() || !src.defined()) throw_error("put: undefined");
  if (index.scalar_type() != ScalarType::Int64) throw_error("put: index must be int64");
  auto out = a.contiguous().clone();
  auto idx = index.contiguous();
  auto ss = src.contiguous();

  if (ss.scalar_type() != out.scalar_type()) throw_error("put: src dtype mismatch");
  const auto n = idx.numel();
  if (!(ss.numel() == 1 || ss.numel() == n)) throw_error("put: src must be scalar or same numel as index");

  const auto* ip = reinterpret_cast<const std::int64_t*>(idx.raw_data());
  const auto dt = out.dtype();
  auto* op = const_cast<std::byte*>(out.raw_data());
  const auto* sp = ss.raw_data();

  const auto total = out.numel();

  for (std::int64_t i=0;i<n;++i) {
    std::int64_t k = ip[i];
    if (k < 0) k += total;
    if (k < 0 || k >= total) throw_error("put: index out of range");
    const auto s_off = (ss.numel() == 1) ? 0 : i;
    if (!accumulate) {
      std::memcpy(op + (std::ptrdiff_t)k * (std::ptrdiff_t)dt.itemsize,
                  sp + (std::ptrdiff_t)s_off * (std::ptrdiff_t)dt.itemsize,
                  dt.itemsize);
    } else {
      add_inplace_at(out.scalar_type(), op, sp, k, s_off);
    }
  }
  return out;
}

} // namespace pluss::codem::native::cpu
