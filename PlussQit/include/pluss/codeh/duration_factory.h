#pragma once

#include <array>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <vector>

#include "pluss/codeh/codeh.h"
#include "pluss/codeh/internal/timeline_factory_detail.h"
#include "pluss/codem/codem.h"
#include "pluss/detail/factory_containers.h"
#include "pluss/detail/nested_timeline_literal.h"
#include "pluss/msharp/msharp.h"

#include "CodeH/compat/Invoke.h"

// `Duration` (capital D) names the type; `pluss::duration` (lowercase) is its
// dedicated factory namespace -- the construction surface, kept separate from
// the type itself the same way `pluss::make_timeline`/`pluss::from_epoch_nanos`
// are kept separate from `Timeline`. `Duration` has no public constructors or
// construction statics of its own anymore (see Duration.h) -- everything
// public goes through here. Reuses Timeline's detail-level rank-check and
// nested-literal-flattening machinery (`timeline_factory_detail.h`,
// `nested_timeline_literal.h`) since both factories build N-D int64-backed
// arrays the same way, but does not include `timeline_factory.h` itself --
// `Duration` doesn't depend on `Timeline` to be constructed.
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

// Every scaled factory funnels through these three: build the *unscaled* (raw
// tick count) Duration via the same ops_internal::duration_from_nanos() every
// other Duration construction path uses, then let Duration's own `mul()` do
// the scaling -- reusing the engine's one canonical, already-tested
// elementwise-multiply implementation (with correct NaT propagation) instead
// of a second, parallel scaling loop living only in this header. `unit_nanos
// == 1` (the `nanos()` factories) skips the multiply entirely rather than
// paying for a no-op `* 1` pass.

inline pluss::codeh::Duration from_scaled_scalar_(std::int64_t count, std::int64_t unit_nanos) {
  const std::array<std::int64_t, 1> flat{count};
  pluss::codeh::Duration raw = pluss::codeh::ops_internal::duration_from_nanos(flat, {});
  return unit_nanos == 1 ? raw : pluss::codeh::detail::impl_mul_i64(raw, unit_nanos);
}

inline pluss::codeh::Duration from_scaled_nested_(
    std::initializer_list<pluss::detail::NestedInt64Literal> nested,
    std::int64_t unit_nanos,
    const char* who) {
  auto lit = pluss::detail::flatten_int64_literal(nested, 3, who);
  pluss::codeh::Duration raw = pluss::codeh::ops_internal::duration_from_nanos(
      std::span<const std::int64_t>(lit.flat.data(), lit.flat.size()),
      std::span<const std::int64_t>(lit.sizes.data(), lit.sizes.size()));
  return unit_nanos == 1 ? raw : pluss::codeh::detail::impl_mul_i64(raw, unit_nanos);
}

inline pluss::codeh::Duration from_scaled_flat_(
    pluss::RangeView<std::int64_t> flat,
    pluss::RangeView<std::int64_t> sizes,
    std::int64_t unit_nanos,
    const char* who) {
  const pluss::detail::ShapeSpec shape(std::span<const std::int64_t>(sizes.data(), sizes.size()));
  pluss::codeh::factory_detail::check_rank(shape, who);
  pluss::codeh::Duration raw = pluss::codeh::ops_internal::duration_from_nanos(
      std::span<const std::int64_t>(flat.data(), flat.size()), shape.span());
  return unit_nanos == 1 ? raw : pluss::codeh::detail::impl_mul_i64(raw, unit_nanos);
}

} // namespace factory_detail_

// pluss::duration::nanos(5)                    -> scalar Duration (0D)
// pluss::duration::nanos({5, 10, 15})           -> 1D Duration
// pluss::duration::nanos({{5, 10}, {15, 20}})   -> up to 3D via nested literal
// pluss::duration::nanos(flat, sizes)           -> N-D Duration (0D..3D), explicit shape
//   (flat/sizes accept a plain braced list directly -- {1,2,3,4} converts to
//   pluss::RangeView<int64_t> implicitly, no explicit wrapping needed)
//
// seconds/minutes/hours/days/weeks/micros/millis all follow the exact same
// four-overload shape, just scaled up to nanoseconds before construction --
// every unit here supports scalar, 1D, nested-N-D, and explicit flat+shape
// construction. `make(..., Unit)` below is the same surface again, but with
// the unit picked at runtime instead of by function name.

