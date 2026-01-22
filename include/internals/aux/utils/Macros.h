#pragma once

#include <sstream>
#include <stdexcept>
#include <iostream>

// === Basic Validation Macro (2-arg) ===
#define PLUSS_VALIDATE(cond, msg)                                                        \
    if (!(cond)) {                                                                       \
        std::ostringstream oss;                                                          \
        oss << __FILE__ << ":" << __LINE__ << ": " << msg;                               \
        throw std::runtime_error(oss.str());                                             \
    }

// === Variadic Validation Macro ===
#define PLUSS_INSPECT(cond, ...)                                                       \
    if (!(cond)) {                                                                       \
        std::ostringstream oss;                                                          \
        oss << __FILE__ << ":" << __LINE__ << ": ";                                      \
        pluss::detail::oss_args(oss, __VA_ARGS__);                                       \
        throw std::runtime_error(oss.str());                                             \
    }

// === Assertion Macro ===
#define PLUSS_ASSERT(cond)                                                               \
    if (!(cond)) {                                                                       \
        throw std::logic_error("Assertion failed: " #cond);                             \
    }

// === Fail Immediately ===
#define PLUSS_FAIL(msg)                                                                  \
    throw std::runtime_error(std::string("Failure: ") + msg);

// === Equality Checker ===
#define PLUSS_CHECK_EQ(a, b)                                                             \
    if (!((a) == (b))) {                                                                 \
        std::ostringstream oss;                                                          \
        oss << __FILE__ << ":" << __LINE__ << ": Expected equality but got ("           \
            << #a << " = " << (a) << ", " << #b << " = " << (b) << ")";             \
        throw std::runtime_error(oss.str());                                             \
    }

// === Warning (non-throwing) ===
#define PLUSS_WARN(...)                                                                  \
    do {                                                                                 \
        std::ostringstream oss;                                                          \
        pluss::detail::oss_args(oss, __VA_ARGS__);                                       \
        std::cerr << "[Pluss Warning] " << oss.str() << std::endl;                       \
    } while (0)

// === Method Dispatch Macro ===
// Takes an object, a method, and arguments, then calls the method on the object with the arguments
// Returns whatever the method returns (if not void)
#define PLUSS_DISPATCH(obj, method, ...) \
    (obj).method(__VA_ARGS__)

    
#define PLUSS_TRANSMIT(obj, method, ...) \
    ([&]() -> decltype(auto) { return (obj).method(__VA_ARGS__); })()

namespace pluss {
namespace detail {

// --- Helper: Base case ---
inline void oss_args(std::ostringstream&) {}

// --- Helper: Recursive printing ---
template <typename First, typename... Rest>
inline void oss_args(std::ostringstream& oss, First&& first, Rest&&... rest) {
    oss << std::forward<First>(first);
    if constexpr (sizeof...(rest) > 0) {
        oss << " ";
        oss_args(oss, std::forward<Rest>(rest)...);
    }
}

} // namespace detail
} // namespace pluss
