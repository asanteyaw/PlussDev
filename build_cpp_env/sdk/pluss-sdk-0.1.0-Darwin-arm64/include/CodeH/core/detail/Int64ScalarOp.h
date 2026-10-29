#pragma once

#include <cstdint>

#include "CodeM/core/Series.h"
#include "CodeM/core/SeriesIterator.h"

namespace pluss::codeh::detail {

// Shared int64 elementwise-scalar fast path for Duration/Timeline's NaT-aware
// scalar arithmetic (Duration's mul/div/floordiv/mod, Timeline's add_nanos).
//
// Contiguous same-shape in/out get a plain __restrict loop the compiler can
// auto-vectorize; anything non-contiguous or broadcast falls back to the
// general SeriesIterator path. `op` is responsible for its own NaT branch --
// kNaT (INT64_MIN) combined with an arbitrary scalar is signed-overflow UB
// for most operations, so the sentinel must be checked *before* computing,
// not masked out afterward. This is why Duration/Timeline don't route this
// through CodeM's engine-level ops_internal::add_i64/mul_i64/etc (see
// Arithmetic.cpp) -- those compute unconditionally across a contiguous
// range with no per-element branch.
//
// Promote this further (e.g. into CodeM) only if a consumer outside
// CodeH's NaT-sentinel model ever needs the identical shape.
template <class Op>
void apply_unary_i64_scalar(const pluss::codem::Series& a, pluss::codem::Series& out, Op&& op) {
  const auto* av = reinterpret_cast<const std::int64_t*>(a.raw_data());
  auto* o = reinterpret_cast<std::int64_t*>(out.raw_data());
  if (a.is_contiguous() && out.is_contiguous()) {
    const std::int64_t n = a.numel();
    const std::int64_t* __restrict avp = av;
    std::int64_t* __restrict op_ = o;
    for (std::int64_t i = 0; i < n; ++i) op_[i] = op(avp[i]);
    return;
  }
  auto it = pluss::codem::SeriesIterator::unary(a, out);
  it.for_each_index([&](std::int64_t, std::int64_t ao, std::int64_t, std::int64_t oo) {
    o[oo] = op(av[ao]);
  });
}

} // namespace pluss::codeh::detail
