#pragma once

#include "MSharp/RangeView.h"
#include "pluss/error.h"

#include <array>
#include <cstdint>
#include <concepts>
#include <initializer_list>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace pluss::codem {
class Series;
} // namespace pluss::codem

namespace pluss::coden {
class Note;
} // namespace pluss::coden

namespace pluss::codeh {
class Timeline;
class Duration;
} // namespace pluss::codeh

namespace pluss::msharp::detail::lister {

template <class Range, class T>
concept RangeOf =
    std::ranges::input_range<Range> &&
    std::constructible_from<T, std::ranges::range_reference_t<Range>>;

template <class Range>
concept StringViewRange =
    std::ranges::input_range<Range> &&
    std::convertible_to<std::remove_cvref_t<std::ranges::range_reference_t<Range>>, std::string_view>;

template <class Range, class T>
inline std::vector<T> collect(Range&& values) {
  std::vector<T> out;
  if constexpr (requires { values.size(); }) {
    out.reserve(static_cast<std::size_t>(values.size()));
  }
  for (auto&& value : values) {
    out.emplace_back(std::forward<decltype(value)>(value));
  }
  return out;
}

inline std::vector<std::string> collect_strings(std::span<const std::string> values) {
  return std::vector<std::string>(values.begin(), values.end());
}

template <StringViewRange Range>
inline std::vector<std::string> collect_strings(Range&& values) {
  std::vector<std::string> out;
  if constexpr (requires { values.size(); }) {
    out.reserve(static_cast<std::size_t>(values.size()));
  }
  for (auto&& value : values) {
    out.emplace_back(std::string_view(std::forward<decltype(value)>(value)));
  }
  return out;
}

inline void check_index(std::int64_t i, std::size_t size, std::string_view where) {
  PLUSS_CHECK(
      i >= 0,
      std::string(where) + ": index out of range: index=" + std::to_string(i) +
          ", size=" + std::to_string(size));
  const auto idx = static_cast<std::size_t>(i);
  PLUSS_CHECK(
      idx < size,
      std::string(where) + ": index out of range: index=" + std::to_string(i) +
          ", size=" + std::to_string(size));
}

} // namespace pluss::msharp::detail::lister

namespace pluss::msharp {

template <class T>
class BasicLister {
public:
  using value_type = T;
  using storage_type = std::vector<T>;
  using iterator = typename storage_type::iterator;
  using const_iterator = typename storage_type::const_iterator;

  BasicLister() = default;

  BasicLister(std::initializer_list<T> values) : values_(values) {}

  BasicLister(std::span<const T> values) : values_(values.begin(), values.end()) {}

  BasicLister(pluss::msharp::RangeView::Of<T> values)
      : values_(values.begin(), values.end()) {}

  BasicLister(std::vector<T> values) : values_(std::move(values)) {}

  template <std::size_t N>
  BasicLister(const std::array<T, N>& values) : values_(values.begin(), values.end()) {}

  template <detail::lister::RangeOf<T> Range>
    requires(!std::same_as<std::remove_cvref_t<Range>, BasicLister<T>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::vector<T>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::span<const T>> &&
             !std::same_as<std::remove_cvref_t<Range>, pluss::msharp::RangeView::Of<T>>)
  BasicLister(Range&& values)
      : values_(detail::lister::collect<Range, T>(std::forward<Range>(values))) {}

  template <class... Items>
    requires(sizeof...(Items) > 0 && (std::constructible_from<T, Items> && ...))
  BasicLister(Items&&... items) : values_{T(std::forward<Items>(items))...} {}

  [[nodiscard]] bool empty() const noexcept { return values_.empty(); }
  [[nodiscard]] std::size_t size() const noexcept { return values_.size(); }

  [[nodiscard]] const T& get(std::int64_t i) const {
    detail::lister::check_index(i, values_.size(), "pluss::msharp::Lister::get");
    return values_[static_cast<std::size_t>(i)];
  }

