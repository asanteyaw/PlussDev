#pragma once

#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/roots.h"
#include "pluss/subsp/natsci/common/quantity.h"

#include <cmath>
#include <cstdint>
#include <numbers>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace pluss::subsp::natsci::physics::astro {

namespace common = ::pluss::subsp::natsci::common;

using GravParam =
    common::DimensionQuotient<common::Volume,
                              common::DimensionPower<common::Time, 2>>;
using SpecEnergy = common::DimensionPower<common::Speed, 2>;
using SpecAngMom = common::DimensionProduct<common::Length, common::Speed>;

struct OrbitOptions final {
  double tolerance = 1e-10;
};

struct KeplerOptions final {
  std::int64_t max_steps = 64;
  double tolerance = 0.0;
  std::int64_t check_every = 4;
  bool strict = true;
};

struct KeplerResult final {
  Series anomaly;
  Series residual;
  Series converged;
  std::int64_t steps;
};

class OrbitState final {
public:
  [[nodiscard]] const common::SeriesQuantity<common::Length> &
  position() const noexcept {
    return position_;
  }

  [[nodiscard]] const common::SeriesQuantity<common::Speed> &
  velocity() const noexcept {
    return velocity_;
  }

  [[nodiscard]] std::vector<std::int64_t> batch_shape() const {
    return {position_.si_value().sizes().begin(),
            position_.si_value().sizes().end() - 1};
  }

private:
  OrbitState(common::SeriesQuantity<common::Length> position,
             common::SeriesQuantity<common::Speed> velocity)
      : position_(std::move(position)), velocity_(std::move(velocity)) {}

  common::SeriesQuantity<common::Length> position_;
  common::SeriesQuantity<common::Speed> velocity_;

  friend OrbitState make_state(const common::SeriesQuantity<common::Length> &,
                               const common::SeriesQuantity<common::Speed> &);
};

struct StatePropResult final {
  OrbitState state;
  Series root;
  Series residual;
  Series converged;
  std::int64_t steps;
};

enum class OrbitSense : std::uint8_t { Prograde, Retrograde };

struct LambertOptions final {
  OrbitSense sense = OrbitSense::Prograde;
  std::int64_t max_steps = 15;
  double tolerance = 0.0;
  std::int64_t check_every = 1;
  bool strict = true;
};

struct LambertResult final {
  common::SeriesQuantity<common::Speed> depart;
  common::SeriesQuantity<common::Speed> arrive;
  Series root;
  Series residual;
  Series converged;
  std::int64_t steps;
};

class Elements final {
public:
  [[nodiscard]] const common::SeriesQuantity<common::Length> &
  axis() const noexcept {
    return axis_;
  }

  [[nodiscard]] const common::SeriesQuantity<common::Dimensionless> &
  ecc() const noexcept {
    return ecc_;
  }

  [[nodiscard]] const common::SeriesQuantity<common::Dimensionless> &
  tilt() const noexcept {
    return tilt_;
  }

  [[nodiscard]] const common::SeriesQuantity<common::Dimensionless> &
  node() const noexcept {
    return node_;
  }

  [[nodiscard]] const common::SeriesQuantity<common::Dimensionless> &
  peri() const noexcept {
    return peri_;
  }

  [[nodiscard]] const common::SeriesQuantity<common::Dimensionless> &
  phase() const noexcept {
    return phase_;
  }

  [[nodiscard]] std::vector<std::int64_t> batch_shape() const {
    return {axis_.si_value().sizes().begin(), axis_.si_value().sizes().end()};
  }

private:
  Elements(common::SeriesQuantity<common::Length> axis,
           common::SeriesQuantity<common::Dimensionless> ecc,
           common::SeriesQuantity<common::Dimensionless> tilt,
           common::SeriesQuantity<common::Dimensionless> node,
           common::SeriesQuantity<common::Dimensionless> peri,
           common::SeriesQuantity<common::Dimensionless> phase)
      : axis_(std::move(axis)), ecc_(std::move(ecc)), tilt_(std::move(tilt)),
        node_(std::move(node)), peri_(std::move(peri)),
        phase_(std::move(phase)) {}

  common::SeriesQuantity<common::Length> axis_;
  common::SeriesQuantity<common::Dimensionless> ecc_;
  common::SeriesQuantity<common::Dimensionless> tilt_;
  common::SeriesQuantity<common::Dimensionless> node_;
  common::SeriesQuantity<common::Dimensionless> peri_;
  common::SeriesQuantity<common::Dimensionless> phase_;

  friend Elements
  make_elem(const common::SeriesQuantity<common::Length> &,
            const common::SeriesQuantity<common::Dimensionless> &,
            const common::SeriesQuantity<common::Dimensionless> &,
            const common::SeriesQuantity<common::Dimensionless> &,
            const common::SeriesQuantity<common::Dimensionless> &,
            const common::SeriesQuantity<common::Dimensionless> &,
            const OrbitOptions &);
};

