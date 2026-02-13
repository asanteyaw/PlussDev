#pragma once

#include <cstdio>
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

} // namespace pluss::codem

#define PLUSS_CALL_GUARD() ::pluss::codem::CallGuard PLUSS__call_guard_{}
