#pragma once

// Minimal stub for exception/assert utilities expected from upstream
#include <stdexcept>

namespace MSharp {
struct Error : public std::runtime_error {
  using std::runtime_error::runtime_error;
};
} // namespace MSharp

// Minimal no-op assert macro used in device validation; these are debug-only
// Define PLUSS assertion/check macros and keep TORCH_* as compatibility
#ifndef PLUSS_INTERNAL_ASSERT_DEBUG_ONLY
#define PLUSS_INTERNAL_ASSERT_DEBUG_ONLY(cond, ...) ((void)0)
#endif

#ifndef PLUSS_INTERNAL_ASSERT
#include <cassert>
#define PLUSS_INTERNAL_ASSERT(cond, ...) assert(cond)
#endif

#ifndef PLUSS_CHECK
#define PLUSS_CHECK(cond, ...) \
  do { \
    if (!(cond)) { \
      throw MSharp::Error("PLUSS_CHECK failed"); \
    } \
  } while (0)
#endif

// Backwards-compatibility aliases (temporary)
#ifndef TORCH_INTERNAL_ASSERT_DEBUG_ONLY
#define TORCH_INTERNAL_ASSERT_DEBUG_ONLY PLUSS_INTERNAL_ASSERT_DEBUG_ONLY
#endif
#ifndef TORCH_INTERNAL_ASSERT
#define TORCH_INTERNAL_ASSERT PLUSS_INTERNAL_ASSERT
#endif
#ifndef TORCH_CHECK
#define TORCH_CHECK PLUSS_CHECK
#endif