namespace detail {

inline bool same_shape(const Series &lhs, const Series &rhs) {
  return lhs.sizes() == rhs.sizes();
}

inline void require_real(const Series &value, const char *operation,
                         const char *role) {
  if (!value.defined())
    throw std::invalid_argument(std::string(operation) + ": " + role +
                                " must be defined");
  if (!value.dtype().is_floating())
    throw std::invalid_argument(std::string(operation) + ": " + role +
                                " must have a real floating dtype");
  if (!value.isfinite().all().item<bool>())
    throw std::invalid_argument(std::string(operation) + ": " + role +
                                " must be finite");
}

inline void require_options(const OrbitOptions &options,
                            const char *operation) {
  if (!(options.tolerance > 0.0) || options.tolerance >= 1.0 ||
      !std::isfinite(options.tolerance))
    throw std::invalid_argument(std::string(operation) +
                                ": tolerance must be finite and in (0, 1)");
}

inline Series stack3(const Series &x, const Series &y, const Series &z) {
  return x.unsqueeze(-1).unite({y.unsqueeze(-1), z.unsqueeze(-1)}, -1);
}

inline Series dot(const Series &lhs, const Series &rhs) {
  return lhs.mul(rhs).sum(-1, false);
}

inline Series wrap(const Series &angle) {
  constexpr double tau = 2.0 * std::numbers::pi;
  const Series reduced = angle.remainder(tau);
  return reduced.lt(0.0).where(reduced.add(tau), reduced);
}

inline Series safe_den(const Series &value, const Series &valid) {
  return valid.where(value, pluss::series::ones_like(value));
}

inline Series cube(const Series &value) { return value.square().mul(value); }

struct StumpffPair final {
  Series c;
  Series s;
};

inline StumpffPair stumpff(const Series &z) {
  const Series one = pluss::series::ones_like(z);
  const Series z2 = z.square();
  const Series z3 = z2.mul(z);
  const Series z4 = z2.square();
  const Series z5 = z4.mul(z);
  const Series c_series = one.mul(0.5)
                              .sub(z.mul(1.0 / 24.0))
                              .add(z2.mul(1.0 / 720.0))
                              .sub(z3.mul(1.0 / 40320.0))
                              .add(z4.mul(1.0 / 3628800.0))
                              .sub(z5.mul(1.0 / 479001600.0));
  const Series s_series = one.mul(1.0 / 6.0)
                              .sub(z.mul(1.0 / 120.0))
                              .add(z2.mul(1.0 / 5040.0))
                              .sub(z3.mul(1.0 / 362880.0))
                              .add(z4.mul(1.0 / 39916800.0))
                              .sub(z5.mul(1.0 / 6227020800.0));

  const Series positive = z.gt(0.1);
  const Series negative = z.lt(-0.1);
  const Series pos_z = positive.where(z, one);
  const Series pos_root = pos_z.sqrt();
  const Series c_pos = pos_root.mul(0.5).sin().square().mul(2.0).div(pos_z);
  const Series s_pos = pos_root.sub(pos_root.sin()).div(cube(pos_root));

  const Series neg_z = negative.where(z.neg(), one);
  const Series neg_root = neg_z.sqrt();
  const Series c_neg = neg_root.mul(0.5).sinh().square().mul(2.0).div(neg_z);
  const Series s_neg = neg_root.sinh().sub(neg_root).div(cube(neg_root));

  return {positive.where(c_pos, negative.where(c_neg, c_series)),
          positive.where(s_pos, negative.where(s_neg, s_series))};
}

inline Series lambert_hypergeom(const Series &argument) {
  Series term = pluss::series::ones_like(argument);
  Series sum = term;
  for (std::int64_t j = 0; j < 32; ++j) {
    const double order = static_cast<double>(j);
    term = term.mul(argument).mul((order + 3.0) / (order + 2.5));
    sum = sum.add(term);
  }
  return sum;
}

inline Series lambert_time(const Series &x, const Series &lambda) {
  const Series one = pluss::series::ones_like(x);
  const Series lambda2 = lambda.square();
  const Series lambda3 = lambda2.mul(lambda);
  const Series e = x.square().sub(1.0);
  const Series distance = x.sub(1.0).abs();
  const Series battin = distance.lt(0.01);
  const Series lagrange = distance.lt(0.2).logical_and(battin.logical_not());
  const Series lancaster = battin.logical_or(lagrange).logical_not();

  const Series z = one.add(lambda2.mul(e)).clamp_min(0.0).sqrt();
  const Series eta = z.sub(lambda.mul(x));
  const Series series_argument = battin.where(
      one.sub(lambda).sub(x.mul(eta)).mul(0.5),
      pluss::series::zeros_like(x));
  const Series q = lambert_hypergeom(series_argument).mul(4.0 / 3.0);
  const Series battin_time =
      detail::cube(eta).mul(q).add(lambda.mul(eta).mul(4.0)).mul(0.5);

  const Series safe_e = lancaster.where(e, one);
  const Series rho_root = safe_e.abs().sqrt();
  const Series g = x.mul(z).sub(lambda.mul(e));
  const Series ellipse = e.lt(0.0);
  const Series ellipse_d = g.clamp(-1.0, 1.0).acos();
  const Series log_argument =
      rho_root.mul(z.sub(lambda.mul(x))).add(g).clamp_min(1e-30);
  const Series hyper_d = log_argument.log();
  const Series d = ellipse.where(ellipse_d, hyper_d);
  const Series lancaster_time =
      x.sub(lambda.mul(z)).sub(d.div(rho_root)).div(safe_e);

  const Series safe_a_den = lagrange.where(one.sub(x.square()), one);
  const Series a = safe_a_den.reciprocal();
  const Series ellipse_a = a.gt(0.0);
  const Series ellipse_x = ellipse_a.where(x.clamp(-1.0, 1.0), one);
  const Series hyper_x = ellipse_a.where(one, x.clamp_min(1.0));
  const Series alpha_ellipse = ellipse_x.acos().mul(2.0);
  const Series alpha_hyper = hyper_x.acosh().mul(2.0);
  const Series ellipse_beta_arg =
      lambda2.div(ellipse_a.where(a, one)).clamp(0.0, 1.0).sqrt();
  const Series hyper_beta_arg =
      lambda2.neg().div(ellipse_a.where(one, a)).clamp_min(0.0).sqrt();
  const Series beta_sign = lambda.ge(0.0).where(one, one.neg());
  const Series beta_ellipse =
      ellipse_beta_arg.asin().mul(2.0).mul(beta_sign);
  const Series beta_hyper =
      hyper_beta_arg.asinh().mul(2.0).mul(beta_sign);
  const Series ellipse_time =
      a.mul(a.clamp_min(0.0).sqrt())
          .mul(alpha_ellipse.sub(alpha_ellipse.sin())
                   .sub(beta_ellipse.sub(beta_ellipse.sin())))
          .mul(0.5);
  const Series hyper_time =
      a.neg()
          .mul(a.neg().clamp_min(0.0).sqrt())
          .mul(beta_hyper.sub(beta_hyper.sinh())
                   .sub(alpha_hyper.sub(alpha_hyper.sinh())))
          .mul(0.5);
  const Series lagrange_time = ellipse_a.where(ellipse_time, hyper_time);

  return battin.where(battin_time,
                      lagrange.where(lagrange_time, lancaster_time));
}

inline pluss::subsp::math::roots::Derivatives
lambert_time_derivatives(const Series &x, const Series &lambda) {
  const Series time = lambert_time(x, lambda);
  const Series one = pluss::series::ones_like(x);
  const Series lambda2 = lambda.square();
  const Series lambda3 = lambda2.mul(lambda);
  const Series one_minus_x2 = one.sub(x.square());
  const Series safe_den = one_minus_x2.abs().lt(1e-12).where(
      one_minus_x2.ge(0.0).where(one.mul(1e-12), one.mul(-1e-12)),
      one_minus_x2);
  const Series y = one.sub(lambda2.mul(one_minus_x2)).clamp_min(0.0).sqrt();
  const Series first = time.mul(x).mul(3.0)
                           .sub(2.0)
                           .add(lambda3.mul(x).mul(2.0).div(y))
                           .div(safe_den);
  const Series second =
      time.mul(3.0)
          .add(x.mul(first).mul(5.0))
          .add(one.sub(lambda2).mul(lambda3).mul(2.0).div(detail::cube(y)))
          .div(safe_den);
  const Series third =
      x.mul(second)
          .mul(7.0)
          .add(first.mul(8.0))
          .sub(one.sub(lambda2)
                   .mul(lambda2)
                   .mul(lambda3)
                   .mul(x)
                   .mul(6.0)
                   .div(detail::cube(y).mul(y.square())))
          .div(safe_den);
  return {time, first, second, third};
}

inline double lambert_tolerance(const Series &like,
                                const LambertOptions &options) {
  if (options.max_steps <= 0)
    throw std::invalid_argument(
        "natsci::astro::lambert: max_steps must be positive");
  if (options.check_every <= 0)
    throw std::invalid_argument(
        "natsci::astro::lambert: check_every must be positive");
  if (options.tolerance < 0.0 || !std::isfinite(options.tolerance))
    throw std::invalid_argument(
        "natsci::astro::lambert: tolerance must be finite and non-negative");
  if (options.tolerance > 0.0)
    return options.tolerance;
  switch (like.scalar_type()) {
  case ScalarType::Float64:
    return 1e-12;
  case ScalarType::Float32:
    return 2e-5;
  case ScalarType::Float16:
    return 2e-2;
  case ScalarType::BFloat16:
    return 5e-2;
  default:
    throw std::invalid_argument(
        "natsci::astro::lambert: expected a real floating dtype");
  }
}

inline void require_same_layout(const Series &first, const Series &value,
                                const char *operation, const char *role) {
  if (!same_shape(first, value))
    throw std::invalid_argument(std::string(operation) + ": " + role +
                                " shape must match the other element fields");
  if (first.scalar_type() != value.scalar_type())
    throw std::invalid_argument(
        std::string(operation) + ": " + role +
        " dtype must match the other element fields (expected dtype id " +
        std::to_string(static_cast<int>(first.scalar_type())) + ", received " +
        std::to_string(static_cast<int>(value.scalar_type())) + ")");
  if (first.platform() != value.platform())
    throw std::invalid_argument(
        std::string(operation) + ": " + role +
        " platform must match the other element fields");
}

template <common::QuantityRep Rep>
inline Series
mu_on(const common::Quantity<GravParam, Rep> &mu, const Series &like,
      const std::vector<std::int64_t> &batch_shape, const char *operation) {
  if constexpr (std::same_as<std::remove_cvref_t<Rep>, double>) {
    if (!(mu.si_value() > 0.0) || !std::isfinite(mu.si_value()))
      throw std::invalid_argument(std::string(operation) +
                                  ": gravitational parameter must be finite "
                                  "and positive");
    return pluss::series::full(
        batch_shape, mu.si_value(),
        Config{}.with_dtype(like.scalar_type()).with_platform(like.platform()));
  } else {
    const Series &raw = mu.si_value();
    require_real(raw, operation, "gravitational parameter");
    if (raw.scalar_type() != like.scalar_type())
      throw std::invalid_argument(
          std::string(operation) +
          ": gravitational parameter dtype must match the orbit");
    if (raw.platform() != like.platform())
      throw std::invalid_argument(
          std::string(operation) +
          ": gravitational parameter platform must match the orbit");
    Series aligned = pluss::series::undefined();
    try {
      aligned = raw.expand(batch_shape);
    } catch (const std::exception &) {
      throw std::invalid_argument(
          std::string(operation) +
          ": gravitational parameter must broadcast to the orbit batch shape");
    }
    if (!aligned.gt(0.0).all().item<bool>())
      throw std::invalid_argument(std::string(operation) +
                                  ": gravitational parameter must be positive");
    return aligned;
  }
}

inline void require_bound(const Elements &elements, const char *operation) {
  if (!elements.ecc().si_value().lt(1.0).all().item<bool>())
    throw std::domain_error(std::string(operation) +
                            ": every orbit must be elliptic");
}

inline double solve_tolerance(const Series &like, const KeplerOptions &options,
                              const char *operation) {
  if (options.max_steps <= 0)
    throw std::invalid_argument(std::string(operation) +
                                ": max_steps must be positive");
  if (options.check_every <= 0)
    throw std::invalid_argument(std::string(operation) +
                                ": check_every must be positive");
  if (options.tolerance < 0.0 || !std::isfinite(options.tolerance))
    throw std::invalid_argument(std::string(operation) +
                                ": tolerance must be finite and non-negative");
  if (options.tolerance > 0.0)
    return options.tolerance;
  switch (like.scalar_type()) {
  case ScalarType::Float64:
    return 1e-13;
  case ScalarType::Float32:
    return 2e-6;
  case ScalarType::Float16:
    return 2e-3;
  case ScalarType::BFloat16:
    return 1e-2;
  default:
    throw std::invalid_argument(std::string(operation) +
                                ": expected a real floating dtype");
  }
}

inline void require_anomaly_pair(const Series &ecc, const Series &anomaly,
                                 const char *operation) {
  require_real(ecc, operation, "eccentricity");
  require_real(anomaly, operation, "anomaly");
  require_same_layout(ecc, anomaly, operation, "anomaly");
  if (!ecc.ge(0.0).all().item<bool>())
    throw std::invalid_argument(std::string(operation) +
                                ": eccentricity must be non-negative");
  if (!ecc.ne(1.0).all().item<bool>())
    throw std::domain_error(std::string(operation) +
                            ": parabolic anomalies require a universal "
                            "variable representation");
}

template <common::QuantityRep Rep>
inline Series
time_on(const common::Quantity<common::Time, Rep> &time, const Series &like,
        const std::vector<std::int64_t> &batch_shape, const char *operation) {
  if constexpr (std::same_as<std::remove_cvref_t<Rep>, double>) {
    if (!std::isfinite(time.si_value()))
      throw std::invalid_argument(std::string(operation) +
                                  ": elapsed time must be finite");
    return pluss::series::full(
        batch_shape, time.si_value(),
        Config{}.with_dtype(like.scalar_type()).with_platform(like.platform()));
  } else {
    const Series &raw = time.si_value();
    require_real(raw, operation, "elapsed time");
    if (raw.scalar_type() != like.scalar_type())
      throw std::invalid_argument(std::string(operation) +
                                  ": elapsed time dtype must match the orbit");
    if (raw.platform() != like.platform())
      throw std::invalid_argument(
          std::string(operation) +
          ": elapsed time platform must match the orbit");
    Series aligned = pluss::series::undefined();
    try {
      aligned = raw.expand(batch_shape);
    } catch (const std::exception &) {
      throw std::invalid_argument(
          std::string(operation) +
          ": elapsed time must broadcast to the orbit batch shape");
    }
    return aligned;
  }
}

} // namespace detail

