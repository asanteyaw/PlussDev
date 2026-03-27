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
  std::span<const Series> values;
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

inline Series pile_span(std::span<const Series> values, std::int64_t dim = 0) {
  if (values.empty()) {
    return {};
  }
  if (values.size() == 1) {
    return values.front().unsqueeze(dim);
  }
  std::vector<Series> tail(values.begin() + 1, values.end());
  return values.front().pile(tail, dim);
}

inline Series unite_span(std::span<const Series> values, std::int64_t dim = 0) {
  if (values.empty()) {
    return {};
  }
  if (values.size() == 1) {
    return values.front();
  }
  std::vector<Series> tail(values.begin() + 1, values.end());
  return values.front().unite(tail, dim);
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

  StackArray(std::span<const Series> values)
      : values_(values.begin(), values.end()) {}

  StackArray(std::vector<Series> values) : values_(std::move(values)) {}

  template <detail::stack_array::SeriesRange Range>
    requires(!std::same_as<std::remove_cvref_t<Range>, StackArray<Series>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::vector<Series>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::span<const Series>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::span<Series>>)
  StackArray(Range&& values)
      : values_(detail::stack_array::collect_series(std::forward<Range>(values))) {}

  template <class... Items>
    requires(sizeof...(Items) > 0 && (std::constructible_from<Series, Items> && ...))
  StackArray(Items&&... items) : values_{Series(std::forward<Items>(items))...} {}

  [[nodiscard]] bool empty() const noexcept { return values_.empty(); }
  [[nodiscard]] std::size_t size() const noexcept { return values_.size(); }

  [[nodiscard]] detail::stack_array::StackSpan data() const noexcept {
    return detail::stack_array::StackSpan{std::span<const Series>(values_.data(), values_.size())};
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

template <class... Items>
  requires(sizeof...(Items) > 0)
inline StackArray<Series> sign_list(Items&&... items) {
  return StackArray<Series>(std::forward<Items>(items)...);
}

inline Series pile(detail::stack_array::StackSpan values, std::int64_t dim = 0) {
  return detail::stack_array::pile_span(values.values, dim);
}

inline Series unite(detail::stack_array::StackSpan values, std::int64_t dim = 0) {
  return detail::stack_array::unite_span(values.values, dim);
}

} // namespace pluss
