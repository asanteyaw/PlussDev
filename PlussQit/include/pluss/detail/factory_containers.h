#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace pluss::detail {

template <class T>
class DataSpan {
 public:
  using value_type = T;
  using size_type = std::size_t;
  using iterator = const T*;

  constexpr DataSpan() noexcept = default;
  constexpr DataSpan(const T* data, size_type size) noexcept : data_(data), size_(size) {}
  constexpr DataSpan(std::span<const T> span) noexcept : data_(span.data()), size_(span.size()) {}

  template <std::size_t N>
  constexpr DataSpan(const std::array<T, N>& data) noexcept : data_(data.data()), size_(N) {}

  template <std::size_t N>
  constexpr DataSpan(const T (&data)[N]) noexcept : data_(data), size_(N) {}

  template <class Alloc>
  constexpr DataSpan(const std::vector<T, Alloc>& data) noexcept : data_(data.data()), size_(data.size()) {}

  constexpr const T* data() const noexcept { return data_; }
  constexpr size_type size() const noexcept { return size_; }
  constexpr bool empty() const noexcept { return size_ == 0; }
  constexpr const T& operator[](size_type i) const noexcept { return data_[i]; }
  constexpr iterator begin() const noexcept { return data_; }
  constexpr iterator end() const noexcept { return data_ + size_; }
  constexpr std::span<const T> span() const noexcept { return std::span<const T>(data_, size_); }

 private:
  const T* data_{nullptr};
  size_type size_{0};
};

class ShapeSpec {
 public:
  ShapeSpec() = default;
  ShapeSpec(std::span<const std::int64_t> dims) : dims_(dims.begin(), dims.end()) {}
  ShapeSpec(DataSpan<std::int64_t> dims) : ShapeSpec(dims.span()) {}
  ShapeSpec(std::initializer_list<std::int64_t> dims) : dims_(dims) {}
  ShapeSpec(const std::vector<std::int64_t>& dims) : dims_(dims) {}
  ShapeSpec(std::vector<std::int64_t>&& dims) : dims_(std::move(dims)) {}

  std::span<const std::int64_t> span() const noexcept {
    return std::span<const std::int64_t>(dims_.data(), dims_.size());
  }

  const std::vector<std::int64_t>& vec() const noexcept { return dims_; }
  std::size_t rank() const noexcept { return dims_.size(); }
  bool empty() const noexcept { return dims_.empty(); }

  std::int64_t numel() const noexcept {
    std::int64_t count = 1;
    for (const auto dim : dims_) {
      count *= dim;
    }
    return count;
  }

 private:
  std::vector<std::int64_t> dims_{};
};

class TextSpan {
 public:
  TextSpan() = default;
  TextSpan(std::span<const std::string_view> views) : views_(views.begin(), views.end()) {}
  TextSpan(DataSpan<std::string_view> views) : TextSpan(views.span()) {}
  TextSpan(const std::vector<std::string_view>& views) : views_(views) {}
  TextSpan(std::vector<std::string_view>&& views) : views_(std::move(views)) {}
  TextSpan(std::initializer_list<const char*> views) {
    views_.reserve(views.size());
    for (const char* view : views) {
      views_.emplace_back(view ? view : "");
    }
  }

  static TextSpan from_strings(std::span<const std::string> strings) {
    TextSpan text;
    text.views_.reserve(strings.size());
    for (const auto& value : strings) {
      text.views_.emplace_back(value);
    }
    return text;
  }

  static TextSpan from_strings(const std::vector<std::string>& strings) {
    return from_strings(std::span<const std::string>(strings.data(), strings.size()));
  }

  std::span<const std::string_view> span() const noexcept {
    return std::span<const std::string_view>(views_.data(), views_.size());
  }

  const std::vector<std::string_view>& vec() const noexcept { return views_; }
  std::size_t size() const noexcept { return views_.size(); }
  bool empty() const noexcept { return views_.empty(); }

 private:
  std::vector<std::string_view> views_{};
};

} // namespace pluss::detail
