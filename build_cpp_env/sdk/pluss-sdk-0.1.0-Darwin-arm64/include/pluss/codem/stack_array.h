#pragma once

#include "pluss/codem/codem.h"

#include <concepts>
#include <initializer_list>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace pluss {

namespace detail::stack_array {

struct StackSpan {
  pluss::msharp::RangeView::Of<Series> values;
};

struct StringSpan {
  std::span<const std::string_view> values;
};

template <class Range>
concept SeriesRange =
    std::ranges::input_range<Range> &&
    std::convertible_to<std::remove_cvref_t<std::ranges::range_reference_t<Range>>, Series>;

template <class Range>
concept StringViewRange =
    std::ranges::input_range<Range> &&
    std::convertible_to<std::remove_cvref_t<std::ranges::range_reference_t<Range>>, std::string_view>;

template <SeriesRange Range>
inline std::vector<Series> collect_series(Range&& values) {
  std::vector<Series> out;
  if constexpr (requires { values.size(); }) {
    out.reserve(static_cast<std::size_t>(values.size()));
  }
  for (auto&& value : values) {
    out.push_back(Series(std::forward<decltype(value)>(value)));
  }
  return out;
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

inline Series pile_span(pluss::msharp::RangeView::Of<Series> values, std::int64_t dim = 0) {
  if (values.empty()) {
    return {};
  }
  if (values.size() == 1) {
    return values.front().unsqueeze(dim);
  }
  return values.front().pile(values.drop_front(), dim);
}

inline Series unite_span(pluss::msharp::RangeView::Of<Series> values, std::int64_t dim = 0, bool strict_dtype = true) {
  if (values.empty()) {
    return {};
  }
  if (values.size() == 1) {
    return values.front();
  }
  return values.front().unite(values.drop_front(), dim, strict_dtype);
}

} // namespace detail::stack_array

template <class T>
class StackArray;

template <>
class StackArray<Series> {
public:
  using value_type = Series;
  using storage_type = std::vector<Series>;
  using iterator = storage_type::iterator;
  using const_iterator = storage_type::const_iterator;

  StackArray() = default;

  StackArray(std::initializer_list<Series> values) : values_(values) {}

  StackArray(pluss::msharp::RangeView::Of<Series> values)
      : values_(values.begin(), values.end()) {}

  StackArray(std::vector<Series> values) : values_(std::move(values)) {}

  template <detail::stack_array::SeriesRange Range>
    requires(!std::same_as<std::remove_cvref_t<Range>, StackArray<Series>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::vector<Series>> &&
             !std::same_as<std::remove_cvref_t<Range>, pluss::msharp::RangeView::Of<Series>>)
  StackArray(Range&& values)
      : values_(detail::stack_array::collect_series(std::forward<Range>(values))) {}

  template <class... Items>
    requires(sizeof...(Items) > 0 && (std::constructible_from<Series, Items> && ...))
  StackArray(Items&&... items) : values_{Series(std::forward<Items>(items))...} {}

  [[nodiscard]] bool empty() const noexcept { return values_.empty(); }
  [[nodiscard]] std::size_t size() const noexcept { return values_.size(); }
  [[nodiscard]] const Series& get(i64_t i) const {
    PLUSS_CHECK(
        i >= 0,
        "pluss::sign_list::get index out of range: index=" + std::to_string(i) +
            ", size=" + std::to_string(values_.size()));
    const std::size_t idx = static_cast<std::size_t>(i);
    PLUSS_CHECK(
        idx < values_.size(),
        "pluss::sign_list::get index out of range: index=" + std::to_string(i) +
            ", size=" + std::to_string(values_.size()));
    return values_[idx];
  }
  [[nodiscard]] Series& get(i64_t i) {
    PLUSS_CHECK(
        i >= 0,
        "pluss::sign_list::get index out of range: index=" + std::to_string(i) +
            ", size=" + std::to_string(values_.size()));
    const std::size_t idx = static_cast<std::size_t>(i);
    PLUSS_CHECK(
        idx < values_.size(),
        "pluss::sign_list::get index out of range: index=" + std::to_string(i) +
            ", size=" + std::to_string(values_.size()));
    return values_[idx];
  }
  [[nodiscard]] const Series& operator[](i64_t i) const { return get(i); }
  [[nodiscard]] Series& operator[](i64_t i) { return get(i); }