[[nodiscard]] inline OrbitState
make_state(const common::SeriesQuantity<common::Length> &position,
           const common::SeriesQuantity<common::Speed> &velocity) {
  const Series &r = position.si_value();
  const Series &v = velocity.si_value();
  detail::require_real(r, "natsci::astro::make_state", "position");
  detail::require_real(v, "natsci::astro::make_state", "velocity");
  if (r.dim() < 1 || r.size(-1) != 3)
    throw std::invalid_argument(
        "natsci::astro::make_state: position must have shape [..., 3]");
  if (!detail::same_shape(r, v))
    throw std::invalid_argument(
        "natsci::astro::make_state: position and velocity shapes must match");
  if (r.scalar_type() != v.scalar_type())
    throw std::invalid_argument(
        "natsci::astro::make_state: position and velocity dtypes must match");
  if (r.platform() != v.platform())
    throw std::invalid_argument("natsci::astro::make_state: position and "
                                "velocity platforms must match");
  return OrbitState(position, velocity);
}

[[nodiscard]] inline Elements
make_elem(const common::SeriesQuantity<common::Length> &axis,
          const common::SeriesQuantity<common::Dimensionless> &ecc,
          const common::SeriesQuantity<common::Dimensionless> &tilt,
          const common::SeriesQuantity<common::Dimensionless> &node,
          const common::SeriesQuantity<common::Dimensionless> &peri,
          const common::SeriesQuantity<common::Dimensionless> &phase,
          const OrbitOptions &options = {}) {
  constexpr const char *op = "natsci::astro::make_elem";
  detail::require_options(options, op);
  const Series &a = axis.si_value();
  const Series &e = ecc.si_value();
  const Series &inc = tilt.si_value();
  const Series &raan = node.si_value();
  const Series &argp = peri.si_value();
  const Series &nu = phase.si_value();
  detail::require_real(a, op, "axis");
  detail::require_real(e, op, "eccentricity");
  detail::require_real(inc, op, "tilt");
  detail::require_real(raan, op, "node");
  detail::require_real(argp, op, "periapsis angle");
  detail::require_real(nu, op, "phase");
  detail::require_same_layout(a, e, op, "eccentricity");
  detail::require_same_layout(a, inc, op, "tilt");
  detail::require_same_layout(a, raan, op, "node");
  detail::require_same_layout(a, argp, op, "periapsis angle");
  detail::require_same_layout(a, nu, op, "phase");

  if (!e.ge(0.0).all().item<bool>())
    throw std::invalid_argument(
        "natsci::astro::make_elem: eccentricity must be non-negative");
  if (!e.sub(1.0).abs().gt(options.tolerance).all().item<bool>())
    throw std::domain_error(
        "natsci::astro::make_elem: parabolic orbits are singular in this "
        "element set");
  const Series valid_axis = e.lt(1.0).logical_and(a.gt(0.0)).logical_or(
      e.gt(1.0).logical_and(a.lt(0.0)));
  if (!valid_axis.all().item<bool>())
    throw std::invalid_argument(
        "natsci::astro::make_elem: elliptic orbits require a positive axis "
        "and hyperbolic orbits require a negative axis");
  if (!inc.ge(0.0).logical_and(inc.le(std::numbers::pi)).all().item<bool>())
    throw std::invalid_argument(
        "natsci::astro::make_elem: tilt must lie in [0, pi]");

  return Elements(
      axis, ecc, tilt,
      common::SeriesQuantity<common::Dimensionless>::from_si(
          detail::wrap(raan)),
      common::SeriesQuantity<common::Dimensionless>::from_si(
          detail::wrap(argp)),
      common::SeriesQuantity<common::Dimensionless>::from_si(detail::wrap(nu)));
}

