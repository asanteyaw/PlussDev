#pragma once

#include <cstdio>
#include <sstream>
#include <source_location>
#include <stdexcept>
#include <string>
#include <utility>

#if PLUSSQIT_HAS_FMT
  #include <fmt/format.h>
#endif

#include "CodeM/core/StackTrace.h"

namespace pluss::codem {

struct Error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

namespace detail {

inline void oss_args(std::ostringstream&) {}

template <typename First, typename... Rest>
inline void oss_args(std::ostringstream& oss, First&& first, Rest&&... rest) {
  oss << std::forward<First>(first);
  if constexpr (sizeof...(rest) > 0) {
    oss << " ";
    oss_args(oss, std::forward<Rest>(rest)...);
  }
}

} // namespace detail

[[noreturn]] inline void throw_error(
    const std::string& msg,
    const std::source_location& loc = std::source_location::current()) {
  std::string full;
  full.reserve(msg.size() + 512);
  full += msg;

  full += "\nTriggered at: ";
  full += loc.file_name();
  full += ":";
  full += std::to_string(loc.line());
  full += " (";
  full += loc.function_name();
  full += ")";

  const auto st = StackTrace::format();
  if (!st.empty()) {
    full += "\nStack trace (most recent call first):\n";
    full += st;
  }

  throw Error(full);
}

inline void warn(
    const std::string& msg,
    const std::source_location& loc = std::source_location::current()) {
  std::string full;
  full.reserve(msg.size() + 512);
  full += msg;

  full += "\nTriggered at: ";
  full += loc.file_name();
  full += ":";
  full += std::to_string(loc.line());
  full += " (";
  full += loc.function_name();
  full += ")";

  const auto st = StackTrace::format();
  if (!st.empty()) {
    full += "\nStack trace (most recent call first):\n";
    full += st;
  }

  std::fwrite(full.data(), 1, full.size(), stderr);
  std::fwrite("\n", 1, 1, stderr);
}

#if PLUSSQIT_HAS_FMT
template <class... Args>
[[noreturn]] inline void throwf(fmt::format_string<Args...> fmtstr, Args&&... args) {
  throw_error(fmt::format(fmtstr, std::forward<Args>(args)...));
}
#else
template <class... Args>
[[noreturn]] inline void throwf(Args&&...) {
  throw_error("CodeM error");
}
#endif

// Torch-like check macro for internal invariants.
#define PLUSS_CHECK(cond, msg) \
  do {                         \
    if (!(cond)) {             \
      ::pluss::codem::throw_error((msg)); \
    }                          \
  } while (false)

// Fail immediately with an error message.
#define PLUSS_FAIL(msg)        \
  do {                         \
    ::pluss::codem::throw_error((msg)); \
  } while (false)

// Warn to stderr with call-site context and stack trace when available.
#define PLUSS_WARN(msg)        \
  do {                         \
    ::pluss::codem::warn((msg)); \
  } while (false)

// Equality check with single evaluation of both operands.
#define PLUSS_CHECK_EQ(a, b)   \
  do {                         \
    const auto& _pluss_lhs = (a); \
    const auto& _pluss_rhs = (b); \
    if (!(_pluss_lhs == _pluss_rhs)) { \
      std::ostringstream _pluss_oss; \
      _pluss_oss << "Expected equality but got (" \
                 << #a << " = " << _pluss_lhs << ", " \
                 << #b << " = " << _pluss_rhs << ")"; \
      ::pluss::codem::throw_error(_pluss_oss.str()); \
    } \
  } while (false)

// Variadic condition check that streams arbitrary context pieces.
#define PLUSS_INSPECT(cond, ...) \
  do { \
    if (!(cond)) { \
      std::ostringstream _pluss_oss; \
      ::pluss::codem::detail::oss_args(_pluss_oss __VA_OPT__(,) __VA_ARGS__); \
      ::pluss::codem::throw_error(_pluss_oss.str()); \
    } \
  } while (false)

} // namespace pluss::codem

#define PLUSS_CALL_GUARD() ::pluss::codem::CallGuard PLUSS__call_guard_{}
