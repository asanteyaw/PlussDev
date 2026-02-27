#pragma once

#include <array>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "pluss/coden/coden.h"
#include "pluss/detail/factory_containers.h"
#include "pluss/detail/nested_note_literal.h"

namespace pluss::coden {

inline Note content(std::string_view utf8) {
  const std::array<std::string_view, 1> flat{utf8};
  return ops_internal::note_from_utf8(flat, {});
}

inline Note content(std::u8string_view utf8) {
  return content(std::string_view{reinterpret_cast<const char*>(utf8.data()), utf8.size()});
}

inline Note content(const char* utf8) {
  return content(std::string_view{utf8 ? utf8 : ""});
}

inline Note content(char c) {
  std::string s(1, c);
  return content(std::string_view{s});
}

inline Note content(::pluss::detail::TextSpan utf8_1d) {
  const std::array<std::int64_t, 1> sizes{static_cast<std::int64_t>(utf8_1d.size())};
  return ops_internal::note_from_utf8(utf8_1d.span(), sizes);
}

inline Note content(std::span<const std::string_view> utf8_1d) {
  return content(::pluss::detail::TextSpan(utf8_1d));
}

inline Note content(const std::vector<std::string_view>& utf8_1d) {
  return content(::pluss::detail::TextSpan(utf8_1d));
}

inline Note content(std::initializer_list<const char*> utf8_1d) {
  return content(::pluss::detail::TextSpan(utf8_1d));
}

inline Note content(std::initializer_list<::pluss::detail::NestedNoteLiteral> utf8_nd) {
  auto flattened = ::pluss::detail::flatten_note_literal(utf8_nd, 3, "pluss::make_note");
  return ops_internal::note_from_utf8(
      ::pluss::detail::TextSpan::from_strings(flattened.flat).span(),
      std::span<const std::int64_t>(flattened.sizes.data(), flattened.sizes.size()));
}

inline Note content(std::string&& utf8) {
  const std::array<std::string_view, 1> flat{utf8};
  return ops_internal::note_from_utf8(flat, {});
}

inline Note content(std::u8string&& utf8) {
  return content(std::string{reinterpret_cast<const char*>(utf8.data()), utf8.size()});
}

inline Note content(std::vector<std::string>&& utf8_1d) {
  return content(::pluss::detail::TextSpan::from_strings(utf8_1d).span());
}

inline Note content(std::vector<std::string_view>&& utf8_1d) {
  return content(::pluss::detail::TextSpan(utf8_1d));
}

} // namespace pluss::coden
