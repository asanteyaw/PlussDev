#pragma once

#include <cstdint>
#include <ostream>
#include <random>
#include <string>
#include <vector>

#include "pluss/codeh/codeh.h"
#include "pluss/codeh/internal/int64_flat_reader.h"
#include "pluss/codem/series_factory.h"
#include "pluss/detail/factory_core.h"

#include "CodeH/compat/Invoke.h"

// `Duration` (capital D) names the type; `pluss::duration` (lowercase) is its
// dedicated factory namespace -- the construction surface, kept separate from
// the type itself the same way `pluss::timeline::make`/`pluss::timeline::from_epoch`
// are kept separate from `Timeline`. `Duration` has no public constructors or
// construction statics of its own anymore (see Duration.h) -- everything
// public goes through here.
//
// Every unit factory takes a single Int64FlatReader parameter instead of
// separate scalar/nested/flat+shape overloads -- the reader (shared with
// Timeline::from_epoch) does the shape dispatch via its own constructor set,
// so a caller writing pluss::duration::days(90), pluss::duration::days({1,2,3}),
// or pluss::duration::days(flat, sizes) all resolve to the same one function,
// just via a different implicit conversion into Int64FlatReader. See
// int64_flat_reader.h for why this is NOT built on Series' DataReader.
namespace pluss::duration {

// The unit dispatched on by `make()`, for callers who want one entry point
// parameterized at runtime rather than picking `nanos`/`seconds`/etc. by name
// at the call site.
enum class Unit {
  Nanos,
  Microseconds,
  Milliseconds,
  Seconds,
  Minutes,
  Hours,
  Days,
  Weeks,
};

namespace factory_detail_ {

constexpr std::int64_t kNanosPerUnit[] = {
    1LL,                          // Nanos
    1'000LL,                      // Microseconds
    1'000'000LL,                  // Milliseconds
    1'000'000'000LL,              // Seconds
    60LL * 1'000'000'000LL,       // Minutes
    3'600LL * 1'000'000'000LL,    // Hours
    86'400LL * 1'000'000'000LL,   // Days
    7LL * 86'400LL * 1'000'000'000LL, // Weeks
};

constexpr std::int64_t unit_nanos_(Unit unit) {
  return kNanosPerUnit[static_cast<std::size_t>(unit)];
}

// Every scaled factory funnels through this: build the *unscaled* (raw tick
// count) Duration via the same ops_internal::duration_from_nanos() every
// other Duration construction path uses, then let Duration's own vectorized
// impl_mul_i64 do the scaling -- the engine's one canonical, already-tested
// (and int64-exact-fast-path) elementwise-multiply, not a second scaling loop
// living in this header. `unit_nanos == 1` (the `nanos()` factory) skips the
// multiply entirely rather than paying for a no-op `* 1` pass.
inline pluss::Duration from_reader_scaled_(pluss::codeh::detail::Int64FlatReader data,
                                           std::int64_t unit_nanos) {
  pluss::Duration raw = pluss::codeh::ops_internal::duration_from_nanos(data.flat(), data.sizes());
  return unit_nanos == 1 ? raw : pluss::codeh::detail::impl_mul_i64(raw, unit_nanos);
}

} // namespace factory_detail_

// pluss::duration::nanos(5)                    -> scalar Duration (0D)
// pluss::duration::nanos({5, 10, 15})           -> 1D Duration
// pluss::duration::nanos({{5, 10}, {15, 20}})   -> up to 3D via nested literal
// pluss::duration::nanos(flat, sizes)           -> N-D Duration (0D..3D), explicit shape
// pluss::duration::nanos(existing_vector)       -> 1D Duration from an existing container
//
// seconds/minutes/hours/days/weeks/microseconds/milliseconds all follow the
// same shape, just scaled up to nanoseconds before construction. `make(...,
// Unit)` below is the same surface again, but with the unit picked at
// runtime instead of by function name.

// Two overloads per unit, not one: `days(data)` covers scalar/nested/bare-1D
// (a single call-site argument implicitly converts to Int64FlatReader), and
// `days(flat, sizes)` covers the explicit-shape form -- two separate
// call-site arguments can't implicitly combine into one converted parameter,
// so this one has to stay a real second overload (the same reason
// pluss::make_series(flat, sizes) takes shape as a distinct second parameter
// rather than folding it into DataReader's own conversion).
#define PLUSS_DURATION_UNIT_FACTORY_(name, unit) \
  inline pluss::Duration name(pluss::codeh::detail::Int64FlatReader data) { \
    return factory_detail_::from_reader_scaled_(std::move(data), factory_detail_::unit_nanos_(unit)); \
  } \
  inline pluss::Duration name(pluss::msharp::RangeView::Of<std::int64_t> flat, \
                              pluss::msharp::RangeView::Of<std::int64_t> sizes) { \
    return name(pluss::codeh::detail::Int64FlatReader(flat, sizes)); \
  }

PLUSS_DURATION_UNIT_FACTORY_(nanos, Unit::Nanos)
PLUSS_DURATION_UNIT_FACTORY_(microseconds, Unit::Microseconds)
PLUSS_DURATION_UNIT_FACTORY_(milliseconds, Unit::Milliseconds)
PLUSS_DURATION_UNIT_FACTORY_(seconds, Unit::Seconds)
PLUSS_DURATION_UNIT_FACTORY_(minutes, Unit::Minutes)
PLUSS_DURATION_UNIT_FACTORY_(hours, Unit::Hours)
PLUSS_DURATION_UNIT_FACTORY_(days, Unit::Days)
PLUSS_DURATION_UNIT_FACTORY_(weeks, Unit::Weeks)

#undef PLUSS_DURATION_UNIT_FACTORY_

// pluss::duration::make(5, Unit::Days)                    -> scalar
// pluss::duration::make({1, 2, 3}, Unit::Hours)            -> 1D
// pluss::duration::make({{1, 2}, {3, 4}}, Unit::Minutes)   -> nested N-D
// pluss::duration::make(flat, sizes, Unit::Seconds)        -> explicit N-D
//
// The same construction surface as the named functions above, generalized to
// take the unit as a runtime value -- for call sites where the unit isn't
// known until runtime (e.g. read from a config file or chosen by a template/
// generic code path) and naming eight near-identical functions by hand isn't
// an option.
inline pluss::Duration make(pluss::codeh::detail::Int64FlatReader data, Unit unit) {
  return factory_detail_::from_reader_scaled_(std::move(data), factory_detail_::unit_nanos_(unit));
}
inline pluss::Duration make(pluss::msharp::RangeView::Of<std::int64_t> flat,
                            pluss::msharp::RangeView::Of<std::int64_t> sizes,
                            Unit unit) {
  return make(pluss::codeh::detail::Int64FlatReader(flat, sizes), unit);
}

// pluss::duration::range(start, end, step) -> 1D Duration, like pandas timedelta_range.
// Inclusive start, stops before exceeding end; all three arguments are
// themselves scalar Durations, typically built from one of the factories above.
inline pluss::Duration range(const pluss::Duration& start,
                             const pluss::Duration& end,
                             const pluss::Duration& step) {
  return pluss::codeh::detail::impl_range(start, end, step);
}

// pluss::duration::print(d) -- human-readable rendering, mirroring
// pluss::series::print/pluss::timeline::print. `std::cout << d` also works
// directly (see CodeH/core/Printing.h's Duration operator<<).
inline std::string print(const pluss::Duration& d) {
  return pluss::codeh::print_duration(d);
}
inline std::ostream& print(std::ostream& os, const pluss::Duration& d) {
  return pluss::codeh::print_duration(os, d);
}

// ---- Shape factories (pluss::zeros/full/random-inspired, Duration-native) ----
//
// pluss::duration::zeros(sizes) -> Duration of all-zero (0 nanoseconds).
// Routed through Series::zeros' own memset-based factory (wrapped, not
// copied, by ops_internal::duration_from_series) rather than a host-side
// fill + second memcpy through duration_from_nanos.
inline pluss::Duration zeros(pluss::msharp::RangeView::Int sizes) {
  return pluss::codeh::ops_internal::duration_from_series(
      pluss::series::zeros(sizes, pluss::codem::ScalarType::Int64));
}
inline pluss::Duration zeros(std::initializer_list<std::int64_t> sizes) {
  return zeros(pluss::msharp::RangeView::Int(sizes));
}

// pluss::duration::zeros_like(like) -> same shape as `like`, all zero.
inline pluss::Duration zeros_like(const pluss::Duration& like) {
  return zeros(like.sizes());
}

// pluss::duration::full(sizes, fill_value) -> shape filled with a repeated
// scalar Duration. Stays int64-exact (no double round trip) by reusing the
// same flat-vector + duration_from_nanos path the unit factories and range()
// already use, rather than Series::full's double-typed fill_value.
inline pluss::Duration full(pluss::msharp::RangeView::Int sizes, const pluss::Duration& fill_value) {
  CODEM_CHECK(fill_value.dim() == 0, "pluss::duration::full: fill_value must be a scalar Duration");
  std::int64_t n = 1;
  for (const auto s : sizes) n *= s;
  const std::int64_t nanos = pluss::codeh::detail::impl_to_nanos_flat(fill_value)[0];
  const std::vector<std::int64_t> flat(static_cast<std::size_t>(n), nanos);
  return pluss::codeh::ops_internal::duration_from_nanos(flat, sizes);
}
inline pluss::Duration full(std::initializer_list<std::int64_t> sizes, const pluss::Duration& fill_value) {
  return full(pluss::msharp::RangeView::Int(sizes), fill_value);
}

// pluss::duration::full_like(like, fill_value) -> same shape as `like`, filled with fill_value.
inline pluss::Duration full_like(const pluss::Duration& like, const pluss::Duration& fill_value) {
  return full(like.sizes(), fill_value);
}

// pluss::duration::random(sizes, lo, hi, seed) -> uniform-random Durations in
// [lo, hi) -- e.g. synthetic time-gap generation for test/demo data. Built on
// a real 64-bit RNG + std::uniform_int_distribution<int64_t> (the same
// exact-integer convention pluss::poisson/pluss::bernoulli/pluss::randperm
// already use), not a double-based Series::rand scaling, which would lose
// precision once (hi - lo) in nanoseconds exceeds a double's ~2^53 mantissa
// (about 104 days).
inline pluss::Duration random(pluss::msharp::RangeView::Int sizes,
                              const pluss::Duration& lo,
                              const pluss::Duration& hi,
                              std::uint64_t seed = 0) {
  CODEM_CHECK(lo.dim() == 0 && hi.dim() == 0, "pluss::duration::random: lo/hi must be scalar Durations");
  const std::int64_t lo_ns = pluss::codeh::detail::impl_to_nanos_flat(lo)[0];
  const std::int64_t hi_ns = pluss::codeh::detail::impl_to_nanos_flat(hi)[0];
  CODEM_CHECK(hi_ns > lo_ns, "pluss::duration::random: expected hi > lo");
  std::int64_t n = 1;
  for (const auto s : sizes) n *= s;
  auto rng = pluss::detail::FactoryCore::make_rng(seed);
  std::uniform_int_distribution<std::int64_t> dist(lo_ns, hi_ns - 1);
  std::vector<std::int64_t> flat(static_cast<std::size_t>(n));
  for (auto& v : flat) v = dist(rng);
  return pluss::codeh::ops_internal::duration_from_nanos(flat, sizes);
}
inline pluss::Duration random(std::initializer_list<std::int64_t> sizes,
                              const pluss::Duration& lo,
                              const pluss::Duration& hi,
                              std::uint64_t seed = 0) {
  return random(pluss::msharp::RangeView::Int(sizes), lo, hi, seed);
}

} // namespace pluss::duration

namespace pluss {
// Permanent flat shortcut, mirroring pluss::make_series/make_note/
// make_timeline/make_timeindex -- unlike those, Duration never had a flat
// compat surface (it was namespaced from the start), so this is a new
// addition for symmetry rather than a preserved legacy name.
inline pluss::Duration make_duration(pluss::codeh::detail::Int64FlatReader data, pluss::duration::Unit unit) {
  return pluss::duration::make(std::move(data), unit);
}
inline pluss::Duration make_duration(pluss::msharp::RangeView::Of<std::int64_t> flat,
                                     pluss::msharp::RangeView::Of<std::int64_t> sizes,
                                     pluss::duration::Unit unit) {
  return pluss::duration::make(flat, sizes, unit);
}
} // namespace pluss
