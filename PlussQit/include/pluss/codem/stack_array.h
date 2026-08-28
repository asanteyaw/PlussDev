#pragma once

#include "pluss/codem/codem.h"
#include "pluss/msharp/lister.h"

#include <concepts>
#include <initializer_list>
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

struct Int64Span {
  std::span<const std::int64_t> values;
};

struct DoubleSpan {
  std::span<const double> values;
};

inline Series pile_span(pluss::msharp::RangeView::Of<Series> values, std::int64_t dim = 0) {
  if (values.empty()) {
    return {};
  }
  if (values.size() == 1) {
    return values.front().unsqueeze(dim);
  }
  return values.front().pile(values.drop_front(), dim);
}

inline Series unite_span(pluss::msharp::RangeView::Of<Series> values,
                         std::int64_t dim = 0,
                         bool strict_dtype = true) {
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
  using lister_type = pluss::msharp::Lister::Series;
  using iterator = typename lister_type::iterator;
  using const_iterator = typename lister_type::const_iterator;

  StackArray() = default;
  StackArray(std::initializer_list<Series> values) : values_(values) {}
  StackArray(pluss::msharp::RangeView::Of<Series> values) : values_(values) {}
  StackArray(std::vector<Series> values) : values_(std::move(values)) {}
  StackArray(const lister_type& values) : values_(values) {}
  StackArray(lister_type&& values) : values_(std::move(values)) {}

  template <pluss::msharp::detail::lister::RangeOf<Series> Range>
    requires(!std::same_as<std::remove_cvref_t<Range>, StackArray<Series>> &&
             !std::same_as<std::remove_cvref_t<Range>, lister_type> &&
             !std::same_as<std::remove_cvref_t<Range>, std::vector<Series>> &&
             !std::same_as<std::remove_cvref_t<Range>, pluss::msharp::RangeView::Of<Series>>)
  StackArray(Range&& values) : values_(std::forward<Range>(values)) {}

  template <class... Items>
    requires(sizeof...(Items) > 0 && (std::constructible_from<Series, Items> && ...))
  StackArray(Items&&... items) : values_(std::forward<Items>(items)...) {}

  [[nodiscard]] bool empty() const noexcept { return values_.empty(); }
  [[nodiscard]] std::size_t size() const noexcept { return values_.size(); }
  [[nodiscard]] const Series& get(i64_t i) const { return values_.get(i); }
  [[nodiscard]] Series& get(i64_t i) { return values_.get(i); }
  [[nodiscard]] const Series& operator[](i64_t i) const { return get(i); }
  [[nodiscard]] Series& operator[](i64_t i) { return get(i); }

  [[nodiscard]] StackArray extend(const Series& value) const {
    return StackArray(values_.extend(value));
  }
  [[nodiscard]] StackArray extend(pluss::msharp::RangeView::Of<Series> values) const {
    return StackArray(values_.extend(values));
  }
  [[nodiscard]] StackArray extend(std::initializer_list<Series> values) const {
    return StackArray(values_.extend(values));
  }
  template <pluss::msharp::detail::lister::RangeOf<Series> Range>
    requires(!std::same_as<std::remove_cvref_t<Range>, std::vector<Series>> &&
             !std::same_as<std::remove_cvref_t<Range>, pluss::msharp::RangeView::Of<Series>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::initializer_list<Series>>)
  [[nodiscard]] StackArray extend(Range&& values) const {
    return StackArray(values_.extend(std::forward<Range>(values)));
  }

  [[nodiscard]] detail::stack_array::StackSpan data() const noexcept {
    return detail::stack_array::StackSpan{values_.data()};
  }

  [[nodiscard]] const lister_type& lister() const noexcept { return values_; }

  operator lister_type() const & { return values_; }
  operator lister_type() && { return std::move(values_); }

  [[nodiscard]] iterator begin() noexcept { return values_.begin(); }
  [[nodiscard]] iterator end() noexcept { return values_.end(); }
  [[nodiscard]] const_iterator begin() const noexcept { return values_.begin(); }
  [[nodiscard]] const_iterator end() const noexcept { return values_.end(); }

private:
  lister_type values_{};
};

template <>
class StackArray<std::string_view> {
public:
  using value_type = std::string_view;
  using lister_type = pluss::msharp::Lister::StringView;
  using iterator = typename lister_type::iterator;
  using const_iterator = typename lister_type::const_iterator;

  StackArray() = default;
  StackArray(std::initializer_list<std::string_view> values) : values_(values) {}
  StackArray(std::initializer_list<const char*> values) : values_(values) {}
  StackArray(std::span<const std::string_view> values) : values_(values) {}
  StackArray(std::span<const std::string> values) : values_(values) {}
  StackArray(std::vector<std::string> values) : values_(std::move(values)) {}
  StackArray(std::vector<std::string_view> values) : values_(std::move(values)) {}
  StackArray(const lister_type& values) : values_(values) {}
  StackArray(lister_type&& values) : values_(std::move(values)) {}

