#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/Errors.h"
#include "CodeM/core/SeriesImpl.h"
#include "MSharp/Parallel.h"

#include <algorithm>
#include <cstring>

namespace pluss::codem {


SeriesIterator SeriesIterator::ternary(const Series& m, const Series& a, const Series& b, const Series& out) {
  SeriesIterator it;
  it.m_ = m;
  it.a_ = a;
  it.b_ = b;
  it.out_ = out;
  it.shape_ = broadcast_shape(broadcast_shape(m.sizes(), a.sizes()), b.sizes());
  // Expand strides to broadcasted shape
  it.m_strides_ = expand_strides(m.sizes(), m.strides(), it.shape_);
  it.a_strides_ = expand_strides(a.sizes(), a.strides(), it.shape_);
  it.b_strides_ = expand_strides(b.sizes(), b.strides(), it.shape_);
  it.o_strides_ = expand_strides(out.sizes(), out.strides(), it.shape_);
  it.coalesce_dims();
  return it;
}

void SeriesIterator::for_each_index_ternary(
    const std::function<void(std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t)>& fn) const {
  const std::int64_t nd = (std::int64_t)shape_.size();
  if (nd == 0) {
    fn(0, 0, 0, 0, 0);
    return;
  }
  // Simple recursive-less iteration: maintain index vector
  std::vector<std::int64_t> idx((size_t)nd, 0);
  const std::int64_t n = numel();
  // Parallel threshold and body similar to for_each_index
  constexpr std::int64_t grain = 1 << 14;
  auto linear_to_index = [&](std::int64_t lin, std::vector<std::int64_t>& idx_out) {
    for (std::int64_t i = nd - 1; i >= 0; --i) {
      const auto s = shape_[(size_t)i];
      idx_out[(size_t)i] = (s == 0) ? 0 : (lin % s);
      lin = (s == 0) ? 0 : (lin / s);
    }
  };

  auto offset = [&](const std::vector<std::int64_t>& strides, const std::vector<std::int64_t>& idx_in) -> std::int64_t {
    std::int64_t off = 0;
    for (std::size_t i = 0; i < idx_in.size(); ++i) off += idx_in[i] * strides[i];
    return off;
  };

  if (n >= (1 << 18)) {
    auto body = [&](std::int64_t b0, std::int64_t b1){
      std::vector<std::int64_t> local_idx((size_t)nd);
      for (std::int64_t lin = b0; lin < b1; ++lin) {
        linear_to_index(lin, local_idx);
        const std::int64_t mo = offset(m_strides_, local_idx);
        const std::int64_t ao = offset(a_strides_, local_idx);
        const std::int64_t bo = offset(b_strides_, local_idx);
        const std::int64_t oo = offset(o_strides_, local_idx);
        fn(lin, mo, ao, bo, oo);
      }
    };
    pluss::msharp::parallel_for(0, n, grain, body);
    return;
  }

  for (std::int64_t lin = 0; lin < n; ++lin) {
    std::int64_t mo = 0, ao = 0, bo = 0, oo = 0;
    for (std::int64_t d = 0; d < nd; ++d) {
      mo += idx[(size_t)d] * m_strides_[(size_t)d];
      ao += idx[(size_t)d] * a_strides_[(size_t)d];
      bo += idx[(size_t)d] * b_strides_[(size_t)d];
      oo += idx[(size_t)d] * o_strides_[(size_t)d];
    }
    fn(lin, mo, ao, bo, oo);
    // increment idx
    for (std::int64_t d = nd - 1; d >= 0; --d) {
      idx[(size_t)d] += 1;
      if (idx[(size_t)d] < shape_[(size_t)d]) break;
      idx[(size_t)d] = 0;
      if (d == 0) break;
    }
  }
}

std::int64_t SeriesIterator::numel() const noexcept { return compute_numel(shape_); }

std::vector<std::int64_t> SeriesIterator::broadcast_shape(std::span<const std::int64_t> a,
                                                          std::span<const std::int64_t> b) {
  const std::size_t na = a.size(), nb = b.size();
  const std::size_t n = std::max(na, nb);
  std::vector<std::int64_t> out(n, 1);

  for (std::size_t i = 0; i < n; ++i) {
    const std::int64_t da = (i < n - na) ? 1 : a[i - (n - na)];
    const std::int64_t db = (i < n - nb) ? 1 : b[i - (n - nb)];
    if (da == db) out[i] = da;
    else if (da == 1) out[i] = db;
    else if (db == 1) out[i] = da;
    else throw_error("broadcast: incompatible shapes");
  }
  return out;
}

std::vector<std::int64_t> SeriesIterator::expand_strides(std::span<const std::int64_t> sizes,
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
    else throw_error("expand_strides: incompatible");
  }
  return out_strides;
}

