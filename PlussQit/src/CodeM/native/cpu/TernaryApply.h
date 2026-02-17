#pragma once

#include "CodeM/core/SeriesIterator.h"
#include "CodeM/core/SeriesSlim.h"
#include "CodeM/core/Half.h"

#include <algorithm>

static bool fast_contiguous_ternary_ok_(const pluss::codem::Series& a, const pluss::codem::Series& b, const pluss::codem::Series& c, const pluss::codem::Series& out) {
  if (!(a.is_contiguous() && b.is_contiguous() && c.is_contiguous() && out.is_contiguous())) return false;
  if (!(a.numel() == b.numel() && a.numel() == c.numel() && a.numel() == out.numel())) return false;
  auto as = a.sizes();
  auto bs = b.sizes();
  auto cs = c.sizes();
  auto os = out.sizes();
  if (!(as.size() == bs.size() && as.size() == cs.size() && as.size() == os.size())) return false;
  return std::equal(as.begin(), as.end(), bs.begin()) && std::equal(as.begin(), as.end(), cs.begin()) && std::equal(as.begin(), as.end(), os.begin());
}

namespace pluss::codem::native::cpu {

template <class T, class Op>
static void ternary_apply(const Series& m, const Series& a, const Series& b, Series& out, Op&& op) {
  if (fast_contiguous_ternary_ok_(m,a,b,out)) {
    const auto n = out.numel();
    const auto* mp = reinterpret_cast<const T*>(m.raw_data());
    const auto* ap = reinterpret_cast<const T*>(a.raw_data());
    const auto* bp = reinterpret_cast<const T*>(b.raw_data());
    auto* optr = reinterpret_cast<T*>(out.raw_mutable_data());
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
    for (std::int64_t i = 0; i < n; ++i) optr[i] = op(mp[i], ap[i], bp[i]);
    return;
  }
  SeriesIterator it = SeriesIterator::ternary(m, a, b, out);
  const auto* mp = reinterpret_cast<const T*>(m.raw_data());
  const auto* ap = reinterpret_cast<const T*>(a.raw_data());
  const auto* bp = reinterpret_cast<const T*>(b.raw_data());
  auto* optr = reinterpret_cast<T*>(out.raw_mutable_data());
  it.for_each_index_ternary([&](std::int64_t, std::int64_t mo, std::int64_t ao, std::int64_t bo, std::int64_t oo){
    optr[oo] = op(mp[mo], ap[ao], bp[bo]);
  });
}

template <class HalfT, class FromF32, class ToF32, class Op>
static void ternary_half_apply(const Series& m, const Series& a, const Series& b, Series& out, Op&& op, FromF32&& from, ToF32&& to) {
  if (fast_contiguous_ternary_ok_(m,a,b,out)) {
    const auto n = out.numel();
    const auto* mp = reinterpret_cast<const HalfT*>(m.raw_data());
    const auto* ap = reinterpret_cast<const HalfT*>(a.raw_data());
    const auto* bp = reinterpret_cast<const HalfT*>(b.raw_data());
    auto* optr = reinterpret_cast<HalfT*>(out.raw_mutable_data());
#if defined(__clang__)
#pragma clang loop vectorize(enable)
#pragma clang loop interleave(enable)
#endif
    for (std::int64_t i = 0; i < n; ++i) {
      const float xm = to(mp[i]);
      const float xa = to(ap[i]);
      const float xb = to(bp[i]);
      optr[i] = from(op(xm, xa, xb));
    }
    return;
  }
  SeriesIterator it = SeriesIterator::ternary(m, a, b, out);
  const auto* mp = reinterpret_cast<const HalfT*>(m.raw_data());
  const auto* ap = reinterpret_cast<const HalfT*>(a.raw_data());
  const auto* bp = reinterpret_cast<const HalfT*>(b.raw_data());
  auto* optr = reinterpret_cast<HalfT*>(out.raw_mutable_data());
  it.for_each_index_ternary([&](std::int64_t, std::int64_t mo, std::int64_t ao, std::int64_t bo, std::int64_t oo){
    const float xm = to(mp[mo]);
    const float xa = to(ap[ao]);
    const float xb = to(bp[bo]);
    optr[oo] = from(op(xm, xa, xb));
  });
}

} // namespace pluss::codem::native::cpu
