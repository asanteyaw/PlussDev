#pragma once

#include "pluss/subsp/natsci/common/quantity.h"

#include <stdexcept>
#include <utility>

namespace pluss::subsp::natsci::common {

namespace detail {

inline void validate_uncertainty(double, double uncertainty) {
  if (!(uncertainty >= 0.0))
    throw std::invalid_argument("natsci::common::Measurement: standard "
                                "uncertainty must be non-negative");
}

inline void validate_uncertainty(const Series &value,
                                 const Series &uncertainty) {
  if (!value.defined() || !uncertainty.defined())
    throw std::invalid_argument(
        "natsci::common::Measurement: expected defined Series values");
  if (!value.dtype().is_floating() || !uncertainty.dtype().is_floating())
    throw std::invalid_argument("natsci::common::Measurement: Series values "
                                "must have a real floating dtype");
  if (value.scalar_type() != uncertainty.scalar_type())
    throw std::invalid_argument(
        "natsci::common::Measurement: value and uncertainty dtype mismatch");
  if (value.platform() != uncertainty.platform())
    throw std::invalid_argument(
        "natsci::common::Measurement: value and uncertainty platform mismatch");
  if (value.dim() != uncertainty.dim())
    throw std::invalid_argument(
        "natsci::common::Measurement: value and uncertainty shape mismatch");
  for (std::int64_t axis = 0; axis < value.dim(); ++axis) {
    if (value.size(axis) != uncertainty.size(axis))
      throw std::invalid_argument(
          "natsci::common::Measurement: value and uncertainty shape mismatch");
  }
  if (!uncertainty.ge(0.0).all().item<bool>())
    throw std::invalid_argument("natsci::common::Measurement: standard "
                                "uncertainty must be non-negative");
}

} // namespace detail

// First-order propagation for independent one-standard-deviation uncertainty.
// Series representations preserve arbitrary batch shape, backend and tape
// links.
template <DimensionType D, QuantityRep Rep = double> class Measurement final {
public:
  Measurement(Quantity<D, Rep> value, Quantity<D, Rep> standard_uncertainty)
      : value_(std::move(value)),
        standard_uncertainty_(std::move(standard_uncertainty)) {
    detail::validate_uncertainty(value_.si_value(),
                                 standard_uncertainty_.si_value());
  }

  [[nodiscard]] const Quantity<D, Rep> &value() const noexcept {
    return value_;
  }
  [[nodiscard]] const Quantity<D, Rep> &standard_uncertainty() const noexcept {
    return standard_uncertainty_;
  }

  [[nodiscard]] Rep relative_standard_uncertainty() const {
    return detail::div_rep(standard_uncertainty_.si_value(),
                           detail::abs_rep(value_.si_value()));
  }

  [[nodiscard]] Quantity<D, Rep> lower() const {
    return value_ - standard_uncertainty_;
  }

  [[nodiscard]] Quantity<D, Rep> upper() const {
    return value_ + standard_uncertainty_;
  }

  template <QuantityRep OtherRep>
  [[nodiscard]] auto operator+(const Measurement<D, OtherRep> &rhs) const {
    using ResultRep =
        decltype(detail::add_rep(value_.si_value(), rhs.value().si_value()));
    return Measurement<D, ResultRep>(
        value_ + rhs.value(), Quantity<D, ResultRep>::from_si(detail::hypot_rep(
                                  standard_uncertainty_.si_value(),
                                  rhs.standard_uncertainty().si_value())));
  }

  template <QuantityRep OtherRep>
  [[nodiscard]] auto operator-(const Measurement<D, OtherRep> &rhs) const {
    using ResultRep =
        decltype(detail::sub_rep(value_.si_value(), rhs.value().si_value()));
    return Measurement<D, ResultRep>(
        value_ - rhs.value(), Quantity<D, ResultRep>::from_si(detail::hypot_rep(
                                  standard_uncertainty_.si_value(),
                                  rhs.standard_uncertainty().si_value())));
  }

  template <DimensionType OtherDimension, QuantityRep OtherRep>
  [[nodiscard]] auto
  operator*(const Measurement<OtherDimension, OtherRep> &rhs) const {
    using ResultDimension = DimensionProduct<D, OtherDimension>;
    const auto lhs_term = detail::mul_rep(rhs.value().si_value(),
                                          standard_uncertainty_.si_value());
    const auto rhs_term = detail::mul_rep(
        value_.si_value(), rhs.standard_uncertainty().si_value());
    using ResultRep = decltype(detail::hypot_rep(lhs_term, rhs_term));
    return Measurement<ResultDimension, ResultRep>(
        value_ * rhs.value(), Quantity<ResultDimension, ResultRep>::from_si(
                                  detail::hypot_rep(lhs_term, rhs_term)));
  }

  template <DimensionType OtherDimension, QuantityRep OtherRep>
  [[nodiscard]] auto
  operator/(const Measurement<OtherDimension, OtherRep> &rhs) const {
    using ResultDimension = DimensionQuotient<D, OtherDimension>;
    const auto result =
        detail::div_rep(value_.si_value(), rhs.value().si_value());
    const auto lhs_term = detail::div_rep(standard_uncertainty_.si_value(),
                                          rhs.value().si_value());
    const auto rhs_term = detail::div_rep(
        detail::mul_rep(result, rhs.standard_uncertainty().si_value()),
        rhs.value().si_value());
    using ResultRep = decltype(detail::hypot_rep(lhs_term, rhs_term));
    return Measurement<ResultDimension, ResultRep>(
        Quantity<ResultDimension, ResultRep>::from_si(result),
        Quantity<ResultDimension, ResultRep>::from_si(
            detail::hypot_rep(lhs_term, rhs_term)));
  }

private:
  Quantity<D, Rep> value_;
  Quantity<D, Rep> standard_uncertainty_;
};

} // namespace pluss::subsp::natsci::common