void SeriesIterator::coalesce_dims() {
  // Combine adjacent dims when possible: if stride pattern suggests contiguous merge.
  if (shape_.size() <= 1) return;

  std::vector<std::int64_t> new_shape;
  std::vector<std::int64_t> nm;
  std::vector<std::int64_t> na, nb, no;

  new_shape.reserve(shape_.size());
  if (!m_strides_.empty()) nm.reserve(shape_.size());
  na.reserve(shape_.size()); nb.reserve(shape_.size()); no.reserve(shape_.size());

  auto can_merge = [&](std::size_t i, std::size_t j) {
    // Merge i and j if:
    //  - both dims are not broadcasted (stride != 0) OR both broadcasted (stride == 0)
    //  - and stride_i == shape_j * stride_j for each operand where stride != 0
    auto ok_operand = [&](const std::vector<std::int64_t>& st)->bool{
      if (st.empty()) return true;
      const auto si = st[i], sj = st[j];
      if (si == 0 && sj == 0) return true;
      if (si == 0 || sj == 0) return false;
      return si == shape_[j] * sj;
    };
    return ok_operand(m_strides_) && ok_operand(a_strides_) && ok_operand(b_strides_) && ok_operand(o_strides_);
  };

  std::size_t i = 0;
  while (i < shape_.size()) {
    std::int64_t cur = shape_[i];
    std::int64_t sm = m_strides_.empty() ? 0 : m_strides_[i];
    std::int64_t sa = a_strides_[i], sb = b_strides_[i], so = o_strides_[i];
    std::size_t j = i;
    while (j + 1 < shape_.size() && can_merge(j, j+1)) {
      cur *= shape_[j+1];
      // keep sj (the "inner" stride) after merge
      if (!m_strides_.empty()) sm = m_strides_[j+1];
      sa = a_strides_[j+1];
      sb = b_strides_[j+1];
      so = o_strides_[j+1];
      ++j;
    }
    new_shape.push_back(cur);
    if (!m_strides_.empty()) nm.push_back(sm);
    na.push_back(sa); nb.push_back(sb); no.push_back(so);
    i = j + 1;
  }

  shape_ = std::move(new_shape);
  if (!m_strides_.empty()) m_strides_ = std::move(nm);
  a_strides_ = std::move(na);
  b_strides_ = std::move(nb);
  o_strides_ = std::move(no);
}

SeriesIterator SeriesIterator::binary(const Series& a, const Series& b, const Series& out) {
  if (!a.defined() || !b.defined() || !out.defined()) throw_error("SeriesIterator: undefined");
  if (a.scalar_type() != b.scalar_type()) throw_error("SeriesIterator: dtype mismatch (binary)");

  SeriesIterator it;
  it.shape_ = broadcast_shape(a.sizes(), b.sizes());

  if ((std::size_t)out.dim() != it.shape_.size()) throw_error("out rank mismatch");
  for (std::size_t k = 0; k < it.shape_.size(); ++k)
    if (out.sizes()[k] != it.shape_[k]) throw_error("out shape mismatch");

  it.a_ = a; it.b_ = b; it.out_ = out;
  it.a_strides_ = expand_strides(a.sizes(), a.strides(), it.shape_);
  it.b_strides_ = expand_strides(b.sizes(), b.strides(), it.shape_);
  it.o_strides_ = expand_strides(out.sizes(), out.strides(), it.shape_);

  it.coalesce_dims();
  return it;
}

SeriesIterator SeriesIterator::unary(const Series& a, const Series& out) {
  if (!a.defined() || !out.defined()) throw_error("SeriesIterator: undefined");

  SeriesIterator it;
  it.shape_ = std::vector<std::int64_t>(a.sizes().begin(), a.sizes().end());

  if ((std::size_t)out.dim() != it.shape_.size()) throw_error("out rank mismatch");
  for (std::size_t k = 0; k < it.shape_.size(); ++k)
    if (out.sizes()[k] != it.shape_[k]) throw_error("out shape mismatch");

  it.a_ = a; it.b_ = a; it.out_ = out;
  it.a_strides_ = expand_strides(a.sizes(), a.strides(), it.shape_);
  it.b_strides_ = it.a_strides_;
  it.o_strides_ = expand_strides(out.sizes(), out.strides(), it.shape_);

  it.coalesce_dims();
  return it;
}