  template <pluss::msharp::detail::lister::StringViewRange Range>
    requires(!std::same_as<std::remove_cvref_t<Range>, StackArray<std::string_view>> &&
             !std::same_as<std::remove_cvref_t<Range>, lister_type> &&
             !std::same_as<std::remove_cvref_t<Range>, std::vector<std::string>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::vector<std::string_view>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::span<const std::string_view>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::span<std::string_view>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::span<const std::string>>)
  StackArray(Range&& values) : values_(std::forward<Range>(values)) {}

  [[nodiscard]] bool empty() const noexcept { return values_.empty(); }
  [[nodiscard]] std::size_t size() const noexcept { return values_.size(); }
  [[nodiscard]] std::string_view get(i64_t i) const { return values_.get(i); }

  [[nodiscard]] detail::stack_array::StringSpan data() const noexcept {
    return detail::stack_array::StringSpan{values_.span()};
  }

  [[nodiscard]] const lister_type& lister() const noexcept { return values_; }

  operator lister_type() const & { return values_; }
  operator lister_type() && { return std::move(values_); }

  [[nodiscard]] iterator begin() noexcept { return values_.begin(); }
  [[nodiscard]] iterator end() noexcept { return values_.end(); }
  [[nodiscard]] const_iterator begin() const noexcept { return values_.begin(); }
  [[nodiscard]] const_iterator end() const noexcept { return values_.end(); }

private:
  lister_type values_{};
};

template <>
class StackArray<std::string> {
public:
  using value_type = std::string;
  using lister_type = pluss::msharp::Lister::String;
  using iterator = typename lister_type::iterator;
  using const_iterator = typename lister_type::const_iterator;

  StackArray() = default;
  StackArray(std::initializer_list<std::string> values) : values_(values) {}
  StackArray(std::initializer_list<const char*> values) : values_(values) {}
  StackArray(std::span<const std::string> values) : values_(values) {}
  StackArray(std::vector<std::string> values) : values_(std::move(values)) {}
  StackArray(const lister_type& values) : values_(values) {}
  StackArray(lister_type&& values) : values_(std::move(values)) {}

  template <pluss::msharp::detail::lister::StringViewRange Range>
    requires(!std::same_as<std::remove_cvref_t<Range>, StackArray<std::string>> &&
             !std::same_as<std::remove_cvref_t<Range>, lister_type> &&
             !std::same_as<std::remove_cvref_t<Range>, std::vector<std::string>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::span<const std::string>>)
  StackArray(Range&& values) : values_(std::forward<Range>(values)) {}

  [[nodiscard]] bool empty() const noexcept { return values_.empty(); }
  [[nodiscard]] std::size_t size() const noexcept { return values_.size(); }
  [[nodiscard]] const std::string& get(i64_t i) const { return values_.get(i); }
  [[nodiscard]] std::string& get(i64_t i) { return values_.get(i); }
  [[nodiscard]] const std::string& operator[](i64_t i) const { return get(i); }
  [[nodiscard]] std::string& operator[](i64_t i) { return get(i); }

  [[nodiscard]] StackArray extend(std::string_view value) const {
    return StackArray(values_.extend(value));
  }
  [[nodiscard]] StackArray extend(std::span<const std::string> values) const {
    return StackArray(values_.extend(values));
  }
  [[nodiscard]] StackArray extend(std::initializer_list<std::string> values) const {
    return StackArray(values_.extend(values));
  }

  [[nodiscard]] detail::stack_array::StringSpan data() const {
    return detail::stack_array::StringSpan{values_.text_view().span()};
  }

  [[nodiscard]] const lister_type& lister() const noexcept { return values_; }

  operator lister_type() const & { return values_; }
  operator lister_type() && { return std::move(values_); }

  [[nodiscard]] iterator begin() noexcept { return values_.begin(); }
  [[nodiscard]] iterator end() noexcept { return values_.end(); }
  [[nodiscard]] const_iterator begin() const noexcept { return values_.begin(); }
  [[nodiscard]] const_iterator end() const noexcept { return values_.end(); }

private:
  lister_type values_{};
};

template <>
class StackArray<std::int64_t> {
public:
  using value_type = std::int64_t;
  using lister_type = pluss::msharp::Lister::Int;
  using iterator = typename lister_type::iterator;
  using const_iterator = typename lister_type::const_iterator;

  StackArray() = default;
  StackArray(std::initializer_list<std::int64_t> values) : values_(values) {}
  StackArray(std::span<const std::int64_t> values) : values_(values) {}
  StackArray(std::vector<std::int64_t> values) : values_(std::move(values)) {}
  StackArray(const lister_type& values) : values_(values) {}
  StackArray(lister_type&& values) : values_(std::move(values)) {}

  template <pluss::msharp::detail::lister::RangeOf<std::int64_t> Range>
    requires(!std::same_as<std::remove_cvref_t<Range>, StackArray<std::int64_t>> &&
             !std::same_as<std::remove_cvref_t<Range>, lister_type> &&
             !std::same_as<std::remove_cvref_t<Range>, std::vector<std::int64_t>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::span<const std::int64_t>>)
  StackArray(Range&& values) : values_(std::forward<Range>(values)) {}

