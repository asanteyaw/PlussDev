#pragma once

#include <array>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "pluss/codeh/codeh.h"
#include "pluss/detail/factory_containers.h"
#include "pluss/detail/nested_timeline_literal.h"

namespace pluss::codeh {

inline Timeline moment(std::string_view s) {
  const std::array<std::string_view, 1> flat{s};
  return ops_internal::timeline_from_utf8(flat, {});
}

inline Timeline moment(const char* s) {
  return moment(std::string_view{s ? s : ""});
}

inline Timeline moment(::pluss::detail::TextSpan s1d) {
  const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(s1d.size())};
  return ops_internal::timeline_from_utf8(s1d.span(), sizes);
}

inline Timeline moment(std::span<const std::string_view> s1d) {
  return moment(::pluss::detail::TextSpan(s1d));
}

inline Timeline moment(const std::vector<std::string_view>& s1d) {
  return moment(::pluss::detail::TextSpan(s1d));
}

inline Timeline moment(std::initializer_list<const char*> s1d) {
  return moment(::pluss::detail::TextSpan(s1d));
}

inline Timeline moment(std::initializer_list<::pluss::detail::NestedTimelineLiteral> nested) {
  auto lit = ::pluss::detail::flatten_timeline_literal(nested, 3, "make_timeline");
  return ops_internal::timeline_from_utf8(
      ::pluss::detail::TextSpan::from_strings(lit.flat).span(),
      std::span<const std::int64_t>(lit.sizes.data(), lit.sizes.size()));
}

} // namespace pluss::codeh
