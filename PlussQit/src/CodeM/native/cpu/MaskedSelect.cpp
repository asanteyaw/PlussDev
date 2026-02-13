#include "CodeM/core/Errors.h"
#include "CodeM/core/Series.h"
#include "CodeM/core/ScalarType.h"
#include "CodeM/core/SeriesIterator.h"

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
    else throw_error("masked_select: mask not broadcastable");
  }
  return out_strides;
}

Series masked_select(const Series& a, const Series& mask) {
  if (!a.defined() || !mask.defined()) throw_error("masked_select: undefined");
  if (mask.scalar_type() != ScalarType::Bool) throw_error("masked_select: mask must be bool");

  // Torch-like: mask broadcastable to a (or vice versa); iteration is broadcast_shape(a, mask).
  auto shape = SeriesIterator::broadcast_shape(a.sizes(), mask.sizes());

  auto aa = a.contiguous();
  auto mm = mask.contiguous();

  const auto dt = aa.dtype();
  const auto* ap = aa.raw_data();
  const auto* mp = reinterpret_cast<const std::uint8_t*>(mm.raw_data());

  auto a_es = expand_strides(aa.sizes(), aa.strides(), shape);
  auto m_es = expand_strides(mm.sizes(), mm.strides(), shape);

  const std::int64_t total = compute_numel(shape);

  // Count
  std::vector<std::int64_t> idx(shape.size(), 0);
  auto lin_to_idx = [&](std::int64_t lin) {
    for (std::int64_t d = (std::int64_t)shape.size() - 1; d >= 0; --d) {
      const auto s = shape[(std::size_t)d];
      idx[(std::size_t)d] = (s == 0) ? 0 : (lin % s);
      lin = (s == 0) ? 0 : (lin / s);
    }
  };
  auto off = [&](const std::vector<std::int64_t>& st) -> std::int64_t {
    std::int64_t o = 0;
    for (std::size_t i = 0; i < idx.size(); ++i) o += idx[i] * st[i];
    return o;
  };

  std::int64_t count = 0;
  for (std::int64_t lin = 0; lin < total; ++lin) {
    lin_to_idx(lin);
    const auto mo = off(m_es);
    if (mp[mo]) ++count;
  }

  Series out = Series::empty({count}, aa.scalar_type());
  auto* out_b = const_cast<std::byte*>(out.raw_data());

  std::int64_t j = 0;
  for (std::int64_t lin = 0; lin < total; ++lin) {
    lin_to_idx(lin);
    const auto mo = off(m_es);
    if (!mp[mo]) continue;
    const auto ao = off(a_es);
    std::memcpy(out_b + (std::ptrdiff_t)j * (std::ptrdiff_t)dt.itemsize,
                ap + (std::ptrdiff_t)ao * (std::ptrdiff_t)dt.itemsize,
                dt.itemsize);
    ++j;
  }
  return out;
}

} // namespace pluss::codem::native::cpu
