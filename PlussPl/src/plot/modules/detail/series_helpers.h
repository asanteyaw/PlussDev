#pragma once

#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <vector>

#include <fmt/format.h>

#include "pluss/codem/codem.h"
#include "pluss/coden/coden.h"
#include "pluss/codeh/codeh.h"

namespace pluss::plotism::detail {

template <typename T>
concept PlotNumeric = std::is_arithmetic_v<std::remove_cvref_t<T>> &&
                      !std::same_as<std::remove_cvref_t<T>, bool> &&
                      !std::same_as<std::remove_cvref_t<T>, char> &&
                      !std::same_as<std::remove_cvref_t<T>, signed char> &&
                      !std::same_as<std::remove_cvref_t<T>, unsigned char> &&
                      !std::same_as<std::remove_cvref_t<T>, wchar_t> &&
                      !std::same_as<std::remove_cvref_t<T>, char8_t> &&
                      !std::same_as<std::remove_cvref_t<T>, char16_t> &&
                      !std::same_as<std::remove_cvref_t<T>, char32_t>;

template <typename Range>
concept NumericRange = std::ranges::input_range<Range> && PlotNumeric<std::ranges::range_value_t<Range>>;

template <typename Input>
concept PlotInput = std::same_as<std::remove_cvref_t<Input>, pluss::Series> ||
                    std::same_as<std::remove_cvref_t<Input>, pluss::Note> ||
                    std::same_as<std::remove_cvref_t<Input>, pluss::Timeline> ||
                    std::same_as<std::remove_cvref_t<Input>, pluss::TimeIndex> ||
                    NumericRange<Input>;

inline std::vector<double> make_index_axis(std::size_t size) {
  std::vector<double> x(size, 0.0);
  std::iota(x.begin(), x.end(), 0.0);
  return x;
}

inline std::vector<double> series_to_plot_vector(const pluss::Series& values, std::string_view where) {
  if (!values.defined()) {
    throw std::invalid_argument(fmt::format("plotism::{}: Series is undefined", where));
  }
  if (values.numel() < 0) {
    throw std::invalid_argument(fmt::format("plotism::{}: Series has invalid numel", where));
  }
  return values.to_vec<double>();
}

inline std::string_view trim_ascii(std::string_view text) {
  std::size_t begin = 0;
  std::size_t end = text.size();
  while (begin < end && static_cast<unsigned char>(text[begin]) <= 32U) {
    ++begin;
  }
  while (end > begin && static_cast<unsigned char>(text[end - 1]) <= 32U) {
    --end;
  }
  return text.substr(begin, end - begin);
}

inline bool parse_double(std::string_view text, double& out) {
  const auto trimmed = trim_ascii(text);
  if (trimmed.empty()) {
    return false;
  }
  const char* begin = trimmed.data();
  const char* end = begin + trimmed.size();
  const auto [ptr, ec] = std::from_chars(begin, end, out, std::chars_format::general);
  return ec == std::errc() && ptr == end;
}

inline std::vector<double> note_to_plot_vector(const pluss::Note& values, std::string_view where) {
  if (!values.defined()) {
    throw std::invalid_argument(fmt::format("plotism::{}: Note is undefined", where));
  }

  std::vector<double> numeric;
  numeric.reserve(static_cast<std::size_t>(values.numel()));
  bool all_numeric = true;
  for (const auto token : values) {
    double parsed = 0.0;
    if (!parse_double(token, parsed)) {
      all_numeric = false;
      break;
    }
    numeric.push_back(parsed);
  }
  if (all_numeric) {
    return numeric;
  }

  std::vector<double> lengths;
  lengths.reserve(static_cast<std::size_t>(values.numel()));
  for (const auto token : values) {
    lengths.push_back(static_cast<double>(token.size()));
  }
  return lengths;
}

inline std::vector<double> timeline_to_plot_vector(const pluss::Timeline& values, std::string_view where) {
  if (!values.defined()) {
    throw std::invalid_argument(fmt::format("plotism::{}: Timeline is undefined", where));
  }
  return values.epoch_seconds().to_vec<double>();
}

inline std::vector<double> timeindex_to_plot_vector(const pluss::TimeIndex& values, std::string_view where) {
  return timeline_to_plot_vector(values.timeline(), where);
}

template <NumericRange Range>
inline std::vector<double> numeric_range_to_plot_vector(const Range& values) {
  std::vector<double> out;
  if constexpr (std::ranges::sized_range<Range>) {
    out.reserve(static_cast<std::size_t>(std::ranges::size(values)));
  }
  for (const auto value : values) {
    out.push_back(static_cast<double>(value));
  }
  return out;
}

inline std::vector<double> to_plot_vector(const pluss::Series& values, std::string_view where) {
  return series_to_plot_vector(values, where);
}

inline std::vector<double> to_plot_vector(const pluss::Note& values, std::string_view where) {
  return note_to_plot_vector(values, where);
}

inline std::vector<double> to_plot_vector(const pluss::Timeline& values, std::string_view where) {
  return timeline_to_plot_vector(values, where);
}

inline std::vector<double> to_plot_vector(const pluss::TimeIndex& values, std::string_view where) {
  return timeindex_to_plot_vector(values, where);
}

template <NumericRange Range>
inline std::vector<double> to_plot_vector(const Range& values, std::string_view) {
  return numeric_range_to_plot_vector(values);
}

} // namespace pluss::plotism::detail
