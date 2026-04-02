#pragma once

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace pluss::codem {
class Series;
class SymInt;
} // namespace pluss::codem

namespace pluss::coden {
class Note;
} // namespace pluss::coden

namespace pluss::codeh {
class Timeline;
} // namespace pluss::codeh

namespace pluss::msharp {

template <class T>
class BasicRangeView {
public:
  using value_type = T;
  using size_type = std::size_t;
  using pointer = const T*;
  using const_pointer = const T*;
  using reference = const T&;
  using const_reference = const T&;
  using iterator = const T*;
  using const_iterator = const T*;
  using reverse_iterator = std::reverse_iterator<const_iterator>;
  using const_reverse_iterator = reverse_iterator;

  constexpr BasicRangeView() noexcept = default;
  constexpr BasicRangeView(const T* data, std::size_t size) noexcept
      : data_(data), size_(size) {}
  constexpr BasicRangeView(std::span<const T> view) noexcept
      : data_(view.data()), size_(view.size()) {}

  template <class Alloc>
  constexpr BasicRangeView(const std::vector<T, Alloc>& values) noexcept
      : data_(values.data()), size_(values.size()) {}

  constexpr BasicRangeView(std::initializer_list<T> values) noexcept
      : data_(values.begin()), size_(values.size()) {}

  template <class It>
    requires(std::contiguous_iterator<It> &&
             std::is_convertible_v<std::remove_cv_t<std::iter_value_t<It>>*, const T*>)
  constexpr BasicRangeView(It first, std::size_t size) noexcept
      : data_(size == 0 ? nullptr : std::to_address(first)), size_(size) {}

  template <std::size_t N>
  constexpr BasicRangeView(const T (&array)[N]) noexcept
      : data_(array), size_(N) {}

  [[nodiscard]] constexpr const T* data() const noexcept { return data_; }
  [[nodiscard]] constexpr std::size_t size() const noexcept { return size_; }
  [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }

  [[nodiscard]] constexpr const T& operator[](std::size_t i) const noexcept {
    return data_[i];
  }
  [[nodiscard]] constexpr const T& front() const noexcept { return data_[0]; }
  [[nodiscard]] constexpr const T& back() const noexcept { return data_[size_ - 1]; }
  [[nodiscard]] constexpr const T& at(std::size_t i) const {
    if (i >= size_) {
      throw std::out_of_range("msharp::BasicRangeView::at: index out of range");
    }
    return data_[i];
  }

  [[nodiscard]] constexpr const_iterator begin() const noexcept { return data_; }
  [[nodiscard]] constexpr const_iterator end() const noexcept { return data_ + size_; }
  [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
  [[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }
  [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }
  [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }
  [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return rend(); }

  [[nodiscard]] constexpr std::span<const T> span() const noexcept {
    return std::span<const T>(data_, size_);
  }
  constexpr operator std::span<const T>() const noexcept { return span(); }

  [[nodiscard]] constexpr bool equals(BasicRangeView<T> rhs) const noexcept {
    return size_ == rhs.size_ && std::equal(begin(), end(), rhs.begin());
  }

  [[nodiscard]] constexpr BasicRangeView<T> subview(
      std::size_t offset, std::size_t count = std::dynamic_extent) const noexcept {
    const auto off = std::min(offset, size_);
    const auto rem = size_ - off;
    const auto n = count == std::dynamic_extent ? rem : std::min(count, rem);
    return BasicRangeView<T>(data_ + off, n);
  }

  [[nodiscard]] constexpr BasicRangeView<T> first(std::size_t count) const noexcept {
    return subview(0, count);
  }

  [[nodiscard]] constexpr BasicRangeView<T> last(std::size_t count) const noexcept {
    const auto n = std::min(count, size_);
    return subview(size_ - n, n);
  }

  [[nodiscard]] constexpr BasicRangeView<T> slice(std::size_t offset, std::size_t count) const noexcept {
    return subview(offset, count);
  }
  [[nodiscard]] constexpr BasicRangeView<T> slice(std::size_t offset) const noexcept {
    return subview(offset, std::dynamic_extent);
  }
  [[nodiscard]] constexpr BasicRangeView<T> drop_front(std::size_t count = 1) const noexcept {
    return subview(std::min(count, size_), std::dynamic_extent);
  }
  [[nodiscard]] constexpr BasicRangeView<T> drop_back(std::size_t count = 1) const noexcept {
    const auto n = std::min(count, size_);
    return first(size_ - n);
  }
  [[nodiscard]] constexpr BasicRangeView<T> take_front(std::size_t count = 1) const noexcept {
    return first(count);
  }
  [[nodiscard]] constexpr BasicRangeView<T> take_back(std::size_t count = 1) const noexcept {
    return last(count);
  }

  template <class Pred>
  [[nodiscard]] constexpr bool all_match(Pred&& pred) const {
    return std::all_of(begin(), end(), std::forward<Pred>(pred));
  }

  [[nodiscard]] std::vector<T> vec() const {
    return std::vector<T>(begin(), end());
  }

  [[nodiscard]] constexpr bool operator==(BasicRangeView<T> rhs) const noexcept {
    return equals(rhs);
  }
  [[nodiscard]] constexpr bool operator!=(BasicRangeView<T> rhs) const noexcept {
    return !equals(rhs);
  }

private:
  const T* data_{nullptr};
  std::size_t size_{0};
};

template <class T>
using RangeViewT = BasicRangeView<T>;

namespace RangeView {

template <class T>
using Of = BasicRangeView<T>;

using Int = Of<std::int64_t>;
using Double = Of<double>;
using SymInt = Of<pluss::codem::SymInt>;
using Series = Of<pluss::codem::Series>;
using Note = Of<pluss::coden::Note>;
using Timeline = Of<pluss::codeh::Timeline>;

} // namespace RangeView


} // namespace pluss::msharp