  [[nodiscard]] T& get(std::int64_t i) {
    detail::lister::check_index(i, values_.size(), "pluss::msharp::Lister::get");
    return values_[static_cast<std::size_t>(i)];
  }

  [[nodiscard]] const T& operator[](std::int64_t i) const { return get(i); }
  [[nodiscard]] T& operator[](std::int64_t i) { return get(i); }

  [[nodiscard]] BasicLister extend(const T& value) const {
    BasicLister out = *this;
    out.values_.push_back(value);
    return out;
  }

  [[nodiscard]] BasicLister extend(std::span<const T> values) const {
    BasicLister out = *this;
    out.values_.insert(out.values_.end(), values.begin(), values.end());
    return out;
  }

  [[nodiscard]] BasicLister extend(pluss::msharp::RangeView::Of<T> values) const {
    BasicLister out = *this;
    out.values_.reserve(out.values_.size() + static_cast<std::size_t>(values.size()));
    for (const auto& value : values) {
      out.values_.push_back(value);
    }
    return out;
  }

  [[nodiscard]] BasicLister extend(std::initializer_list<T> values) const {
    BasicLister out = *this;
    out.values_.reserve(out.values_.size() + values.size());
    for (const auto& value : values) {
      out.values_.push_back(value);
    }
    return out;
  }

  template <detail::lister::RangeOf<T> Range>
    requires(!std::same_as<std::remove_cvref_t<Range>, std::vector<T>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::span<const T>> &&
             !std::same_as<std::remove_cvref_t<Range>, pluss::msharp::RangeView::Of<T>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::initializer_list<T>>)
  [[nodiscard]] BasicLister extend(Range&& values) const {
    BasicLister out = *this;
    if constexpr (requires { values.size(); }) {
      out.values_.reserve(out.values_.size() + static_cast<std::size_t>(values.size()));
    }
    for (auto&& value : values) {
      out.values_.emplace_back(std::forward<decltype(value)>(value));
    }
    return out;
  }

  [[nodiscard]] pluss::msharp::RangeView::Of<T> data() const noexcept {
    return pluss::msharp::RangeView::Of<T>(values_);
  }

  [[nodiscard]] std::span<const T> span() const noexcept {
    return std::span<const T>(values_.data(), values_.size());
  }

  [[nodiscard]] const std::vector<T>& storage() const noexcept { return values_; }
  [[nodiscard]] std::vector<T>& storage() noexcept { return values_; }

  [[nodiscard]] iterator begin() noexcept { return values_.begin(); }
  [[nodiscard]] iterator end() noexcept { return values_.end(); }
  [[nodiscard]] const_iterator begin() const noexcept { return values_.begin(); }
  [[nodiscard]] const_iterator end() const noexcept { return values_.end(); }

private:
  storage_type values_{};
};

template <>
class BasicLister<std::string_view> {
public:
  using value_type = std::string_view;
  using storage_type = std::vector<std::string_view>;
  using iterator = typename storage_type::iterator;
  using const_iterator = typename storage_type::const_iterator;

  BasicLister() = default;

  BasicLister(std::initializer_list<std::string_view> values)
      : owned_(detail::lister::collect_strings(values)) {
    rebuild_views_();
  }

  BasicLister(std::initializer_list<const char*> values) {
    owned_.reserve(values.size());
    for (const char* value : values) {
      owned_.emplace_back(value ? value : "");
    }
    rebuild_views_();
  }

  BasicLister(std::span<const std::string_view> values)
      : owned_(detail::lister::collect_strings(values)) {
    rebuild_views_();
  }

  BasicLister(std::span<const std::string> values)
      : owned_(detail::lister::collect_strings(values)) {
    rebuild_views_();
  }

  BasicLister(std::vector<std::string> values)
      : owned_(std::move(values)) {
    rebuild_views_();
  }

  BasicLister(std::vector<std::string_view> values)
      : owned_(detail::lister::collect_strings(values)) {
    rebuild_views_();
  }