[[nodiscard]] inline common::SeriesQuantity<SpecAngMom>
ang_mom(const OrbitState &state) {
  return common::SeriesQuantity<SpecAngMom>::from_si(
      state.position().si_value().cross(state.velocity().si_value(), -1));
}

template <common::QuantityRep Rep>
[[nodiscard]] inline common::SeriesQuantity<SpecEnergy>
energy(const OrbitState &state, const common::Quantity<GravParam, Rep> &mu) {
  const Series &r = state.position().si_value();
  const Series &v = state.velocity().si_value();
  const Series radius = r.norm(2.0, -1, false);
  if (!radius.gt(0.0).all().item<bool>())
    throw std::domain_error(
        "natsci::astro::energy: position must not coincide with the primary");
  const Series gm =
      detail::mu_on(mu, r, state.batch_shape(), "natsci::astro::energy");
  return common::SeriesQuantity<SpecEnergy>::from_si(
      v.square().sum(-1, false).mul(0.5).sub(gm.div(radius)));
}

template <common::QuantityRep Rep>
[[nodiscard]] inline common::SeriesQuantity<common::Dimensionless>
ecc_vec(const OrbitState &state, const common::Quantity<GravParam, Rep> &mu) {
  const Series &r = state.position().si_value();
  const Series &v = state.velocity().si_value();
  const Series radius = r.norm(2.0, -1, false);
  if (!radius.gt(0.0).all().item<bool>())
    throw std::domain_error(
        "natsci::astro::ecc_vec: position must not coincide with the primary");
  const Series gm =
      detail::mu_on(mu, r, state.batch_shape(), "natsci::astro::ecc_vec");
  const Series speed2 = v.square().sum(-1, false);
  const Series radial = detail::dot(r, v);
  return common::SeriesQuantity<common::Dimensionless>::from_si(
      r.mul(speed2.sub(gm.div(radius)).unsqueeze(-1))
          .sub(v.mul(radial.unsqueeze(-1)))
          .div(gm.unsqueeze(-1)));
}

template <common::QuantityRep Rep>
[[nodiscard]] inline Elements
to_elem(const OrbitState &state, const common::Quantity<GravParam, Rep> &mu,
        const OrbitOptions &options = {}) {
  constexpr const char *op = "natsci::astro::to_elem";
  detail::require_options(options, op);
  const Series &r = state.position().si_value();
  const Series &v = state.velocity().si_value();
  const Series radius = r.norm(2.0, -1, false);
  if (!radius.gt(0.0).all().item<bool>())
    throw std::domain_error(
        "natsci::astro::to_elem: position must not coincide with the primary");
  const Series gm = detail::mu_on(mu, r, state.batch_shape(), op);
  const Series h = r.cross(v, -1);
  const Series hmag = h.norm(2.0, -1, false);
  if (!hmag.gt(0.0).all().item<bool>())
    throw std::domain_error(
        "natsci::astro::to_elem: radial trajectories have no orbital plane");

  const Series speed2 = v.square().sum(-1, false);
  const Series specific = speed2.mul(0.5).sub(gm.div(radius));
  const Series axis = gm.neg().div(specific.mul(2.0));
  const Series radial = detail::dot(r, v);
  const Series evec = r.mul(speed2.sub(gm.div(radius)).unsqueeze(-1))
                          .sub(v.mul(radial.unsqueeze(-1)))
                          .div(gm.unsqueeze(-1));
  const Series ecc = evec.norm(2.0, -1, false);
  if (!ecc.sub(1.0).abs().gt(options.tolerance).all().item<bool>())
    throw std::domain_error(
        "natsci::astro::to_elem: parabolic orbits are singular in this "
        "element set");

  const Series hx = h.select(-1, 0);
  const Series hy = h.select(-1, 1);
  const Series hz = h.select(-1, 2);
  const Series zero = pluss::series::zeros_like(hx);
  const Series nvec = detail::stack3(hy.neg(), hx, zero);
  const Series nmag = nvec.norm(2.0, -1, false);
  const Series inclined = nmag.gt(hmag.mul(options.tolerance));
  const Series eccentric = ecc.gt(options.tolerance);
  const Series safe_h = detail::safe_den(hmag, hmag.gt(0.0));
  const Series tilt = hz.div(safe_h).clamp(-1.0, 1.0).acos();

  const Series node = detail::wrap(hx.atan2(hy.neg()));
  const Series safe_n = detail::safe_den(nmag, inclined);
  const Series safe_e = detail::safe_den(ecc, eccentric);
  const Series peri_y =
      detail::dot(nvec.cross(evec, -1), h).div(safe_n.mul(safe_e).mul(safe_h));
  const Series peri_x = detail::dot(nvec, evec).div(safe_n.mul(safe_e));
  const Series general_peri = detail::wrap(peri_y.atan2(peri_x));

  const Series phase_y =
      detail::dot(evec.cross(r, -1), h).div(safe_e.mul(radius).mul(safe_h));
  const Series phase_x = detail::dot(evec, r).div(safe_e.mul(radius));
  const Series general_phase = detail::wrap(phase_y.atan2(phase_x));

  const Series circle_y =
      detail::dot(nvec.cross(r, -1), h).div(safe_n.mul(radius).mul(safe_h));
  const Series circle_x = detail::dot(nvec, r).div(safe_n.mul(radius));
  const Series circle_phase = detail::wrap(circle_y.atan2(circle_x));

  const Series orient = hz.ge(0.0).where(pluss::series::ones_like(hz),
                                         pluss::series::full_like(hz, -1.0));
  const Series flat_peri =
      detail::wrap(evec.select(-1, 1).mul(orient).atan2(evec.select(-1, 0)));
  const Series flat_phase =
      detail::wrap(r.select(-1, 1).mul(orient).atan2(r.select(-1, 0)));

  const Series out_node = inclined.where(node, zero);
  const Series out_peri =
      eccentric.where(inclined.where(general_peri, flat_peri), zero);
  const Series out_phase =
      eccentric.where(general_phase, inclined.where(circle_phase, flat_phase));

  return make_elem(
      common::SeriesQuantity<common::Length>::from_si(axis),
      common::SeriesQuantity<common::Dimensionless>::from_si(ecc),
      common::SeriesQuantity<common::Dimensionless>::from_si(tilt),
      common::SeriesQuantity<common::Dimensionless>::from_si(out_node),
      common::SeriesQuantity<common::Dimensionless>::from_si(out_peri),
      common::SeriesQuantity<common::Dimensionless>::from_si(out_phase),
      options);
}