inline pluss::codeh::Duration nanos(std::int64_t count) {
  return factory_detail_::from_scaled_scalar_(count, 1);
}
inline pluss::codeh::Duration nanos(std::initializer_list<pluss::detail::NestedInt64Literal> nested) {
  return factory_detail_::from_scaled_nested_(nested, 1, "pluss::duration::nanos");
}
inline pluss::codeh::Duration nanos(pluss::RangeView<std::int64_t> flat,
                                    pluss::RangeView<std::int64_t> sizes) {
  return factory_detail_::from_scaled_flat_(flat, sizes, 1, "pluss::duration::nanos");
}

inline pluss::codeh::Duration microseconds(std::int64_t count) {
  return factory_detail_::from_scaled_scalar_(count, factory_detail_::unit_nanos_(Unit::Microseconds));
}
inline pluss::codeh::Duration microseconds(std::initializer_list<pluss::detail::NestedInt64Literal> nested) {
  return factory_detail_::from_scaled_nested_(nested, factory_detail_::unit_nanos_(Unit::Microseconds), "pluss::duration::microseconds");
}
inline pluss::codeh::Duration microseconds(pluss::RangeView<std::int64_t> flat,
                                           pluss::RangeView<std::int64_t> sizes) {
  return factory_detail_::from_scaled_flat_(flat, sizes, factory_detail_::unit_nanos_(Unit::Microseconds), "pluss::duration::microseconds");
}

inline pluss::codeh::Duration milliseconds(std::int64_t count) {
  return factory_detail_::from_scaled_scalar_(count, factory_detail_::unit_nanos_(Unit::Milliseconds));
}
inline pluss::codeh::Duration milliseconds(std::initializer_list<pluss::detail::NestedInt64Literal> nested) {
  return factory_detail_::from_scaled_nested_(nested, factory_detail_::unit_nanos_(Unit::Milliseconds), "pluss::duration::milliseconds");
}
inline pluss::codeh::Duration milliseconds(pluss::RangeView<std::int64_t> flat,
                                           pluss::RangeView<std::int64_t> sizes) {
  return factory_detail_::from_scaled_flat_(flat, sizes, factory_detail_::unit_nanos_(Unit::Milliseconds), "pluss::duration::milliseconds");
}

inline pluss::codeh::Duration seconds(std::int64_t count) {
  return factory_detail_::from_scaled_scalar_(count, factory_detail_::unit_nanos_(Unit::Seconds));
}
inline pluss::codeh::Duration seconds(std::initializer_list<pluss::detail::NestedInt64Literal> nested) {
  return factory_detail_::from_scaled_nested_(nested, factory_detail_::unit_nanos_(Unit::Seconds), "pluss::duration::seconds");
}
inline pluss::codeh::Duration seconds(pluss::RangeView<std::int64_t> flat,
                                      pluss::RangeView<std::int64_t> sizes) {
  return factory_detail_::from_scaled_flat_(flat, sizes, factory_detail_::unit_nanos_(Unit::Seconds), "pluss::duration::seconds");
}

inline pluss::codeh::Duration minutes(std::int64_t count) {
  return factory_detail_::from_scaled_scalar_(count, factory_detail_::unit_nanos_(Unit::Minutes));
}
inline pluss::codeh::Duration minutes(std::initializer_list<pluss::detail::NestedInt64Literal> nested) {
  return factory_detail_::from_scaled_nested_(nested, factory_detail_::unit_nanos_(Unit::Minutes), "pluss::duration::minutes");
}
inline pluss::codeh::Duration minutes(pluss::RangeView<std::int64_t> flat,
                                      pluss::RangeView<std::int64_t> sizes) {
  return factory_detail_::from_scaled_flat_(flat, sizes, factory_detail_::unit_nanos_(Unit::Minutes), "pluss::duration::minutes");
}

