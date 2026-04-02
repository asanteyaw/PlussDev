#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "pluss/codeh/codeh.h"
#include "pluss/detail/nested_timeline_literal.h"

namespace pluss {

// Public factories (preferred): construct a 1D TimeIndex.
// Users MUST NOT construct TimeIndex directly.
inline pluss::codeh::TimeIndex make_timeindex(const pluss::codeh::Timeline& t, bool require_sorted = true) {
  return ::pluss::codeh::_compat::invoke<pluss::codeh::TimeIndex>("make_timeindex", t, require_sorted);
}

inline pluss::codeh::TimeIndex make_timeindex(std::string_view scalar, bool require_sorted = true) {
  const std::array<std::string_view, 1> flat{scalar};
  const std::array<std::int64_t, 1> size{1};
  const auto t = ::pluss::codeh::ops_internal::timeline_from_utf8(flat, size);
  return make_timeindex(t, require_sorted);
}

inline pluss::codeh::TimeIndex make_timeindex(const char* scalar, bool require_sorted = true) {
  return make_timeindex(std::string_view{scalar ? scalar : ""}, require_sorted);
}

inline pluss::codeh::TimeIndex make_timeindex(std::span<const std::string_view> s1d, bool require_sorted = true) {
  const std::array<std::int64_t, 1> size{static_cast<std::int64_t>(s1d.size())};
  const auto t = ::pluss::codeh::ops_internal::timeline_from_utf8(s1d, size);
  return make_timeindex(t, require_sorted);
}

inline pluss::codeh::TimeIndex make_timeindex(const std::vector<std::string_view>& s1d, bool require_sorted = true) {
  return make_timeindex(std::span<const std::string_view>(s1d.data(), s1d.size()), require_sorted);
}

inline pluss::codeh::TimeIndex make_timeindex(const std::vector<std::string>& s1d, bool require_sorted = true) {
  std::vector<std::string_view> views(s1d.size());
  std::ranges::transform(s1d, views.begin(),
                         [](const std::string& s) { return std::string_view{s}; });
  return make_timeindex(std::span<const std::string_view>(views.data(), views.size()), require_sorted);
}

inline pluss::codeh::TimeIndex make_timeindex(std::initializer_list<const char*> s1d, bool require_sorted = true) {
  std::vector<std::string_view> flat;
  flat.reserve(s1d.size());
  std::ranges::transform(s1d, std::back_inserter(flat),
                         [](const char* p) { return std::string_view{p ? p : ""}; });
  return make_timeindex(std::span<const std::string_view>(flat.data(), flat.size()), require_sorted);
}

inline pluss::codeh::TimeIndex make_timeindex(
    std::initializer_list<detail::NestedTimelineLiteral> nested,
    bool require_sorted = true) {
  auto lit = detail::flatten_timeline_literal(nested, 1, "make_timeindex");
  std::vector<std::string_view> views(lit.flat.size());
  std::ranges::transform(lit.flat, views.begin(),
                         [](const std::string& s) { return std::string_view{s}; });
  const auto t = ::pluss::codeh::ops_internal::timeline_from_utf8(
      std::span<const std::string_view>(views.data(), views.size()),
      std::span<const std::int64_t>(lit.sizes.data(), lit.sizes.size()));
  return make_timeindex(t, require_sorted);
}

inline pluss::codeh::TimeIndex make_timeindex(std::span<const std::int64_t> epoch_nanos,
                                              bool require_sorted = true) {
  const std::array<std::int64_t, 1> size{static_cast<std::int64_t>(epoch_nanos.size())};
  const auto t = ::pluss::codeh::ops_internal::timeline_from_epoch_nanos(epoch_nanos, size);
  return make_timeindex(t, require_sorted);
}

inline pluss::codeh::TimeIndex make_timeindex(const std::vector<std::int64_t>& epoch_nanos,
                                              bool require_sorted = true) {
  return make_timeindex(std::span<const std::int64_t>(epoch_nanos.data(), epoch_nanos.size()),
                        require_sorted);
}

} // namespace pluss