template <common::QuantityRep Rep>
[[nodiscard]] inline OrbitState
from_elem(const Elements &elements,
          const common::Quantity<GravParam, Rep> &mu) {
  const Series &a = elements.axis().si_value();
  const Series &e = elements.ecc().si_value();
  const Series &inc = elements.tilt().si_value();
  const Series &raan = elements.node().si_value();
  const Series &argp = elements.peri().si_value();
  const Series &nu = elements.phase().si_value();
  const Series gm =
      detail::mu_on(mu, a, elements.batch_shape(), "natsci::astro::from_elem");
  const Series p = a.mul(pluss::series::ones_like(e).sub(e.square()));
  if (!p.gt(0.0).all().item<bool>())
    throw std::domain_error(
        "natsci::astro::from_elem: semilatus rectum must be positive");
  const Series denom = pluss::series::ones_like(e).add(e.mul(nu.cos()));
  if (!denom.gt(0.0).all().item<bool>())
    throw std::domain_error(
        "natsci::astro::from_elem: phase lies beyond a hyperbolic asymptote");

  const Series cO = raan.cos();
  const Series sO = raan.sin();
  const Series co = argp.cos();
  const Series so = argp.sin();
  const Series ci = inc.cos();
  const Series si = inc.sin();
  const Series cf = nu.cos();
  const Series sf = nu.sin();

  const Series px = cO.mul(co).sub(sO.mul(so).mul(ci));
  const Series py = sO.mul(co).add(cO.mul(so).mul(ci));
  const Series pz = so.mul(si);
  const Series qx = cO.mul(so).neg().sub(sO.mul(co).mul(ci));
  const Series qy = sO.mul(so).neg().add(cO.mul(co).mul(ci));
  const Series qz = co.mul(si);
  const Series p_hat = detail::stack3(px, py, pz);
  const Series q_hat = detail::stack3(qx, qy, qz);

  const Series radius = p.div(denom);
  const Series position = p_hat.mul(cf.unsqueeze(-1))
                              .add(q_hat.mul(sf.unsqueeze(-1)))
                              .mul(radius.unsqueeze(-1));
  const Series scale = gm.div(p).sqrt();
  const Series velocity = p_hat.mul(sf.neg().unsqueeze(-1))
                              .add(q_hat.mul(e.add(cf).unsqueeze(-1)))
                              .mul(scale.unsqueeze(-1));
  return make_state(common::SeriesQuantity<common::Length>::from_si(position),
                    common::SeriesQuantity<common::Speed>::from_si(velocity));
}

[[nodiscard]] inline common::SeriesQuantity<common::Length>
peri(const Elements &elements) {
  return common::SeriesQuantity<common::Length>::from_si(
      elements.axis().si_value().mul(
          pluss::series::ones_like(elements.ecc().si_value())
              .sub(elements.ecc().si_value())));
}

[[nodiscard]] inline common::SeriesQuantity<common::Length>
apo(const Elements &elements) {
  detail::require_bound(elements, "natsci::astro::apo");
  return common::SeriesQuantity<common::Length>::from_si(
      elements.axis().si_value().mul(
          pluss::series::ones_like(elements.ecc().si_value())
              .add(elements.ecc().si_value())));
}

template <common::QuantityRep Rep>
[[nodiscard]] inline common::SeriesQuantity<common::Time>
period(const Elements &elements, const common::Quantity<GravParam, Rep> &mu) {
  detail::require_bound(elements, "natsci::astro::period");
  const Series &a = elements.axis().si_value();
  const Series gm =
      detail::mu_on(mu, a, elements.batch_shape(), "natsci::astro::period");
  return common::SeriesQuantity<common::Time>::from_si(
      detail::cube(a).div(gm).sqrt().mul(2.0 * std::numbers::pi));
}

[[nodiscard]] inline Series to_mean(const Series &ecc, const Series &phase) {
  constexpr const char *op = "natsci::astro::to_mean";
  detail::require_anomaly_pair(ecc, phase, op);
  const Series elliptic = ecc.lt(1.0);
  const Series one = pluss::series::ones_like(ecc);
  const Series denom = one.add(ecc.mul(phase.cos()));
  if (!elliptic.logical_or(denom.gt(0.0)).all().item<bool>())
    throw std::domain_error(
        "natsci::astro::to_mean: phase lies beyond a hyperbolic asymptote");
  const Series safe_denom = detail::safe_den(denom, denom.ne(0.0));

  const Series sin_e = one.sub(ecc.square())
                           .clamp_min(0.0)
                           .sqrt()
                           .mul(phase.sin())
                           .div(safe_denom);
  const Series cos_e = ecc.add(phase.cos()).div(safe_denom);
  const Series e_anomaly = sin_e.atan2(cos_e);
  const Series ellipse_mean = detail::wrap(e_anomaly.sub(ecc.mul(sin_e)));

  const Series safe_hyper_e = elliptic.where(one, ecc);
  const Series sinh_h = safe_hyper_e.square()
                            .sub(1.0)
                            .clamp_min(0.0)
                            .sqrt()
                            .mul(phase.sin())
                            .div(safe_denom);
  const Series h_anomaly = sinh_h.asinh();
  const Series hyper_mean = ecc.mul(sinh_h).sub(h_anomaly);
  return elliptic.where(ellipse_mean, hyper_mean);
}

