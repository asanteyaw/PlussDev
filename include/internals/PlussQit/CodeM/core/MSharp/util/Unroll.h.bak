#pragma once
#include <MSharp/macros/Macros.h>
#include <type_traits>

// Utility to guarantee complete unrolling of a loop where the bounds are known
// at compile time. Various pragmas achieve similar effects, but are not as
// portable across compilers.

// Example: MSharp::ForcedUnroll<4>{}(f); is equivalent to f(0); f(1); f(2); f(3);

namespace MSharp {

template <int n>
struct ForcedUnroll {
  template <typename Func, typename... Args>
  C10_ALWAYS_INLINE void operator()(const Func& f, Args... args) const {
    ForcedUnroll<n - 1>{}(f, args...);
    f(std::integral_constant<int, n - 1>{}, args...);
  }
};

template <>
struct ForcedUnroll<1> {
  template <typename Func, typename... Args>
  C10_ALWAYS_INLINE void operator()(const Func& f, Args... args) const {
    f(std::integral_constant<int, 0>{}, args...);
  }
};

} // namespace MSharp
