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
#include "pluss/detail/nested_timeline_literal.h"

#include "CodeH/compat/Invoke.h"

namespace pluss {

namespace detail {

inline void check_codeh_rank(std::span<const std::int64_t> sizes, std::string_view where) {
  CODEM_CHECK(sizes.size() <= 3, std::string(where) + ": Timeline supports ranks up to 3");
}

} // namespace detail

// ---- Public factories (preferred construction path) ----
//
// moment("2026-02-15")                -> scalar Timeline (0D)
// moment({"2026-02-15","2026-02-16"}) -> 1D Timeline
// moment({{{...}}})                   -> up to 3D Timeline via nested literal
// make_timeline(flat, sizes)          -> N-D Timeline (0D..3D)
// from_epoch_nanos(...)               -> from int64 epoch nanos
//
// Duration factories:
// duration_seconds(5) -> scalar Duration

inline pluss::codeh::Timeline moment(std::string_view s) {
  const std::array<std::string_view,1> flat{s};
  return pluss::codeh::ops_internal::timeline_from_utf8(flat, {});
}

inline pluss::codeh::Timeline moment(const char* s) {
  return moment(std::string_view{s ? s : ""});
}

inline pluss::codeh::Timeline moment(std::span<const std::string_view> s1d) {
  const std::array<std::int64_t,1> sizes{static_cast<std::int64_t>(s1d.size())};
  return pluss::codeh::ops_internal::timeline_from_utf8(s1d, sizes);
}

inline pluss::codeh::Timeline moment(const std::vector<std::string_view>& s1d) {
  return moment(std::span<const std::string_view>(s1d.data(), s1d.size()));
}

inline pluss::codeh::Timeline moment(std::initializer_list<const char*> s1d) {
  std::vector<std::string_view> flat;
  flat.reserve(s1d.size());
  std::ranges::transform(s1d, std::back_inserter(flat),
                         [](const char* p) { return std::string_view{p ? p : ""}; });
  return moment(flat);
}

inline pluss::codeh::Timeline moment(
    std::initializer_list<detail::NestedTimelineLiteral> nested) {
  auto lit = detail::flatten_timeline_literal(nested, 3, "moment");
  std::vector<std::string_view> views(lit.flat.size());
  std::ranges::transform(lit.flat, views.begin(),
                         [](const std::string& s) { return std::string_view{s}; });
  return pluss::codeh::ops_internal::timeline_from_utf8(
      std::span<const std::string_view>(views.data(), views.size()),
      std::span<const std::int64_t>(lit.sizes.data(), lit.sizes.size()));
}

inline pluss::codeh::Timeline make_timeline(std::span<const std::string_view> flat,
                                           std::span<const std::int64_t> sizes) {
  detail::check_codeh_rank(sizes, "make_timeline");
  return pluss::codeh::ops_internal::timeline_from_utf8(flat, sizes);
}

inline pluss::codeh::Timeline make_timeline(const std::vector<std::string_view>& flat,
                                            const std::vector<std::int64_t>& sizes) {
  return make_timeline(std::span<const std::string_view>(flat.data(), flat.size()),
                       std::span<const std::int64_t>(sizes.data(), sizes.size()));
}

inline pluss::codeh::Timeline make_timeline(const std::vector<std::string>& flat,
                                            const std::vector<std::int64_t>& sizes) {
  std::vector<std::string_view> views(flat.size());
  std::ranges::transform(flat, views.begin(),
                         [](const std::string& s) { return std::string_view{s}; });
  return make_timeline(views, sizes);
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
  detail::check_codeh_rank(sizes, "from_epoch_nanos");
  return pluss::codeh::ops_internal::timeline_from_epoch_nanos(flat, sizes);
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
  return ::pluss::codeh::_compat::invoke<pluss::codeh::Timeline>("date_range", start, end, std::string(freq), step);
}

} // namespace pluss