[[nodiscard]] inline Series to_true(const Series &ecc, const Series &anomaly) {
  constexpr const char *op = "natsci::astro::to_true";
  detail::require_anomaly_pair(ecc, anomaly, op);
  const Series elliptic = ecc.lt(1.0);
  const Series one = pluss::series::ones_like(ecc);
  const Series zero = pluss::series::zeros_like(anomaly);
  const Series ellipse_anomaly = elliptic.where(anomaly, zero);
  const Series hyper_anomaly = elliptic.where(zero, anomaly);
  const Series ellipse_y =
      one.sub(ecc.square()).clamp_min(0.0).sqrt().mul(ellipse_anomaly.sin());
  const Series ellipse_x = ellipse_anomaly.cos().sub(ecc);
  const Series hyper_y =
      ecc.square().sub(1.0).clamp_min(0.0).sqrt().mul(hyper_anomaly.sinh());
  const Series hyper_x = ecc.sub(hyper_anomaly.cosh());
  return detail::wrap(elliptic.where(ellipse_y, hyper_y)
                          .atan2(elliptic.where(ellipse_x, hyper_x)));
}

[[nodiscard]] inline KeplerResult
solve_mean(const Series &ecc, const Series &mean,
           const KeplerOptions &options = {}) {
  constexpr const char *op = "natsci::astro::solve_mean";
  detail::require_anomaly_pair(ecc, mean, op);
  const double tolerance = detail::solve_tolerance(mean, options, op);
  constexpr double tau = 2.0 * std::numbers::pi;
  const Series elliptic = ecc.lt(1.0);
  const Series wrapped = detail::wrap(mean);
  const Series centered =
      wrapped.gt(std::numbers::pi).where(wrapped.sub(tau), wrapped);
  const Series sign =
      centered.ge(0.0).where(pluss::series::ones_like(centered),
                             pluss::series::full_like(centered, -1.0));
  const Series ellipse_start = centered.add(sign.mul(ecc).mul(0.71));
  const Series safe_hyper_e =
      elliptic.where(pluss::series::ones_like(ecc), ecc);
  const Series hyper_start = mean.div(safe_hyper_e).asinh();
  const Series start = elliptic.where(ellipse_start, hyper_start);

  const auto function = [ecc, mean, centered, elliptic](const Series &value) {
    const Series ellipse = value.sub(ecc.mul(value.sin())).sub(centered);
    const Series hyper = ecc.mul(value.sinh()).sub(value).sub(mean);
    return elliptic.where(ellipse, hyper);
  };
  const auto derivative = [ecc, elliptic](const Series &value) {
    const Series ellipse =
        pluss::series::ones_like(value).sub(ecc.mul(value.cos()));
    const Series hyper = ecc.mul(value.cosh()).sub(1.0);
    return elliptic.where(ellipse, hyper);
  };
  pluss::subsp::math::roots::Options root_options;
  root_options.max_iterations = options.max_steps;
  root_options.x_tolerance = tolerance;
  root_options.check_every = options.check_every;
  const auto solved = pluss::subsp::math::roots::newton(
      function, start, derivative, root_options);
  const Series anomaly =
      elliptic.where(solved.root.add(mean.sub(centered)), solved.root);
  const Series residual = function(solved.root);
  const Series residual_reference = elliptic.where(centered.abs(), mean.abs());
  const Series scale = residual_reference.add(1.0).mul(tolerance);
  const Series converged = residual.abs().le(scale);
  if (options.strict && !converged.all().item<bool>())
    throw std::runtime_error(
        "natsci::astro::solve_mean: Kepler solve did not converge");
  return {anomaly, residual, converged, solved.iterations};
}

template <common::QuantityRep Rep>
[[nodiscard]] inline common::SeriesQuantity<common::Frequency>
mean_rate(const Elements &elements,
          const common::Quantity<GravParam, Rep> &mu) {
  const Series &axis = elements.axis().si_value();
  const Series gm = detail::mu_on(mu, axis, elements.batch_shape(),
                                  "natsci::astro::mean_rate");
  const Series magnitude = axis.abs();
  return common::SeriesQuantity<common::Frequency>::from_si(
      gm.div(detail::cube(magnitude)).sqrt());
}

struct PropResult final {
  Elements elements;
  KeplerResult solve;
};

template <common::QuantityRep MuRep, common::QuantityRep TimeRep>
[[nodiscard]] inline PropResult
advance(const Elements &elements,
        const common::Quantity<common::Time, TimeRep> &elapsed,
        const common::Quantity<GravParam, MuRep> &mu,
        const KeplerOptions &options = {}) {
  const Series &ecc = elements.ecc().si_value();
  const Series time = detail::time_on(elapsed, ecc, elements.batch_shape(),
                                      "natsci::astro::advance");
  const Series initial_mean = to_mean(ecc, elements.phase().si_value());
  detail::require_same_layout(ecc, initial_mean, "natsci::astro::advance",
                              "initial mean anomaly");
  const Series rate = mean_rate(elements, mu).si_value();
  detail::require_same_layout(ecc, rate, "natsci::astro::advance", "mean rate");
  const Series increment = rate.mul(time);
  detail::require_same_layout(ecc, increment, "natsci::astro::advance",
                              "mean increment");
  const Series mean = initial_mean.add(increment);
  KeplerResult solved = solve_mean(ecc, mean, options);
  Elements next =
      make_elem(elements.axis(), elements.ecc(), elements.tilt(),
                elements.node(), elements.peri(),
                common::SeriesQuantity<common::Dimensionless>::from_si(
                    to_true(ecc, solved.anomaly)));
  return {std::move(next), std::move(solved)};
}

