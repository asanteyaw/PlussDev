#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <random>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "MSharp/RangeView.h"
#include "pluss/codeh/codeh.h"
#include "pluss/codeh/duration_factory.h"
#include "pluss/codeh/internal/int64_flat_reader.h"
#include "pluss/codeh/internal/timeline_factory_detail.h"
#include "pluss/codeh/internal/timeline_text_reader.h"
#include "pluss/codem/series_factory.h"
#include "pluss/detail/factory_containers.h"
#include "pluss/detail/factory_core.h"
#include "pluss/detail/nested_timeline_literal.h"

#include "CodeH/compat/Invoke.h"

namespace pluss::timeline {

// ---- Public factories (preferred construction path) ----
//
// make("2026-02-15")          -> scalar Timeline (0D)
// make({"2026-02-15","2026-02-16"}) -> 1D Timeline
// make({{{...}}})             -> up to 3D Timeline via nested literal
// make(flat, sizes)           -> N-D Timeline (0D..3D)
// from_epoch(...)             -> from int64 epoch nanos
//
// Duration has its own dedicated factory namespace, `pluss::duration::`
// (`pluss::duration::seconds(5)`, `pluss::duration::range(...)`, etc.) --
// see duration_factory.h, included above for transitive availability.

// One reader-taking overload covers scalar/1D (three source shapes)/nested
// N-D via a single implicit conversion at the call site; two more overloads
// cover the explicit flat+shape form (two source shapes), which -- like
// Duration's unit factories and Timeline::from_epoch -- can't fold into the
// reader-taking overload since two call-site arguments can't implicitly
// combine into one converted parameter. See timeline_text_reader.h.
inline pluss::Timeline make(pluss::codeh::detail::TimelineTextReader data) {
  return pluss::codeh::factory_detail::from_textspan(data.flat(), data.shape());
}
inline pluss::Timeline make(pluss::msharp::RangeView::Of<std::string_view> flat,
                            pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return make(pluss::codeh::detail::TimelineTextReader(flat, sizes));
}
inline pluss::Timeline make(pluss::msharp::RangeView::Of<std::string> flat,
                            pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return make(pluss::codeh::detail::TimelineTextReader(flat, sizes));
}

// Same Int64FlatReader as Duration's unit factories (see int64_flat_reader.h
// for why this is NOT built on Series' DataReader -- precision). Two
// overloads, not one, for the same reason Duration's unit factories need
// two: `from_epoch(data)` covers scalar/nested/bare-1D via implicit
// conversion of a single call-site argument; `from_epoch(flat, sizes)`
// covers the explicit two-argument form, which can't implicitly fold into
// one converted parameter.
inline pluss::Timeline from_epoch(pluss::codeh::detail::Int64FlatReader data) {
  return pluss::codeh::ops_internal::timeline_from_epoch_nanos(data.flat(), data.sizes());
}
inline pluss::Timeline from_epoch(pluss::msharp::RangeView::Of<std::int64_t> flat,
                                  pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return from_epoch(pluss::codeh::detail::Int64FlatReader(flat, sizes));
}

// Same as from_epoch(data)/from_epoch(flat, sizes) above, but stamps the
// resulting Timeline with an explicit display timezone instead of defaulting
// to UTC. The underlying instant is identical either way -- epoch nanoseconds
// are canonical and tz-independent (see Timeline.h's class comment); only
// which timezone .fmt()/.hour()/.dt-field accessors read wall-clock
// components in changes. Reuses timeline_from_epoch_nanos for the shape
// validation/construction it already does, then re-wraps the resulting
// ticks Series through timeline_from_series (already used directly in this
// file, see epoch() above) to attach the requested tz label.
inline pluss::Timeline from_epoch(pluss::codeh::detail::Int64FlatReader data, std::string_view tz_name) {
  pluss::Timeline t = pluss::codeh::ops_internal::timeline_from_epoch_nanos(data.flat(), data.sizes());
  return pluss::codeh::ops_internal::timeline_from_series(t.unsafe_ticks(), std::string(tz_name));
}
inline pluss::Timeline from_epoch(pluss::msharp::RangeView::Of<std::int64_t> flat,
                                  pluss::msharp::RangeView::Of<std::int64_t> sizes,
                                  std::string_view tz_name) {
  return from_epoch(pluss::codeh::detail::Int64FlatReader(flat, sizes), tz_name);
}

inline pluss::Timeline date_range(const pluss::Timeline& start,
                                  const pluss::Timeline& end,
                                  std::string_view freq,
                                  std::int64_t step = 1) {
  return ::pluss::codeh::_compat::invoke<pluss::Timeline>("date_range", start, {end, std::string(freq), step});
}

// pluss::timeline::unite(values) -- joins N Timelines into one, in the order
// given. Unlike pluss::series::unite/pluss::unite (a real dim-aware N-D
// join, since Series has genuine per-dimension pile/unite methods), this
// wraps Timeline::concat, which always flattens every operand to 1D first --
// there is no Timeline::pile (no way to stack into a higher-rank result),
// and unite here does not take a `dim` the way Series' does, since Timeline
// has no dim-aware join to call. If N-D-preserving pile/unite for Timeline
// (or Duration/Note/TimeIndex) is needed later, that's new engine work, not
// a rename of this.
inline pluss::Timeline unite(pluss::msharp::RangeView::Of<pluss::Timeline> values) {
  CODEM_CHECK(!values.empty(), "pluss::timeline::unite: at least one Timeline is required");
  if (values.size() == 1) {
    return values.front();
  }
  return values.front().concat(values.drop_front().vec());
}
inline pluss::Timeline unite(std::initializer_list<pluss::Timeline> values) {
  return unite(pluss::msharp::RangeView::Of<pluss::Timeline>(values));
}

// pluss::timeline::print(t) -- human-readable rendering, mirroring
// pluss::duration::print/pluss::print_series. `std::cout << t` also works
// directly via pluss::codeh's Timeline operator<<.
inline std::string print(const pluss::Timeline& t) {
  return pluss::codeh::print_timeline(t);
}
inline std::string print(const pluss::Timeline& t, const pluss::TimelinePrintOptions& opt) {
  return pluss::codeh::print_timeline(t, opt);
}
inline std::ostream& print(std::ostream& os, const pluss::Timeline& t,
                           const pluss::TimelinePrintOptions& opt = {}) {
  return pluss::codeh::print_timeline(os, t, opt);
}

// ---- Shape factories (pluss::zeros/full/random-inspired, Timeline-native) ----
//
// pluss::timeline::epoch(sizes) -> Timeline(s) at the Unix epoch (0 epoch
// nanoseconds). The Timeline-domain equivalent of pluss::duration::zeros /
// pluss::zeros: not a meaningless all-zero instant, but the one instant a
// Timeline can legitimately be "empty" toward -- 1970-01-01T00:00:00Z.
// Routed through Series::zeros' own memset-based factory (wrapped, not
// copied, by ops_internal::timeline_from_series).
inline pluss::Timeline epoch(pluss::msharp::RangeView::Int sizes = {}) {
  return pluss::codeh::ops_internal::timeline_from_series(
      pluss::series::zeros(sizes, pluss::codem::ScalarType::Int64));
}
inline pluss::Timeline epoch(std::initializer_list<std::int64_t> sizes) {
  return epoch(pluss::msharp::RangeView::Int(sizes));
}

// pluss::timeline::full(sizes, fill_value) -> shape filled with a repeated
// scalar Timeline instant. Stays int64-exact (no double round trip) by
// reusing the same flat-vector + from_epoch path the rest of this factory
// already uses, rather than Series::full's double-typed fill_value.
inline pluss::Timeline full(pluss::msharp::RangeView::Int sizes, const pluss::Timeline& fill_value) {
  CODEM_CHECK(fill_value.dim() == 0, "pluss::timeline::full: fill_value must be a scalar Timeline");
  std::int64_t n = 1;
  for (const auto s : sizes) n *= s;
  const std::int64_t ticks = pluss::codeh::detail::impl_to_epoch_nanos_flat(fill_value)[0];
  const std::vector<std::int64_t> flat(static_cast<std::size_t>(n), ticks);
  return pluss::codeh::ops_internal::timeline_from_epoch_nanos(flat, sizes);
}
inline pluss::Timeline full(std::initializer_list<std::int64_t> sizes, const pluss::Timeline& fill_value) {
  return full(pluss::msharp::RangeView::Int(sizes), fill_value);
}

// pluss::timeline::full_like(like, fill_value) -> same shape as `like`, filled with fill_value.
inline pluss::Timeline full_like(const pluss::Timeline& like, const pluss::Timeline& fill_value) {
  return full(like.sizes(), fill_value);
}

// pluss::timeline::random(sizes, lo, hi, seed) -> uniform-random instants in
// [lo, hi) -- e.g. synthetic timestamp generation for test/demo data. Built
// on a real 64-bit RNG + std::uniform_int_distribution<int64_t> (the same
// exact-integer convention pluss::duration::random above and
// pluss::poisson/pluss::bernoulli/pluss::randperm already use), not a
// double-based Series::rand scaling, which would lose precision once
// (hi - lo) in nanoseconds exceeds a double's ~2^53 mantissa (about 104 days
// -- i.e. almost any realistic "random date within a few years" range).
inline pluss::Timeline random(pluss::msharp::RangeView::Int sizes,
                              const pluss::Timeline& lo,
                              const pluss::Timeline& hi,
                              std::uint64_t seed = 0) {
  CODEM_CHECK(lo.dim() == 0 && hi.dim() == 0, "pluss::timeline::random: lo/hi must be scalar Timelines");
  const std::int64_t lo_ns = pluss::codeh::detail::impl_to_epoch_nanos_flat(lo)[0];
  const std::int64_t hi_ns = pluss::codeh::detail::impl_to_epoch_nanos_flat(hi)[0];
  CODEM_CHECK(hi_ns > lo_ns, "pluss::timeline::random: expected hi > lo");
  std::int64_t n = 1;
  for (const auto s : sizes) n *= s;
  auto rng = pluss::detail::FactoryCore::make_rng(seed);
  std::uniform_int_distribution<std::int64_t> dist(lo_ns, hi_ns - 1);
  std::vector<std::int64_t> flat(static_cast<std::size_t>(n));
  for (auto& v : flat) v = dist(rng);
  return pluss::codeh::ops_internal::timeline_from_epoch_nanos(flat, sizes);
}
inline pluss::Timeline random(std::initializer_list<std::int64_t> sizes,
                              const pluss::Timeline& lo,
                              const pluss::Timeline& hi,
                              std::uint64_t seed = 0) {
  return random(pluss::msharp::RangeView::Int(sizes), lo, hi, seed);
}

} // namespace pluss::timeline