  BasicLister(pluss::msharp::RangeView::Of<std::string_view> values)
      : owned_(detail::lister::collect_strings(values)) {
    rebuild_views_();
  }

  template <detail::lister::StringViewRange Range>
    requires(!std::same_as<std::remove_cvref_t<Range>, BasicLister<std::string_view>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::vector<std::string>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::vector<std::string_view>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::span<const std::string_view>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::span<std::string_view>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::span<const std::string>> &&
             !std::same_as<std::remove_cvref_t<Range>, pluss::msharp::RangeView::Of<std::string_view>>)
  BasicLister(Range&& values)
      : owned_(detail::lister::collect_strings(std::forward<Range>(values))) {
    rebuild_views_();
  }

  [[nodiscard]] bool empty() const noexcept { return owned_.empty(); }
  [[nodiscard]] std::size_t size() const noexcept { return owned_.size(); }

  [[nodiscard]] std::string_view get(std::int64_t i) const {
    detail::lister::check_index(i, views_.size(), "pluss::msharp::Lister::get");
    return views_[static_cast<std::size_t>(i)];
  }

  [[nodiscard]] std::string_view operator[](std::int64_t i) const { return get(i); }

  [[nodiscard]] BasicLister extend(std::string_view value) const {
    BasicLister out = *this;
    out.owned_.emplace_back(value);
    out.rebuild_views_();
    return out;
  }

  [[nodiscard]] BasicLister extend(std::span<const std::string_view> values) const {
    BasicLister out = *this;
    out.owned_.reserve(out.owned_.size() + values.size());
    for (const auto value : values) {
      out.owned_.emplace_back(value);
    }
    out.rebuild_views_();
    return out;
  }

  [[nodiscard]] BasicLister extend(std::initializer_list<std::string_view> values) const {
    return extend(std::span<const std::string_view>(values.begin(), values.size()));
  }

  template <detail::lister::StringViewRange Range>
    requires(!std::same_as<std::remove_cvref_t<Range>, std::vector<std::string>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::vector<std::string_view>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::span<const std::string_view>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::span<std::string_view>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::initializer_list<std::string_view>>)
  [[nodiscard]] BasicLister extend(Range&& values) const {
    BasicLister out = *this;
    if constexpr (requires { values.size(); }) {
      out.owned_.reserve(out.owned_.size() + static_cast<std::size_t>(values.size()));
    }
    for (auto&& value : values) {
      out.owned_.emplace_back(std::string_view(std::forward<decltype(value)>(value)));
    }
    out.rebuild_views_();
    return out;
  }

  [[nodiscard]] pluss::msharp::RangeView::Of<std::string_view> data() const noexcept {
    return pluss::msharp::RangeView::Of<std::string_view>(views_);
  }

  [[nodiscard]] std::span<const std::string_view> span() const noexcept {
    return std::span<const std::string_view>(views_.data(), views_.size());
  }

  [[nodiscard]] const std::vector<std::string>& owned() const noexcept { return owned_; }

  [[nodiscard]] iterator begin() noexcept { return views_.begin(); }
  [[nodiscard]] iterator end() noexcept { return views_.end(); }
  [[nodiscard]] const_iterator begin() const noexcept { return views_.begin(); }
  [[nodiscard]] const_iterator end() const noexcept { return views_.end(); }

private:
  void rebuild_views_() {
    views_.clear();
    views_.reserve(owned_.size());
    for (const auto& value : owned_) {
      views_.emplace_back(value);
    }
  }

  std::vector<std::string> owned_{};
  storage_type views_{};
};

template <>
class BasicLister<std::string> {
public:
  using value_type = std::string;
  using storage_type = std::vector<std::string>;
  using iterator = typename storage_type::iterator;
  using const_iterator = typename storage_type::const_iterator;

  BasicLister() = default;

  BasicLister(std::initializer_list<std::string> values) : values_(values) {}

  BasicLister(std::initializer_list<const char*> values) {
    values_.reserve(values.size());
    for (const char* value : values) {
      values_.emplace_back(value ? value : "");
    }
  }