  [[nodiscard]] StackArray extend(const Series& value) const {
    StackArray out = *this;
    out.values_.push_back(value);
    return out;
  }
  [[nodiscard]] StackArray extend(pluss::msharp::RangeView::Of<Series> values) const {
    StackArray out = *this;
    out.values_.reserve(out.values_.size() + values.size());
    for (const auto& value : values) {
      out.values_.push_back(value);
    }
    return out;
  }
  [[nodiscard]] StackArray extend(std::initializer_list<Series> values) const {
    return extend(pluss::msharp::RangeView::Of<Series>(values));
  }
  template <detail::stack_array::SeriesRange Range>
    requires(!std::same_as<std::remove_cvref_t<Range>, pluss::msharp::RangeView::Of<Series>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::initializer_list<Series>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::vector<Series>>)
  [[nodiscard]] StackArray extend(Range&& values) const {
    StackArray out = *this;
    if constexpr (requires { values.size(); }) {
      out.values_.reserve(out.values_.size() + static_cast<std::size_t>(values.size()));
    }
    for (auto&& value : values) {
      out.values_.push_back(Series(std::forward<decltype(value)>(value)));
    }
    return out;
  }

  [[nodiscard]] detail::stack_array::StackSpan data() const noexcept {
    return detail::stack_array::StackSpan{pluss::msharp::RangeView::Of<Series>(values_)};
  }

  [[nodiscard]] iterator begin() noexcept { return values_.begin(); }
  [[nodiscard]] iterator end() noexcept { return values_.end(); }
  [[nodiscard]] const_iterator begin() const noexcept { return values_.begin(); }
  [[nodiscard]] const_iterator end() const noexcept { return values_.end(); }

private:
  storage_type values_{};
};

template <>
class StackArray<std::string_view> {
public:
  using value_type = std::string_view;
  using storage_type = std::vector<std::string_view>;
  using iterator = storage_type::iterator;
  using const_iterator = storage_type::const_iterator;

  StackArray() = default;

  StackArray(std::initializer_list<std::string_view> values) {
    owned_.reserve(values.size());
    for (std::string_view value : values) {
      owned_.emplace_back(value);
    }
    rebuild_views_();
  }

  StackArray(std::span<const std::string_view> values)
      : owned_(detail::stack_array::collect_strings(values)) {
    rebuild_views_();
  }

  StackArray(std::vector<std::string> values)
      : owned_(std::move(values)) {
    rebuild_views_();
  }

  StackArray(std::vector<std::string_view> values)
      : owned_(detail::stack_array::collect_strings(values)) {
    rebuild_views_();
  }

  template <detail::stack_array::StringViewRange Range>
    requires(!std::same_as<std::remove_cvref_t<Range>, StackArray<std::string_view>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::vector<std::string>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::vector<std::string_view>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::span<const std::string_view>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::span<std::string_view>>)
  StackArray(Range&& values)
      : owned_(detail::stack_array::collect_strings(std::forward<Range>(values))) {
    rebuild_views_();
  }

  [[nodiscard]] bool empty() const noexcept { return owned_.empty(); }
  [[nodiscard]] std::size_t size() const noexcept { return owned_.size(); }
  [[nodiscard]] std::string_view get(i64_t i) const {
    PLUSS_CHECK(
        i >= 0,
        "pluss::StackArray<string_view>::get index out of range: index=" + std::to_string(i) +
            ", size=" + std::to_string(views_.size()));
    const std::size_t idx = static_cast<std::size_t>(i);
    PLUSS_CHECK(
        idx < views_.size(),
        "pluss::StackArray<string_view>::get index out of range: index=" + std::to_string(i) +
            ", size=" + std::to_string(views_.size()));
    return views_[idx];
  }

  [[nodiscard]] detail::stack_array::StringSpan data() const noexcept {
    return detail::stack_array::StringSpan{
        std::span<const std::string_view>(views_.data(), views_.size())};
  }

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

using sign_list = StackArray<Series>;
using SignList = sign_list;

inline Series pile(detail::stack_array::StackSpan values, std::int64_t dim = 0) {
  return detail::stack_array::pile_span(values.values, dim);
}

inline Series unite(detail::stack_array::StackSpan values, std::int64_t dim = 0, bool strict_dtype = true) {
  return detail::stack_array::unite_span(values.values, dim, strict_dtype);
}

} // namespace pluss
