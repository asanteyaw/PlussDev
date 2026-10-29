#pragma once

#include <cmath>
#include <concepts>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "pluss/codem/codem.h"
#include "pluss/subsp/natsci/common/unit.h"

namespace pluss::subsp::natsci::common {

template <class Rep>
concept QuantityRep =
    std::same_as<std::remove_cvref_t<Rep>, double> ||
    std::same_as<std::remove_cvref_t<Rep>, pluss::codem::Series>;

namespace detail {

inline void validate_quantity_rep(double) {}
inline void validate_quantity_rep(const pluss::codem::Series &value) {
  if (!value.defined())
    throw std::invalid_argument(
        "natsci::common::Quantity: expected a defined Series");
  if (!value.dtype().is_floating() && !value.dtype().is_complex())
    throw std::invalid_argument("natsci::common::Quantity: Series "
                                "representation must be floating or complex");
}

inline double scale_rep(double value, double scale) { return value * scale; }
inline pluss::codem::Series scale_rep(const pluss::codem::Series &value,
                                      double scale) {
  return value.mul(pluss::codem::make_scalar_like(value, scale));
}

inline double unscale_rep(double value, double scale) { return value / scale; }
inline pluss::codem::Series unscale_rep(const pluss::codem::Series &value,
                                        double scale) {
  return value.div(pluss::codem::make_scalar_like(value, scale));
}

inline double add_rep(double lhs, double rhs) { return lhs + rhs; }
inline pluss::codem::Series add_rep(const pluss::codem::Series &lhs,
                                    const pluss::codem::Series &rhs) {
  return lhs.add(rhs);
}
inline pluss::codem::Series add_rep(const pluss::codem::Series &lhs,
                                    double rhs) {
  return lhs.add(rhs, 1.0);
}
inline pluss::codem::Series add_rep(double lhs,
                                    const pluss::codem::Series &rhs) {
  return rhs.add(lhs, 1.0);
}

inline double sub_rep(double lhs, double rhs) { return lhs - rhs; }
inline pluss::codem::Series sub_rep(const pluss::codem::Series &lhs,
                                    const pluss::codem::Series &rhs) {
  return lhs.sub(rhs);
}
inline pluss::codem::Series sub_rep(const pluss::codem::Series &lhs,
                                    double rhs) {
  return lhs.sub(rhs, 1.0);
}
inline pluss::codem::Series sub_rep(double lhs,
                                    const pluss::codem::Series &rhs) {
  return pluss::codem::make_scalar_like(rhs, lhs).sub(rhs);
}

inline double mul_rep(double lhs, double rhs) { return lhs * rhs; }
inline pluss::codem::Series mul_rep(const pluss::codem::Series &lhs,
                                    const pluss::codem::Series &rhs) {
  return lhs.mul(rhs);
}
inline pluss::codem::Series mul_rep(const pluss::codem::Series &lhs,
                                    double rhs) {
  return lhs.mul(pluss::codem::make_scalar_like(lhs, rhs));
}
inline pluss::codem::Series mul_rep(double lhs,
                                    const pluss::codem::Series &rhs) {
  return rhs.mul(pluss::codem::make_scalar_like(rhs, lhs));
}

inline double div_rep(double lhs, double rhs) { return lhs / rhs; }
inline pluss::codem::Series div_rep(const pluss::codem::Series &lhs,
                                    const pluss::codem::Series &rhs) {
  return lhs.div(rhs);
}
inline pluss::codem::Series div_rep(const pluss::codem::Series &lhs,
                                    double rhs) {
  return lhs.div(pluss::codem::make_scalar_like(lhs, rhs));
}
inline pluss::codem::Series div_rep(double lhs,
                                    const pluss::codem::Series &rhs) {
  return pluss::codem::make_scalar_like(rhs, lhs).div(rhs);
}

inline double abs_rep(double value) { return std::abs(value); }
inline pluss::codem::Series abs_rep(const pluss::codem::Series &value) {
  return value.abs();
}

inline double sqrt_rep(double value) { return std::sqrt(value); }
inline pluss::codem::Series sqrt_rep(const pluss::codem::Series &value) {
  return value.sqrt();
}

inline double log_rep(double value) { return std::log(value); }
inline pluss::codem::Series log_rep(const pluss::codem::Series &value) {
  return value.log();
}

inline double exp_rep(double value) { return std::exp(value); }
inline pluss::codem::Series exp_rep(const pluss::codem::Series &value) {
  return value.exp();
}

inline double pow_rep(double base, double exponent) {
  return std::pow(base, exponent);
}
inline pluss::codem::Series pow_rep(const pluss::codem::Series &base,
                                    double exponent) {
  return base.pow(exponent);
}
inline pluss::codem::Series pow_rep(double base,
                                    const pluss::codem::Series &exponent) {
  return exponent.mul(std::log(base)).exp();
}
inline pluss::codem::Series pow_rep(const pluss::codem::Series &base,
                                    const pluss::codem::Series &exponent) {
  return base.log().mul(exponent).exp();
}

inline double zero_rep(double) { return 0.0; }
inline pluss::codem::Series zero_rep(const pluss::codem::Series &value) {
  return pluss::codem::Series::zeros(value.sizes(), value.scalar_type(),
                                     value.platform());
}

template <class Lhs, class Rhs>
[[nodiscard]] auto hypot_rep(const Lhs &lhs, const Rhs &rhs) {
  return sqrt_rep(add_rep(mul_rep(lhs, lhs), mul_rep(rhs, rhs)));
}

} // namespace detail

template <DimensionType D, QuantityRep Rep = double> class Quantity final {
public:
  using dimension = D;
  using rep_type = Rep;

  [[nodiscard]] static Quantity from(Rep value, const Unit<D> &unit) {
    return from_si(detail::scale_rep(value, unit.si_scale()));
  }

  [[nodiscard]] static Quantity from_si(Rep value) {
    return Quantity(std::move(value));
  }

  [[nodiscard]] const Rep &si_value() const noexcept { return si_value_; }

  [[nodiscard]] Rep in(const Unit<D> &unit) const {
    return detail::unscale_rep(si_value_, unit.si_scale());
  }

  template <QuantityRep OtherRep>
  [[nodiscard]] auto operator+(const Quantity<D, OtherRep> &rhs) const {
    using ResultRep = decltype(detail::add_rep(si_value_, rhs.si_value()));
    return Quantity<D, ResultRep>::from_si(
        detail::add_rep(si_value_, rhs.si_value()));
  }

  template <QuantityRep OtherRep>
  [[nodiscard]] auto operator-(const Quantity<D, OtherRep> &rhs) const {
    using ResultRep = decltype(detail::sub_rep(si_value_, rhs.si_value()));
    return Quantity<D, ResultRep>::from_si(
        detail::sub_rep(si_value_, rhs.si_value()));
  }

  template <DimensionType OtherDimension, QuantityRep OtherRep>
  [[nodiscard]] auto
  operator*(const Quantity<OtherDimension, OtherRep> &rhs) const {
    using ResultRep = decltype(detail::mul_rep(si_value_, rhs.si_value()));
    using ResultDimension = DimensionProduct<D, OtherDimension>;
    return Quantity<ResultDimension, ResultRep>::from_si(
        detail::mul_rep(si_value_, rhs.si_value()));
  }

  template <DimensionType OtherDimension, QuantityRep OtherRep>
  [[nodiscard]] auto
  operator/(const Quantity<OtherDimension, OtherRep> &rhs) const {
    using ResultRep = decltype(detail::div_rep(si_value_, rhs.si_value()));
    using ResultDimension = DimensionQuotient<D, OtherDimension>;
    return Quantity<ResultDimension, ResultRep>::from_si(
        detail::div_rep(si_value_, rhs.si_value()));
  }

  [[nodiscard]] Quantity operator*(double scalar) const {
    return from_si(detail::scale_rep(si_value_, scalar));
  }

  [[nodiscard]] Quantity operator/(double scalar) const {
    return from_si(detail::unscale_rep(si_value_, scalar));
  }

  [[nodiscard]] Quantity operator-() const {
    return from_si(detail::scale_rep(si_value_, -1.0));
  }

private:
  explicit Quantity(Rep value) : si_value_(std::move(value)) {
    detail::validate_quantity_rep(si_value_);
  }

  Rep si_value_;
};

template <DimensionType D, QuantityRep Rep>
[[nodiscard]] Quantity<D, Rep> operator*(double scalar,
                                         const Quantity<D, Rep> &value) {
  return value * scalar;
}

template <DimensionType D> using ScalarQuantity = Quantity<D, double>;

template <DimensionType D>
using SeriesQuantity = Quantity<D, pluss::codem::Series>;

} // namespace pluss::subsp::natsci::common