template <common::QuantityRep MuRep, common::QuantityRep TimeRep>
[[nodiscard]] inline StatePropResult
advance(const OrbitState &state,
        const common::Quantity<common::Time, TimeRep> &elapsed,
        const common::Quantity<GravParam, MuRep> &mu,
        const KeplerOptions &options = {}) {
  constexpr const char *op = "natsci::astro::advance";
  const Series &r0_vec = state.position().si_value();
  const Series &v0_vec = state.velocity().si_value();
  const double tolerance = detail::solve_tolerance(r0_vec, options, op);
  const Series gm = detail::mu_on(mu, r0_vec, state.batch_shape(), op);
  const Series time = detail::time_on(elapsed, r0_vec, state.batch_shape(), op);

  const Series r0 = r0_vec.norm(2.0, -1, false);
  const Series v0 = v0_vec.norm(2.0, -1, false);
  if (!r0.gt(0.0).all().item<bool>())
    throw std::domain_error(
        "natsci::astro::advance: position must not coincide with the primary");
  if (!v0.gt(0.0).all().item<bool>())
    throw std::domain_error(
        "natsci::astro::advance: velocity must not be zero");
  if (!r0_vec.cross(v0_vec, -1).norm(2.0, -1, false).gt(0.0).all().item<bool>())
    throw std::domain_error("natsci::astro::advance: position and velocity "
                            "must define a conic plane");

  const Series speed2 = v0_vec.square().sum(-1, false);
  const Series radial = detail::dot(r0_vec, v0_vec);
  const Series sqrt_gm = gm.sqrt();
  const Series sqrt_r0 = r0.sqrt();
  const Series alpha = r0.reciprocal().mul(2.0).sub(speed2.div(gm));
  const Series scaled_alpha = alpha.mul(r0);
  const Series eta = radial.div(sqrt_gm.mul(sqrt_r0));
  const Series tau = sqrt_gm.mul(time).div(r0.mul(sqrt_r0));

  const Series one = pluss::series::ones_like(tau);
  const Series direction = tau.ge(0.0).where(one, one.neg());
  const Series hyper = scaled_alpha.lt(-0.05);
  const Series safe_neg_alpha = hyper.where(scaled_alpha.neg(), one);
  const Series hyper_den =
      eta.add(direction.mul(one.sub(scaled_alpha)).div(safe_neg_alpha.sqrt()));
  const Series hyper_arg =
      scaled_alpha.mul(tau).mul(-2.0).div(hyper.where(hyper_den, one));
  const Series usable_hyper = hyper.logical_and(hyper_arg.gt(0.0));
  const Series safe_arg = usable_hyper.where(hyper_arg, one);
  const Series hyper_start =
      direction.mul(safe_arg.log()).div(safe_neg_alpha.sqrt());
  const Series ellipse_start = scaled_alpha.mul(tau);
  const Series start = usable_hyper.where(
      hyper_start, scaled_alpha.gt(0.05).where(ellipse_start, tau));

  const auto function = [scaled_alpha, eta, tau](const Series &u) {
    const Series z = scaled_alpha.mul(u.square());
    const auto cs = detail::stumpff(z);
    return eta.mul(u.square())
        .mul(cs.c)
        .add(pluss::series::ones_like(u)
                 .sub(scaled_alpha)
                 .mul(detail::cube(u))
                 .mul(cs.s))
        .add(u)
        .sub(tau);
  };
  const auto derivative = [scaled_alpha, eta](const Series &u) {
    const Series z = scaled_alpha.mul(u.square());
    const auto cs = detail::stumpff(z);
    return eta.mul(u)
        .mul(pluss::series::ones_like(u).sub(z.mul(cs.s)))
        .add(pluss::series::ones_like(u)
                 .sub(scaled_alpha)
                 .mul(u.square())
                 .mul(cs.c))
        .add(1.0);
  };

  pluss::subsp::math::roots::Options root_options;
  root_options.max_iterations = options.max_steps;
  root_options.x_tolerance = tolerance;
  root_options.check_every = options.check_every;
  const auto solved = pluss::subsp::math::roots::newton(
      function, start, derivative, root_options);
  const Series residual = function(solved.root);
  const Series converged = residual.abs().le(tau.abs().add(1.0).mul(tolerance));
  if (options.strict && !converged.all().item<bool>())
    throw std::runtime_error(
        "natsci::astro::advance: universal-variable solve did not converge");

  const Series chi = solved.root.mul(sqrt_r0);
  const Series z = alpha.mul(chi.square());
  const auto cs = detail::stumpff(z);
  const Series f = one.sub(chi.square().mul(cs.c).div(r0));
  const Series g = time.sub(detail::cube(chi).mul(cs.s).div(sqrt_gm));
  const Series r_vec =
      r0_vec.mul(f.unsqueeze(-1)).add(v0_vec.mul(g.unsqueeze(-1)));
  const Series radius = r_vec.norm(2.0, -1, false);
  const Series f_rate =
      sqrt_gm.div(radius.mul(r0))
          .mul(alpha.mul(detail::cube(chi)).mul(cs.s).sub(chi));
  const Series g_rate = one.sub(chi.square().mul(cs.c).div(radius));
  const Series v_vec =
      r0_vec.mul(f_rate.unsqueeze(-1)).add(v0_vec.mul(g_rate.unsqueeze(-1)));

  return {make_state(common::SeriesQuantity<common::Length>::from_si(r_vec),
                     common::SeriesQuantity<common::Speed>::from_si(v_vec)),
          solved.root, residual, converged, solved.iterations};
}