  [[nodiscard]] bool empty() const noexcept { return values_.empty(); }
  [[nodiscard]] std::size_t size() const noexcept { return values_.size(); }
  [[nodiscard]] std::int64_t get(i64_t i) const { return values_.get(i); }
  [[nodiscard]] std::int64_t& get(i64_t i) { return values_.get(i); }
  [[nodiscard]] std::int64_t operator[](i64_t i) const { return get(i); }
  [[nodiscard]] std::int64_t& operator[](i64_t i) { return get(i); }

  [[nodiscard]] StackArray extend(std::int64_t value) const {
    return StackArray(values_.extend(value));
  }
  [[nodiscard]] StackArray extend(std::span<const std::int64_t> values) const {
    return StackArray(values_.extend(values));
  }
  [[nodiscard]] StackArray extend(std::initializer_list<std::int64_t> values) const {
    return StackArray(values_.extend(values));
  }

  [[nodiscard]] detail::stack_array::Int64Span data() const noexcept {
    return detail::stack_array::Int64Span{values_.span()};
  }

  [[nodiscard]] const lister_type& lister() const noexcept { return values_; }

  operator lister_type() const & { return values_; }
  operator lister_type() && { return std::move(values_); }

  [[nodiscard]] iterator begin() noexcept { return values_.begin(); }
  [[nodiscard]] iterator end() noexcept { return values_.end(); }
  [[nodiscard]] const_iterator begin() const noexcept { return values_.begin(); }
  [[nodiscard]] const_iterator end() const noexcept { return values_.end(); }

private:
  lister_type values_{};
};

template <>
class StackArray<double> {
public:
  using value_type = double;
  using lister_type = pluss::msharp::Lister::Double;
  using iterator = typename lister_type::iterator;
  using const_iterator = typename lister_type::const_iterator;

  StackArray() = default;
  StackArray(std::initializer_list<double> values) : values_(values) {}
  StackArray(std::span<const double> values) : values_(values) {}
  StackArray(std::vector<double> values) : values_(std::move(values)) {}
  StackArray(const lister_type& values) : values_(values) {}
  StackArray(lister_type&& values) : values_(std::move(values)) {}

  template <pluss::msharp::detail::lister::RangeOf<double> Range>
    requires(!std::same_as<std::remove_cvref_t<Range>, StackArray<double>> &&
             !std::same_as<std::remove_cvref_t<Range>, lister_type> &&
             !std::same_as<std::remove_cvref_t<Range>, std::vector<double>> &&
             !std::same_as<std::remove_cvref_t<Range>, std::span<const double>>)
  StackArray(Range&& values) : values_(std::forward<Range>(values)) {}

  [[nodiscard]] bool empty() const noexcept { return values_.empty(); }
  [[nodiscard]] std::size_t size() const noexcept { return values_.size(); }
  [[nodiscard]] double get(i64_t i) const { return values_.get(i); }
  [[nodiscard]] double& get(i64_t i) { return values_.get(i); }
  [[nodiscard]] double operator[](i64_t i) const { return get(i); }
  [[nodiscard]] double& operator[](i64_t i) { return get(i); }

  [[nodiscard]] StackArray extend(double value) const {
    return StackArray(values_.extend(value));
  }
  [[nodiscard]] StackArray extend(std::span<const double> values) const {
    return StackArray(values_.extend(values));
  }
  [[nodiscard]] StackArray extend(std::initializer_list<double> values) const {
    return StackArray(values_.extend(values));
  }

  [[nodiscard]] detail::stack_array::DoubleSpan data() const noexcept {
    return detail::stack_array::DoubleSpan{values_.span()};
  }

  [[nodiscard]] const lister_type& lister() const noexcept { return values_; }

  operator lister_type() const & { return values_; }
  operator lister_type() && { return std::move(values_); }

  [[nodiscard]] iterator begin() noexcept { return values_.begin(); }
  [[nodiscard]] iterator end() noexcept { return values_.end(); }
  [[nodiscard]] const_iterator begin() const noexcept { return values_.begin(); }
  [[nodiscard]] const_iterator end() const noexcept { return values_.end(); }

private:
  lister_type values_{};
};

using sign_list = StackArray<Series>;
using SignList = sign_list;

inline Series pile(detail::stack_array::StackSpan values, std::int64_t dim = 0) {
  return detail::stack_array::pile_span(values.values, dim);
}

inline Series pile(const pluss::msharp::Lister::Series& values, std::int64_t dim = 0) {
  return detail::stack_array::pile_span(values.data(), dim);
}

inline Series unite(detail::stack_array::StackSpan values,
                    std::int64_t dim = 0,
                    bool strict_dtype = true) {
  return detail::stack_array::unite_span(values.values, dim, strict_dtype);
}

inline Series unite(const pluss::msharp::Lister::Series& values,
                    std::int64_t dim = 0,
                    bool strict_dtype = true) {
  return detail::stack_array::unite_span(values.data(), dim, strict_dtype);
}

} // namespace pluss