void SeriesIterator::for_each_index(const std::function<void(std::int64_t, std::int64_t, std::int64_t, std::int64_t)>& fn) const {
  const auto n = numel();
  // If coalesced down to 1D, offset == lin*stride
  if (shape_.size() == 1) {
    const auto sa = a_strides_[0], sb = b_strides_[0], so = o_strides_[0];
    auto body = [&](std::int64_t b0, std::int64_t b1){
      for (std::int64_t lin = b0; lin < b1; ++lin) {
        fn(lin, lin * sa, lin * sb, lin * so);
      }
    };
    // parallel threshold
    constexpr std::int64_t grain = 1 << 14;
    if (n >= (1 << 18)) pluss::msharp::parallel_for(0, n, grain, body);
    else body(0, n);
    return;
  }

  // Generic N-d index iteration
  if (n >= (1 << 18)) {
    // Parallel path must keep per-thread index state to avoid data races.
    auto body = [&](std::int64_t b0, std::int64_t b1) {
      std::vector<std::int64_t> idx(shape_.size(), 0);
      auto linear_to_index = [&](std::int64_t lin) {
        for (std::int64_t i = static_cast<std::int64_t>(shape_.size()) - 1; i >= 0; --i) {
          const auto s = shape_[static_cast<std::size_t>(i)];
          idx[static_cast<std::size_t>(i)] = (s == 0) ? 0 : (lin % s);
          lin = (s == 0) ? 0 : (lin / s);
        }
      };
      auto offset = [&](const std::vector<std::int64_t>& strides) -> std::int64_t {
        std::int64_t off = 0;
        for (std::size_t i = 0; i < idx.size(); ++i) off += idx[i] * strides[i];
        return off;
      };
      for (std::int64_t lin = b0; lin < b1; ++lin) {
        linear_to_index(lin);
        fn(lin, offset(a_strides_), offset(b_strides_), offset(o_strides_));
      }
    };
    constexpr std::int64_t grain = 1 << 14;
    pluss::msharp::parallel_for(0, n, grain, body);
    return;
  }

  std::vector<std::int64_t> idx(shape_.size(), 0);
  auto linear_to_index = [&](std::int64_t lin) {
    for (std::int64_t i = static_cast<std::int64_t>(shape_.size()) - 1; i >= 0; --i) {
      const auto s = shape_[static_cast<std::size_t>(i)];
      idx[static_cast<std::size_t>(i)] = (s == 0) ? 0 : (lin % s);
      lin = (s == 0) ? 0 : (lin / s);
    }
  };
  auto offset = [&](const std::vector<std::int64_t>& strides) -> std::int64_t {
    std::int64_t off = 0;
    for (std::size_t i = 0; i < idx.size(); ++i) off += idx[i] * strides[i];
    return off;
  };
  for (std::int64_t lin = 0; lin < n; ++lin) {
    linear_to_index(lin);
    fn(lin, offset(a_strides_), offset(b_strides_), offset(o_strides_));
  }
}

void SeriesIterator::copy_out_from_in(const Series& in, Series& out) const {
  const auto dt = in.dtype();
  if (in.scalar_type() != out.scalar_type()) throw_error("copy: dtype mismatch");
  if (numel() != out.numel()) throw_error("copy: numel mismatch");

  // Fast path contiguous with same shape
  if (in.is_contiguous() && out.is_contiguous() &&
      (std::size_t)in.dim() == shape_.size() &&
      (std::size_t)out.dim() == shape_.size() &&
      std::equal(shape_.begin(), shape_.end(), out.sizes().begin())) {
    std::memcpy(out.raw_mutable_data(), in.raw_data(), (std::size_t)numel() * dt.itemsize);
    return;
  }

  for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    const auto* src = in.raw_data() + (std::ptrdiff_t)ao * (std::ptrdiff_t)dt.itemsize;
    auto* dst = const_cast<std::byte*>(out.raw_data()) + (std::ptrdiff_t)oo * (std::ptrdiff_t)dt.itemsize;
    std::memcpy(dst, src, dt.itemsize);
  });
}

} // namespace pluss::codem
