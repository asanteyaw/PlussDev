#pragma once

#include <string>

namespace functors {

// Generic utility for any type T that has `size()` and `c_str()` or equivalent
template <typename T>
inline bool is_char(const T& t) {
    static_assert(std::is_invocable_r_v<size_t, decltype(&T::size), const T&>,
                  "T must have a size() method returning size_t");
    return t.size() == 1;
}

template <typename T>
inline char as_char(const T& t) {
    static_assert(std::is_invocable_r_v<size_t, decltype(&T::size), const T&>,
                  "T must have a size() method returning size_t");
    return t.c_str()[0];
}

}  // namespace functors