#pragma once

#include "pluss/codem/codem.h"

#include <concepts>
#include <initializer_list>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

namespace pluss {

namespace detail::stack_array {

struct StackSpan {
  std::span<const Series> values;
};

template <class Range>
concept SeriesRange =
    std::ranges::input_range<Range> &&
    std::convertible_to<std::remove_cvref_t<std::ranges::range_reference_t<Range>>, Series>;

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

inline Series pile(detail::stack_array::StackSpan values, std::int64_t dim = 0) {
  return detail::stack_array::pile_span(values.values, dim);
}

inline Series unite(detail::stack_array::StackSpan values, std::int64_t dim = 0) {
  return detail::stack_array::unite_span(values.values, dim);
}

} // namespace pluss
