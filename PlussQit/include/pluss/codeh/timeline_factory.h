#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "pluss/codeh/codeh.h"
#include "pluss/codeh/timeline_moment.h"
#include "pluss/detail/factory_containers.h"
#include "pluss/detail/nested_timeline_literal.h"

#include "CodeH/compat/Invoke.h"

namespace pluss {

namespace detail {

inline void check_codeh_rank(detail::ShapeSpec sizes, std::string_view where) {
  CODEM_CHECK(sizes.rank() <= 3, std::string(where) + ": Timeline supports ranks up to 3");
}

} // namespace detail

// ---- Public factories (preferred construction path) ----
//
// make_timeline("2026-02-15")          -> scalar Timeline (0D)
// make_timeline({"2026-02-15","2026-02-16"}) -> 1D Timeline
// make_timeline({{{...}}})             -> up to 3D Timeline via nested literal
// make_timeline(flat, sizes)           -> N-D Timeline (0D..3D)
// from_epoch_nanos(...)               -> from int64 epoch nanos
//
// Duration factories:
// duration_seconds(5) -> scalar Duration

inline pluss::codeh::Timeline make_timeline(std::string_view s) { return pluss::codeh::moment(s); }
inline pluss::codeh::Timeline make_timeline(const char* s) { return pluss::codeh::moment(s); }
inline pluss::codeh::Timeline make_timeline(detail::TextSpan s1d) { return pluss::codeh::moment(s1d); }
inline pluss::codeh::Timeline make_timeline(std::span<const std::string_view> s1d) { return pluss::codeh::moment(s1d); }
inline pluss::codeh::Timeline make_timeline(const std::vector<std::string_view>& s1d) { return pluss::codeh::moment(s1d); }
inline pluss::codeh::Timeline make_timeline(std::initializer_list<const char*> s1d) { return pluss::codeh::moment(s1d); }
inline pluss::codeh::Timeline make_timeline(std::initializer_list<detail::NestedTimelineLiteral> nested) {
  return pluss::codeh::moment(nested);
}

inline pluss::codeh::Timeline make_timeline(detail::TextSpan flat,
                                            detail::ShapeSpec sizes) {
  detail::check_codeh_rank(sizes, "make_timeline");
  return pluss::codeh::ops_internal::timeline_from_utf8(flat.span(), sizes.span());
}

inline pluss::codeh::Timeline make_timeline(std::span<const std::string_view> flat,
                                           std::span<const std::int64_t> sizes) {
  return make_timeline(detail::TextSpan(flat), detail::ShapeSpec(sizes));
}

inline pluss::codeh::Timeline make_timeline(const std::vector<std::string_view>& flat,
                                            const std::vector<std::int64_t>& sizes) {
  return make_timeline(detail::TextSpan(flat), detail::ShapeSpec(sizes));
}

inline pluss::codeh::Timeline make_timeline(const std::vector<std::string>& flat,
                                            const std::vector<std::int64_t>& sizes) {
  return make_timeline(detail::TextSpan::from_strings(flat), detail::ShapeSpec(sizes));
}

inline pluss::codeh::Timeline from_epoch_nanos(std::int64_t scalar) {
  const std::array<std::int64_t, 1> flat{scalar};
  return pluss::codeh::ops_internal::timeline_from_epoch_nanos(flat, {});
}

inline pluss::codeh::Timeline from_epoch_nanos(
    std::initializer_list<detail::NestedInt64Literal> nested) {
  auto lit = detail::flatten_int64_literal(nested, 3, "from_epoch_nanos");
  return pluss::codeh::ops_internal::timeline_from_epoch_nanos(
      std::span<const std::int64_t>(lit.flat.data(), lit.flat.size()),
      std::span<const std::int64_t>(lit.sizes.data(), lit.sizes.size()));
}

inline pluss::codeh::Timeline from_epoch_nanos(std::span<const std::int64_t> flat,
                                               std::span<const std::int64_t> sizes) {
  const detail::ShapeSpec shape(sizes);
  detail::check_codeh_rank(shape, "from_epoch_nanos");
  return pluss::codeh::ops_internal::timeline_from_epoch_nanos(flat, shape.span());
}

inline pluss::codeh::Timeline from_epoch_nanos(const std::vector<std::int64_t>& flat,
                                               const std::vector<std::int64_t>& sizes) {
  return from_epoch_nanos(std::span<const std::int64_t>(flat.data(), flat.size()),
                          std::span<const std::int64_t>(sizes.data(), sizes.size()));
}

// ---- Duration ----
inline pluss::codeh::Duration duration_nanos(std::int64_t nanos) {
  const std::array<std::int64_t,1> flat{nanos};
  return pluss::codeh::ops_internal::duration_from_nanos(flat, {});
}

inline pluss::codeh::Duration duration_seconds(std::int64_t s) {
  return duration_nanos(s * 1000000000LL);
}

inline pluss::codeh::Duration duration_minutes(std::int64_t m) {
  return duration_seconds(m * 60LL);
}

inline pluss::codeh::Duration duration_hours(std::int64_t h) {
  return duration_minutes(h * 60LL);
}

inline pluss::codeh::Duration duration_days(std::int64_t d) {
  return duration_hours(d * 24LL);
}

// ---- Range helpers ----
inline pluss::codeh::Duration timedelta_range(const pluss::codeh::Duration& start,
                                              const pluss::codeh::Duration& end,
                                              const pluss::codeh::Duration& step) {
  return pluss::codeh::Duration::range(start, end, step);
}

inline pluss::codeh::Timeline date_range(const pluss::codeh::Timeline& start,
                                         const pluss::codeh::Timeline& end,
                                         std::string_view freq,
                                         std::int64_t step = 1) {
  return ::pluss::codeh::_compat::invoke<pluss::codeh::Timeline>("date_range", start, {end, std::string(freq), step});
}

} // namespace pluss
