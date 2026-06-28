#pragma once

#include "pluss/error.h"

#include <cctype>
#include <source_location>
#include <sstream>
#include <string>
#include <string_view>

namespace pluss::validate {

namespace detail {

inline bool has_visible_text(std::string_view s) {
  for (const unsigned char c : s) {
    if (!std::isspace(c)) return true;
  }
  return false;
}

inline std::string normalized_message(std::string_view msg, std::string_view fallback) {
  if (has_visible_text(msg)) return std::string(msg);
  return std::string(fallback);
}

} // namespace detail

inline void check(
    bool condition,
    std::string_view message = "validation check failed",
    const std::source_location& loc = std::source_location::current()) {
  if (condition) return;
  ::pluss::codem::throw_error(detail::normalized_message(message, "validation check failed"), loc);
}

[[noreturn]] inline void fail(
    std::string_view message = "validation failed",
    const std::source_location& loc = std::source_location::current()) {
  ::pluss::codem::throw_error(detail::normalized_message(message, "validation failed"), loc);
}

inline void warn(
    std::string_view message,
    const std::source_location& loc = std::source_location::current()) {
  ::pluss::codem::warn(detail::normalized_message(message, "validation warning"), loc);
}

inline void inspect(
    bool condition,
    std::string_view message = "validation inspect failed",
    const std::source_location& loc = std::source_location::current()) {
  if (condition) return;
  ::pluss::codem::throw_error(detail::normalized_message(message, "validation inspect failed"), loc);
}

template <typename Lhs, typename Rhs>
inline void check_eq(
    const Lhs& lhs,
    const Rhs& rhs,
    std::string_view lhs_name = "lhs",
    std::string_view rhs_name = "rhs",
    const std::source_location& loc = std::source_location::current()) {
  if (lhs == rhs) return;
  std::ostringstream oss;
  oss << "Expected equality but got ("
      << lhs_name << " = " << lhs << ", "
      << rhs_name << " = " << rhs << ")";
  ::pluss::codem::throw_error(oss.str(), loc);
}

} // namespace pluss::validate
