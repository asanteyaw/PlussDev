#pragma once

#include "pluss/subsp/natsci/common/quantity.h"

#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace pluss::subsp::natsci::common {

template <DimensionType D> class AffineUnit final {
public:
  constexpr AffineUnit(double si_scale, double si_offset,
                       std::string_view symbol, std::string_view name)
      : si_scale_(si_scale), si_offset_(si_offset), symbol_(symbol),
        name_(name) {
    if (!(si_scale > 0.0) ||
        !(si_scale <= std::numeric_limits<double>::max())) {
      throw std::invalid_argument(
          "natsci::common::AffineUnit: SI scale must be finite and positive");
    }
    if (!(si_offset >= -std::numeric_limits<double>::max() &&
          si_offset <= std::numeric_limits<double>::max())) {
      throw std::invalid_argument(
          "natsci::common::AffineUnit: SI offset must be finite");
    }
  }

  [[nodiscard]] constexpr double si_scale() const noexcept { return si_scale_; }
  [[nodiscard]] constexpr double si_offset() const noexcept {
    return si_offset_;
  }
  [[nodiscard]] constexpr std::string_view symbol() const noexcept {
    return symbol_;
  }
  [[nodiscard]] constexpr std::string_view name() const noexcept {
    return name_;
  }

private:
  double si_scale_;
  double si_offset_;
  std::string_view symbol_;
  std::string_view name_;
};

template <DimensionType D, QuantityRep Rep = double> class Point final {
public:
  using dimension = D;
  using rep_type = Rep;

  [[nodiscard]] static Point from(Rep value, const AffineUnit<D> &unit) {
    return from_si(detail::add_rep(detail::scale_rep(value, unit.si_scale()),
                                   unit.si_offset()));
  }

  [[nodiscard]] static Point from_si(Rep value) {
    return Point(std::move(value));
  }

  [[nodiscard]] const Rep &si_value() const noexcept { return si_value_; }

  [[nodiscard]] Rep in(const AffineUnit<D> &unit) const {
    return detail::unscale_rep(detail::sub_rep(si_value_, unit.si_offset()),
                               unit.si_scale());
  }

  template <QuantityRep OtherRep>
  [[nodiscard]] auto operator-(const Point<D, OtherRep> &rhs) const {
    using ResultRep = decltype(detail::sub_rep(si_value_, rhs.si_value()));
    return Quantity<D, ResultRep>::from_si(
        detail::sub_rep(si_value_, rhs.si_value()));
  }

  template <QuantityRep OtherRep>
  [[nodiscard]] auto operator+(const Quantity<D, OtherRep> &shift) const {
    using ResultRep = decltype(detail::add_rep(si_value_, shift.si_value()));
    return Point<D, ResultRep>::from_si(
        detail::add_rep(si_value_, shift.si_value()));
  }

  template <QuantityRep OtherRep>
  [[nodiscard]] auto operator-(const Quantity<D, OtherRep> &shift) const {
    using ResultRep = decltype(detail::sub_rep(si_value_, shift.si_value()));
    return Point<D, ResultRep>::from_si(
        detail::sub_rep(si_value_, shift.si_value()));
  }

private:
  explicit Point(Rep value) : si_value_(std::move(value)) {
    if constexpr (std::same_as<Rep, pluss::codem::Series>) {
      if (!si_value_.defined() || !si_value_.dtype().is_floating())
        throw std::invalid_argument("natsci::common::Point: Series "
                                    "representation must be real floating");
    }
  }

  Rep si_value_;
};

template <QuantityRep Rep = double>
using TemperaturePoint = Point<Temperature, Rep>;

template <DimensionType D, QuantityRep QuantityRepType,
          QuantityRep PointRepType>
[[nodiscard]] auto operator+(const Quantity<D, QuantityRepType> &shift,
                             const Point<D, PointRepType> &point) {
  return point + shift;
}

namespace units {

inline constexpr AffineUnit<Temperature> kelvin_point{1.0, 0.0, "K", "kelvin"};
inline constexpr AffineUnit<Temperature> celsius{1.0, 273.15, "degC",
                                                 "degree Celsius"};
inline constexpr AffineUnit<Temperature> fahrenheit{
    5.0 / 9.0, 255.3722222222222222, "degF", "degree Fahrenheit"};

} // namespace units

} // namespace pluss::subsp::natsci::common