inline pluss::codeh::Duration hours(std::int64_t count) {
  return factory_detail_::from_scaled_scalar_(count, factory_detail_::unit_nanos_(Unit::Hours));
}
inline pluss::codeh::Duration hours(std::initializer_list<pluss::detail::NestedInt64Literal> nested) {
  return factory_detail_::from_scaled_nested_(nested, factory_detail_::unit_nanos_(Unit::Hours), "pluss::duration::hours");
}
inline pluss::codeh::Duration hours(pluss::RangeView<std::int64_t> flat,
                                    pluss::RangeView<std::int64_t> sizes) {
  return factory_detail_::from_scaled_flat_(flat, sizes, factory_detail_::unit_nanos_(Unit::Hours), "pluss::duration::hours");
}

inline pluss::codeh::Duration days(std::int64_t count) {
  return factory_detail_::from_scaled_scalar_(count, factory_detail_::unit_nanos_(Unit::Days));
}
inline pluss::codeh::Duration days(std::initializer_list<pluss::detail::NestedInt64Literal> nested) {
  return factory_detail_::from_scaled_nested_(nested, factory_detail_::unit_nanos_(Unit::Days), "pluss::duration::days");
}
inline pluss::codeh::Duration days(pluss::RangeView<std::int64_t> flat,
                                   pluss::RangeView<std::int64_t> sizes) {
  return factory_detail_::from_scaled_flat_(flat, sizes, factory_detail_::unit_nanos_(Unit::Days), "pluss::duration::days");
}

inline pluss::codeh::Duration weeks(std::int64_t count) {
  return factory_detail_::from_scaled_scalar_(count, factory_detail_::unit_nanos_(Unit::Weeks));
}
inline pluss::codeh::Duration weeks(std::initializer_list<pluss::detail::NestedInt64Literal> nested) {
  return factory_detail_::from_scaled_nested_(nested, factory_detail_::unit_nanos_(Unit::Weeks), "pluss::duration::weeks");
}
inline pluss::codeh::Duration weeks(pluss::RangeView<std::int64_t> flat,
                                    pluss::RangeView<std::int64_t> sizes) {
  return factory_detail_::from_scaled_flat_(flat, sizes, factory_detail_::unit_nanos_(Unit::Weeks), "pluss::duration::weeks");
}

// pluss::duration::make(5, Unit::Days)                    -> scalar
// pluss::duration::make({1, 2, 3}, Unit::Hours)            -> 1D
// pluss::duration::make({{1, 2}, {3, 4}}, Unit::Minutes)   -> nested N-D
// pluss::duration::make(flat, sizes, Unit::Seconds)        -> explicit N-D
//
// The same construction surface as the named functions above, generalized to
// take the unit as a runtime value -- for call sites where the unit isn't
// known until runtime (e.g. read from a config file or chosen by a template/
// generic code path) and naming five near-identical functions by hand isn't
// an option.
inline pluss::codeh::Duration make(std::int64_t count, Unit unit) {
  return factory_detail_::from_scaled_scalar_(count, factory_detail_::unit_nanos_(unit));
}
inline pluss::codeh::Duration make(std::initializer_list<pluss::detail::NestedInt64Literal> nested, Unit unit) {
  return factory_detail_::from_scaled_nested_(nested, factory_detail_::unit_nanos_(unit), "pluss::duration::make");
}
inline pluss::codeh::Duration make(pluss::RangeView<std::int64_t> flat,
                                   pluss::RangeView<std::int64_t> sizes,
                                   Unit unit) {
  return factory_detail_::from_scaled_flat_(flat, sizes, factory_detail_::unit_nanos_(unit), "pluss::duration::make");
}

// pluss::duration::range(start, end, step) -> 1D Duration, like pandas timedelta_range.
// Inclusive start, stops before exceeding end; all three arguments are
// themselves scalar Durations, typically built from one of the factories above.
inline pluss::codeh::Duration range(const pluss::codeh::Duration& start,
                                    const pluss::codeh::Duration& end,
                                    const pluss::codeh::Duration& step) {
  return pluss::codeh::detail::impl_range(start, end, step);
}

} // namespace pluss::duration
