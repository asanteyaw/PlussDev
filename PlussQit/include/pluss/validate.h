#pragma once

#include "pluss/error.h"

#include <cctype>
#include <initializer_list>
#include <source_location>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace pluss::validate {

struct Warn {
  bool condition = true;
  std::string_view message = "validation warning";
};

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

// A condition with a suggestion: when it fails, the message gains "Did you mean 'x'?" if `given` is close
// to one of `options` (any range of strings, or a map: use pluss::exception::map_keys(map)).
//   pluss::validate::check(columns.contains(name), "unknown column '" + name + "'", name, columns);
template <typename Options>
inline void check(
    bool condition,
    std::string_view message,
    std::string_view given,
    const Options& options,
    const std::source_location& loc = std::source_location::current()) {
  if (condition) return;
  ::pluss::codem::throw_error(detail::normalized_message(message, "validation check failed"), loc, nullptr,
                              ::pluss::exception::make_hint(given, options));
}

// The whole unknown-name check in one call: passes when `given` is one of `options`, otherwise fails with
//   unknown <what> 'given'. Did you mean 'x'?  Valid values: a, b, c
// so the caller writes no message at all.
//   pluss::validate::one_of(palette, palettes, "palette");
template <typename Options>
inline void one_of(
    std::string_view given,
    const Options& options,
    std::string_view what = "value",
    const std::source_location& loc = std::source_location::current()) {
  std::vector<std::string> names;
  for (const auto& option : options) {
    if (std::string_view(option) == given) return;
    names.emplace_back(option);
  }
  ::pluss::codem::throw_error(::pluss::exception::unknown_name_message(std::string(what), std::string(given), names), loc);
}

inline void warn(
    std::string_view message,
    const std::source_location& loc = std::source_location::current()) {
  ::pluss::codem::warn(detail::normalized_message(message, "validation warning"), loc);
}

inline void warn(
    const char* message,
    const std::source_location& loc = std::source_location::current()) {
  ::pluss::codem::warn(detail::normalized_message(message ? std::string_view(message) : std::string_view{},
                                                  "validation warning"),
                       loc);
}

inline void warn(
    bool condition,
    std::string_view message = "validation warning",
    const std::source_location& loc = std::source_location::current()) {
  if (condition) return;
  ::pluss::codem::warn(detail::normalized_message(message, "validation warning"), loc);
}

inline void warn(
    std::initializer_list<Warn> checks,
    const std::source_location& loc = std::source_location::current()) {
  for (const auto& check : checks) {
    if (check.condition) continue;
    ::pluss::codem::warn(detail::normalized_message(check.message, "validation warning"), loc);
  }
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
