#pragma once

#include "MSharp/RangeView.h"
#include "pluss/codem/codem.h"

#include <array>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>

namespace pluss::detail::select_core {

template <class T, class Impl>
inline T select_range(std::string_view where,
                      pluss::msharp::RangeView::Of<pluss::Series> conditions,
                      pluss::msharp::RangeView::Of<T> choices,
                      const T& default_value,
                      Impl&& impl) {
  return std::forward<Impl>(impl)(where, conditions, choices, default_value);
}

template <class T, class Impl>
inline T select_init(std::string_view where,
                     std::initializer_list<pluss::Series> conditions,
                     std::initializer_list<T> choices,
                     const T& default_value,
                     Impl&& impl) {
  return select_range(
      where,
      pluss::msharp::RangeView::Of<pluss::Series>(conditions),
      pluss::msharp::RangeView::Of<T>(choices),
      default_value,
      std::forward<Impl>(impl));
}

template <class T, class Impl>
inline T select_scalar(std::string_view where,
                       const pluss::Series& condition,
                       const T& choice,
                       const T& default_value,
                       Impl&& impl) {
  const std::array<pluss::Series, 1> conditions{condition};
  const std::array<T, 1> choices{choice};
  return select_range(
      where,
      pluss::msharp::RangeView::Of<pluss::Series>(conditions),
      pluss::msharp::RangeView::Of<T>(choices),
      default_value,
      std::forward<Impl>(impl));
}

template <class T, class Combine>
inline T fold_select(std::string_view where,
                     pluss::msharp::RangeView::Of<pluss::Series> conditions,
                     pluss::msharp::RangeView::Of<T> choices,
                     const T& default_value,
                     Combine&& combine) {
  PLUSS_CHECK(!conditions.empty(), std::string(where) + ": at least one condition is required");
  PLUSS_CHECK(conditions.size() == choices.size(),
              std::string(where) + ": condition/choice count mismatch");
  PLUSS_CHECK(default_value.defined(), std::string(where) + ": default value is undefined");

  T out = default_value;
  for (std::size_t i = conditions.size(); i > 0; --i) {
    const auto& cond = conditions[i - 1];
    const auto& choice = choices[i - 1];
    PLUSS_CHECK(cond.defined(), std::string(where) + ": condition is undefined");
    PLUSS_CHECK(choice.defined(), std::string(where) + ": choice is undefined");
    PLUSS_CHECK(cond.scalar_type() == pluss::ScalarType::Bool,
                std::string(where) + ": conditions must use Bool dtype");
    out = std::forward<Combine>(combine)(cond, choice, out);
  }
  return out;
}

} // namespace pluss::detail::select_core