template <common::QuantityRep MuRep, common::QuantityRep TimeRep>
[[nodiscard]] inline LambertResult lambert(
    const common::SeriesQuantity<common::Length> &start,
    const common::SeriesQuantity<common::Length> &finish,
    const common::Quantity<common::Time, TimeRep> &flight,
    const common::Quantity<GravParam, MuRep> &mu,
    const common::SeriesQuantity<common::Dimensionless> &normal,
    const LambertOptions &options = {}) {
  constexpr const char *op = "natsci::astro::lambert";
  const Series &r1_vec = start.si_value();
  const Series &r2_vec = finish.si_value();
  detail::require_real(r1_vec, op, "start position");
  detail::require_real(r2_vec, op, "finish position");
  if (r1_vec.dim() < 1 || r1_vec.size(-1) != 3)
    throw std::invalid_argument(
        "natsci::astro::lambert: positions must have shape [..., 3]");
  if (!detail::same_shape(r1_vec, r2_vec))
    throw std::invalid_argument(
        "natsci::astro::lambert: position shapes must match");
  if (r1_vec.scalar_type() != r2_vec.scalar_type())
    throw std::invalid_argument(
        "natsci::astro::lambert: position dtypes must match");
  if (r1_vec.platform() != r2_vec.platform())
    throw std::invalid_argument(
        "natsci::astro::lambert: position platforms must match");

  const std::vector<std::int64_t> batch_shape(
      r1_vec.sizes().begin(), r1_vec.sizes().end() - 1);
  const Series gm = detail::mu_on(mu, r1_vec, batch_shape, op);
  const Series time = detail::time_on(flight, r1_vec, batch_shape, op);
  if (!time.gt(0.0).all().item<bool>())
    throw std::invalid_argument(
        "natsci::astro::lambert: flight time must be positive");
  const double tolerance = detail::lambert_tolerance(r1_vec, options);

  const Series &normal_raw = normal.si_value();
  detail::require_real(normal_raw, op, "reference normal");
  if (normal_raw.scalar_type() != r1_vec.scalar_type())
    throw std::invalid_argument(
        "natsci::astro::lambert: reference normal dtype must match positions");
  if (normal_raw.platform() != r1_vec.platform())
    throw std::invalid_argument(
        "natsci::astro::lambert: reference normal platform must match positions");
  Series normal_vec = pluss::series::undefined();
  try {
    normal_vec = normal_raw.expand(r1_vec.sizes());
  } catch (const std::exception &) {
    throw std::invalid_argument(
        "natsci::astro::lambert: reference normal must broadcast to [..., 3]");
  }

  const Series r1 = r1_vec.norm(2.0, -1, false);
  const Series r2 = r2_vec.norm(2.0, -1, false);
  const Series normal_norm = normal_vec.norm(2.0, -1, false);
  if (!r1.gt(0.0).logical_and(r2.gt(0.0)).all().item<bool>())
    throw std::domain_error(
        "natsci::astro::lambert: neither position may coincide with the primary");
  if (!normal_norm.gt(0.0).all().item<bool>())
    throw std::domain_error(
        "natsci::astro::lambert: reference normal must not be zero");

  const Series ir1 = r1_vec.div(r1.unsqueeze(-1));
  const Series ir2 = r2_vec.div(r2.unsqueeze(-1));
  const Series plane = ir1.cross(ir2, -1);
  const Series plane_norm = plane.norm(2.0, -1, false);
  if (!plane_norm.gt(1e-12).all().item<bool>())
    throw std::domain_error(
        "natsci::astro::lambert: collinear endpoints do not define a unique transfer plane");
  const Series ih = plane.div(plane_norm.unsqueeze(-1));
  const Series unit_normal = normal_vec.div(normal_norm.unsqueeze(-1));
  const Series orientation = detail::dot(ih, unit_normal);
  if (!orientation.abs().gt(1e-12).all().item<bool>())
    throw std::domain_error(
        "natsci::astro::lambert: reference normal must distinguish the transfer sense");

  const Series chord = r2_vec.sub(r1_vec).norm(2.0, -1, false);
  const Series semiperimeter = chord.add(r1).add(r2).mul(0.5);
  const Series lambda2 =
      pluss::series::ones_like(chord).sub(chord.div(semiperimeter));
  Series lambda = lambda2.clamp(0.0, 1.0).sqrt();
  Series start_tangent = ih.cross(ir1, -1);
  Series finish_tangent = ih.cross(ir2, -1);
  const Series aligned = orientation.gt(0.0);
  lambda = aligned.where(lambda, lambda.neg());
  start_tangent = aligned.unsqueeze(-1).where(start_tangent,
                                                start_tangent.neg());
  finish_tangent = aligned.unsqueeze(-1).where(finish_tangent,
                                                 finish_tangent.neg());
  if (options.sense == OrbitSense::Retrograde) {
    lambda = lambda.neg();
    start_tangent = start_tangent.neg();
    finish_tangent = finish_tangent.neg();
  }

  const Series target =
      gm.mul(2.0).div(detail::cube(semiperimeter)).sqrt().mul(time);
  const Series lambda3 = detail::cube(lambda);
  const Series t00 = lambda.clamp(-1.0, 1.0)
                         .acos()
                         .add(lambda.mul(
                             pluss::series::ones_like(lambda)
                                 .sub(lambda2)
                                 .clamp_min(0.0)
                                 .sqrt()));
  const Series t1 =
      pluss::series::ones_like(lambda).sub(lambda3).mul(2.0 / 3.0);
  const Series high_time = target.ge(t00);
  const Series low_time = target.le(t1);
  const Series high_start = target.sub(t00).neg().div(
      target.sub(t00).add(4.0));
  const Series low_den = pluss::series::ones_like(lambda)
                             .sub(lambda2.mul(lambda3))
                             .mul(target)
                             .mul(0.4);
  const Series low_start =
      t1.mul(t1.sub(target)).div(low_den).add(1.0);
  const Series middle_exponent =
      t1.div(t00).log().reciprocal().mul(std::numbers::ln2);
  const Series middle_start =
      target.div(t00).log().mul(middle_exponent).exp().sub(1.0);
  Series root_start = high_time.where(
      high_start, low_time.where(low_start, middle_start));
  const double root_nudge =
      r1_vec.scalar_type() == ScalarType::Float64 ? 1e-8 : 1e-4;
  root_start = root_start.sub(1.0).abs().lt(root_nudge).where(
      root_start.add(root_nudge), root_start);

  const auto evaluate = [lambda, target](const Series &x) {
    auto derivatives = detail::lambert_time_derivatives(x, lambda);
    derivatives.value = derivatives.value.sub(target);
    return derivatives;
  };
  pluss::subsp::math::roots::Options root_options;
  root_options.max_iterations = options.max_steps;
  root_options.x_tolerance = tolerance;
  root_options.f_tolerance = tolerance;
  root_options.check_every = options.check_every;
  const auto solved = pluss::subsp::math::roots::householder(
      evaluate, root_start, root_options);
  const Series residual = solved.residual;
  const Series converged =
      residual.abs().le(target.abs().add(1.0).mul(tolerance));
  if (options.strict && !converged.all().item<bool>())
    throw std::runtime_error(
        "natsci::astro::lambert: transfer-time solve did not converge");

  const Series y = pluss::series::ones_like(lambda)
                       .sub(lambda2)
                       .add(lambda2.mul(solved.root.square()))
                       .clamp_min(0.0)
                       .sqrt();
  const Series gamma = gm.mul(semiperimeter).mul(0.5).sqrt();
  const Series rho = r1.sub(r2).div(chord);
  const Series sigma =
      pluss::series::ones_like(rho).sub(rho.square()).clamp_min(0.0).sqrt();
  const Series common_term = lambda.mul(y);
  const Series depart_radial =
      gamma.mul(common_term.sub(solved.root)
                    .sub(rho.mul(common_term.add(solved.root))))
          .div(r1);
  const Series arrive_radial =
      gamma.neg()
          .mul(common_term.sub(solved.root)
                   .add(rho.mul(common_term.add(solved.root))))
          .div(r2);
  const Series tangent =
      gamma.mul(sigma).mul(y.add(lambda.mul(solved.root)));
  const Series depart_tangent = tangent.div(r1);
  const Series arrive_tangent = tangent.div(r2);
  const Series depart =
      ir1.mul(depart_radial.unsqueeze(-1))
          .add(start_tangent.mul(depart_tangent.unsqueeze(-1)));
  const Series arrive =
      ir2.mul(arrive_radial.unsqueeze(-1))
          .add(finish_tangent.mul(arrive_tangent.unsqueeze(-1)));

  return {common::SeriesQuantity<common::Speed>::from_si(depart),
          common::SeriesQuantity<common::Speed>::from_si(arrive), solved.root,
          residual, converged, solved.iterations};
}

template <common::QuantityRep MuRep, common::QuantityRep TimeRep>
[[nodiscard]] inline LambertResult lambert(
    const common::SeriesQuantity<common::Length> &start,
    const common::SeriesQuantity<common::Length> &finish,
    const common::Quantity<common::Time, TimeRep> &flight,
    const common::Quantity<GravParam, MuRep> &mu,
    const LambertOptions &options = {}) {
  const Series &like = start.si_value();
  if (!like.defined() || like.dim() < 1 || like.size(-1) != 3)
    throw std::invalid_argument(
        "natsci::astro::lambert: positions must have shape [..., 3]");
  std::vector<std::int64_t> batch_shape(like.sizes().begin(),
                                        like.sizes().end() - 1);
  const Series zero = pluss::series::zeros(
      batch_shape, Config{}.with_dtype(like.scalar_type())
                       .with_platform(like.platform()));
  const Series normal =
      detail::stack3(zero, zero, pluss::series::ones_like(zero));
  return lambert(
      start, finish, flight, mu,
      common::SeriesQuantity<common::Dimensionless>::from_si(normal), options);
}

} // namespace pluss::subsp::natsci::physics::astro