  BasicLister(std::span<const std::string> values)
      : values_(values.begin(), values.end()) {}

  BasicLister(std::vector<std::string> values)
      : values_(std::move(values)) {}

  template <detail::lister::StringViewRange Range>
    requires(!std::same_as<std::remove_cvref_t<Range>, BasicLister<std::string>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::vector<std::string>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::span<const std::string>>)
  BasicLister(Range&& values)
      : values_(detail::lister::collect_strings(std::forward<Range>(values))) {}

  [[nodiscard]] bool empty() const noexcept { return values_.empty(); }
  [[nodiscard]] std::size_t size() const noexcept { return values_.size(); }

  [[nodiscard]] const std::string& get(std::int64_t i) const {
    detail::lister::check_index(i, values_.size(), "pluss::msharp::Lister::get");
    return values_[static_cast<std::size_t>(i)];
  }

  [[nodiscard]] std::string& get(std::int64_t i) {
    detail::lister::check_index(i, values_.size(), "pluss::msharp::Lister::get");
    return values_[static_cast<std::size_t>(i)];
  }

  [[nodiscard]] const std::string& operator[](std::int64_t i) const { return get(i); }
  [[nodiscard]] std::string& operator[](std::int64_t i) { return get(i); }

  [[nodiscard]] BasicLister extend(std::string_view value) const {
    BasicLister out = *this;
    out.values_.emplace_back(value);
    return out;
  }

  [[nodiscard]] BasicLister extend(std::span<const std::string> values) const {
    BasicLister out = *this;
    out.values_.insert(out.values_.end(), values.begin(), values.end());
    return out;
  }

  [[nodiscard]] BasicLister extend(std::initializer_list<std::string> values) const {
    return extend(std::span<const std::string>(values.begin(), values.size()));
  }

  template <detail::lister::StringViewRange Range>
    requires(!std::same_as<std::remove_cvref_t<Range>, std::vector<std::string>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::span<const std::string>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::initializer_list<std::string>>)
  [[nodiscard]] BasicLister extend(Range&& values) const {
    BasicLister out = *this;
    if constexpr (requires { values.size(); }) {
      out.values_.reserve(out.values_.size() + static_cast<std::size_t>(values.size()));
    }
    for (auto&& value : values) {
      out.values_.emplace_back(std::string_view(std::forward<decltype(value)>(value)));
    }
    return out;
  }

  [[nodiscard]] pluss::msharp::RangeView::Of<std::string> data() const noexcept {
    return pluss::msharp::RangeView::Of<std::string>(values_);
  }

  [[nodiscard]] pluss::msharp::RangeView::Of<std::string_view> text_view() const {
    views_.clear();
    views_.reserve(values_.size());
    for (const auto& value : values_) {
      views_.emplace_back(value);
    }
    return pluss::msharp::RangeView::Of<std::string_view>(views_);
  }

  [[nodiscard]] const std::vector<std::string>& storage() const noexcept { return values_; }

  [[nodiscard]] iterator begin() noexcept { return values_.begin(); }
  [[nodiscard]] iterator end() noexcept { return values_.end(); }
  [[nodiscard]] const_iterator begin() const noexcept { return values_.begin(); }
  [[nodiscard]] const_iterator end() const noexcept { return values_.end(); }

private:
  storage_type values_{};
  mutable std::vector<std::string_view> views_{};
};

namespace Lister {

template <class T>
using Of = BasicLister<T>;

using Int = Of<std::int64_t>;
using Bool = Of<bool>;
using Double = Of<double>;
using StringView = Of<std::string_view>;
using String = Of<std::string>;
using SymInt = Of<pluss::msharp::SymInt>;
using SymBool = Of<pluss::msharp::SymBool>;
using Series = Of<pluss::codem::Series>;
using Note = Of<pluss::coden::Note>;
using Timeline = Of<pluss::codeh::Timeline>;
using Duration = Of<pluss::codeh::Duration>;

} // namespace Lister

} // namespace pluss::msharp
