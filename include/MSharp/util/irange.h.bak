// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <MSharp/util/TypeSafeSignMath.h>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace MSharp {

namespace detail {

template <
    typename I,
    bool one_sided = false,
    std::enable_if_t<std::is_integral_v<I>, int> = 0>
struct integer_iterator {
  using iterator_category = std::input_iterator_tag;
  using value_type = I;
  using difference_type = std::ptrdiff_t;
  using pointer = I*;
  using reference = I&;

  explicit constexpr integer_iterator(I value) : value(value) {}

  constexpr I operator*() const {
    return value;
  }

  constexpr I const* operator->() const {
    return &value;
  }

  constexpr integer_iterator& operator++() {
    ++value;
    return *this;
  }

  constexpr integer_iterator operator++(int) {
    const auto copy = *this;
    ++*this;
    return copy;
  }

  constexpr bool operator==(const integer_iterator& other) const {
    if constexpr (one_sided) {
      return is_negative(other.value) || value == other.value;
    } else {
      return value == other.value;
    }
    return false;
  }

  constexpr bool operator!=(const integer_iterator& other) const {
    return !(*this == other);
  }

 protected:
  I value;
};

} // namespace detail

template <
    typename I,
    bool one_sided = false,
    std::enable_if_t<std::is_integral_v<I>, bool> = true>
struct integer_range {
 public:
  constexpr integer_range(I begin, I end) : begin_(begin), end_(end) {}
  using iterator = detail::integer_iterator<I, one_sided>;
  constexpr iterator begin() const {
    return begin_;
  }
  constexpr iterator end() const {
    return end_;
  }

 private:
  iterator begin_;
  iterator end_;
};

template <
    typename Integer1,
    typename Integer2,
    std::enable_if_t<std::is_integral_v<Integer1>, bool> = true,
    std::enable_if_t<std::is_integral_v<Integer2>, bool> = true>
constexpr integer_range<Integer2> irange(Integer1 begin, Integer2 end) {
  return {
      static_cast<Integer2>(begin),
      std::max(static_cast<Integer2>(begin), end)};
}

template <
    typename Integer,
    std::enable_if_t<std::is_integral_v<Integer>, bool> = true>
constexpr integer_range<Integer, true> irange(Integer end) {
  return {Integer(), end};
}

} // namespace MSharp