namespace pluss {

// pluss::make_timeline is the one flat spelling kept as a permanent
// shortcut (mirroring pluss::make_series/make_note/make_timeindex/
// make_duration) for the common construct-from-data case. Every other flat
// Timeline spelling (pluss::from_epoch_nanos, pluss::date_range) was a
// transitional compatibility alias and has been removed now that all
// internal call sites across the codebase go through pluss::timeline::*
// directly.
//
// Each overload mirrors pluss::timeline::make's parameter list exactly (not
// a generic forwarding template): a template can't deduce a type from a
// bare {...} braced-list argument in ANY position, including the `sizes`
// parameter of the (flat, sizes) overloads below -- e.g.
// pluss::make_timeline(some_vector, {2, 2}) needs the second parameter to
// already be the concrete RangeView type for the braced-list-to-RangeView
// implicit conversion to kick in.
inline pluss::Timeline make_timeline(std::string_view s) {
  return pluss::timeline::make(s);
}
inline pluss::Timeline make_timeline(const char* s) {
  return pluss::timeline::make(s);
}
inline pluss::Timeline make_timeline(std::initializer_list<const char*> s1d) {
  return pluss::timeline::make(s1d);
}
inline pluss::Timeline make_timeline(pluss::msharp::RangeView::Of<std::string_view> s1d) {
  return pluss::timeline::make(s1d);
}
inline pluss::Timeline make_timeline(pluss::msharp::RangeView::Of<std::string> s1d) {
  return pluss::timeline::make(s1d);
}
inline pluss::Timeline make_timeline(
    std::initializer_list<pluss::detail::NestedTimelineLiteral> nested) {
  return pluss::timeline::make(nested);
}
inline pluss::Timeline make_timeline(pluss::msharp::RangeView::Of<std::string_view> flat,
                                     pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return pluss::timeline::make(flat, sizes);
}
inline pluss::Timeline make_timeline(pluss::msharp::RangeView::Of<std::string> flat,
                                     pluss::msharp::RangeView::Of<std::int64_t> sizes) {
  return pluss::timeline::make(flat, sizes);
}

// Do NOT delete these below. They are for future reference 
// [[deprecated("use pluss::timeline::from_epoch instead")]]

// inline pluss::Timeline from_epoch_nanos(std::int64_t scalar) {

//   return pluss::timeline::from_epoch(scalar);

// }

// [[deprecated("use pluss::timeline::from_epoch instead")]]

// inline pluss::Timeline from_epoch_nanos(

//     std::initializer_list<pluss::detail::NestedInt64Literal> nested) {

//   return pluss::timeline::from_epoch(nested);

// }

// [[deprecated("use pluss::timeline::from_epoch instead")]]

// inline pluss::Timeline from_epoch_nanos(pluss::msharp::RangeView::Of<std::int64_t> flat,

//                                         pluss::msharp::RangeView::Of<std::int64_t> sizes) {

//   return pluss::timeline::from_epoch(flat, sizes);

// }



// [[deprecated("use pluss::timeline::date_range instead")]]

// inline pluss::Timeline date_range(const pluss::Timeline& start,

//                                   const pluss::Timeline& end,

//                                   std::string_view freq,

//                                   std::int64_t step = 1) {

//   return pluss::timeline::date_range(start, end, freq, step);

// }

} // namespace pluss

// Keep moment helpers transitively available from timeline_factory.
#include "pluss/codeh/timeline_moment.h"
