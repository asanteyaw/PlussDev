#pragma once

#include <initializer_list>
#include <ranges>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace pluss::detail {

template <class T>
struct IsInitializerList : std::false_type {};

template <class T>
struct IsInitializerList<std::initializer_list<T>> : std::true_type {};

template <class T>
inline constexpr bool IsInitializerListV = IsInitializerList<std::remove_cvref_t<T>>::value;

template <class NameRange>
concept StringViewNameRange =
    std::ranges::input_range<NameRange> &&
    !std::is_convertible_v<NameRange, std::string_view> &&
    !IsInitializerListV<NameRange> &&
    !std::same_as<std::remove_cvref_t<NameRange>, std::span<const std::string_view>> &&
    std::convertible_to<std::remove_cvref_t<std::ranges::range_reference_t<NameRange>>, std::string_view>;

template <StringViewNameRange NameRange, class Fn>
decltype(auto) with_name_span(const NameRange& names, Fn&& fn) {
  std::vector<std::string_view> owned;
  if constexpr (std::ranges::sized_range<NameRange>) {
    owned.reserve(static_cast<std::size_t>(std::ranges::size(names)));
  }
  for (auto&& name : names) {
    owned.emplace_back(std::string_view(name));
  }
  return std::forward<Fn>(fn)(std::span<const std::string_view>(owned.data(), owned.size()));
}

} // namespace pluss::detail
