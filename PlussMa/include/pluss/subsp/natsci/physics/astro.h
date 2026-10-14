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

using GravParam = common::DimensionQuotient<
    common::Volume, common::DimensionPower<common::Time, 2>>;
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
  [[nodiscard]] const common::SeriesQuantity<common::Length> &position() const
      noexcept {
    return position_;
  }

  [[nodiscard]] const common::SeriesQuantity<common::Speed> &velocity() const
      noexcept {
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

  friend OrbitState
  make_state(const common::SeriesQuantity<common::Length> &,
             const common::SeriesQuantity<common::Speed> &);
};

class Elements final {
public:
  [[nodiscard]] const common::SeriesQuantity<common::Length> &axis() const
      noexcept {
    return axis_;
  }

  [[nodiscard]] const common::SeriesQuantity<common::Dimensionless> &ecc() const
      noexcept {
    return ecc_;
  }

  [[nodiscard]] const common::SeriesQuantity<common::Dimensionless> &tilt() const
      noexcept {
    return tilt_;
  }

  [[nodiscard]] const common::SeriesQuantity<common::Dimensionless> &node() const
      noexcept {
    return node_;
  }

  [[nodiscard]] const common::SeriesQuantity<common::Dimensionless> &peri() const
      noexcept {
    return peri_;
  }

  [[nodiscard]] const common::SeriesQuantity<common::Dimensionless> &phase() const
      noexcept {
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

inline Series cube(const Series &value) {
  return value.square().mul(value);
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
    throw std::invalid_argument(std::string(operation) + ": " + role +
                                " platform must match the other element fields");
}

template <common::QuantityRep Rep>
inline Series mu_on(const common::Quantity<GravParam, Rep> &mu,
                    const Series &like,
                    const std::vector<std::int64_t> &batch_shape,
                    const char *operation) {
  if constexpr (std::same_as<std::remove_cvref_t<Rep>, double>) {
    if (!(mu.si_value() > 0.0) || !std::isfinite(mu.si_value()))
      throw std::invalid_argument(std::string(operation) +
                                  ": gravitational parameter must be finite "
                                  "and positive");
    return pluss::series::full(batch_shape, mu.si_value(),
                               Config{}.with_dtype(like.scalar_type())
                                   .with_platform(like.platform()));
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

inline double solve_tolerance(const Series &like,
                              const KeplerOptions &options,
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
inline Series time_on(const common::Quantity<common::Time, Rep> &time,
                      const Series &like,
                      const std::vector<std::int64_t> &batch_shape,
                      const char *operation) {
  if constexpr (std::same_as<std::remove_cvref_t<Rep>, double>) {
    if (!std::isfinite(time.si_value()))
      throw std::invalid_argument(std::string(operation) +
                                  ": elapsed time must be finite");
    return pluss::series::full(batch_shape, time.si_value(),
                               Config{}.with_dtype(like.scalar_type())
                                   .with_platform(like.platform()));
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
    throw std::invalid_argument(
        "natsci::astro::make_state: position and velocity platforms must match");
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
  const Series valid_axis =
      e.lt(1.0).logical_and(a.gt(0.0)).logical_or(
          e.gt(1.0).logical_and(a.lt(0.0)));
  if (!valid_axis.all().item<bool>())
    throw std::invalid_argument(
        "natsci::astro::make_elem: elliptic orbits require a positive axis "
        "and hyperbolic orbits require a negative axis");
  if (!inc.ge(0.0).logical_and(inc.le(std::numbers::pi)).all().item<bool>())
    throw std::invalid_argument(
        "natsci::astro::make_elem: tilt must lie in [0, pi]");

  return Elements(axis, ecc, tilt,
                  common::SeriesQuantity<common::Dimensionless>::from_si(
                      detail::wrap(raan)),
                  common::SeriesQuantity<common::Dimensionless>::from_si(
                      detail::wrap(argp)),
                  common::SeriesQuantity<common::Dimensionless>::from_si(
                      detail::wrap(nu)));
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
  const Series gm = detail::mu_on(mu, r, state.batch_shape(),
                                  "natsci::astro::energy");
  return common::SeriesQuantity<SpecEnergy>::from_si(
      v.square().sum(-1, false).mul(0.5).sub(gm.div(radius)));
}

template <common::QuantityRep Rep>
[[nodiscard]] inline common::SeriesQuantity<common::Dimensionless>
ecc_vec(const OrbitState &state,
        const common::Quantity<GravParam, Rep> &mu) {
  const Series &r = state.position().si_value();
  const Series &v = state.velocity().si_value();
  const Series radius = r.norm(2.0, -1, false);
  if (!radius.gt(0.0).all().item<bool>())
    throw std::domain_error(
        "natsci::astro::ecc_vec: position must not coincide with the primary");
  const Series gm = detail::mu_on(mu, r, state.batch_shape(),
                                  "natsci::astro::ecc_vec");
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
  const Series evec =
      r.mul(speed2.sub(gm.div(radius)).unsqueeze(-1))
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
  const Series peri_y = detail::dot(nvec.cross(evec, -1), h).div(
      safe_n.mul(safe_e).mul(safe_h));
  const Series peri_x = detail::dot(nvec, evec).div(safe_n.mul(safe_e));
  const Series general_peri = detail::wrap(peri_y.atan2(peri_x));

  const Series phase_y = detail::dot(evec.cross(r, -1), h).div(
      safe_e.mul(radius).mul(safe_h));
  const Series phase_x = detail::dot(evec, r).div(safe_e.mul(radius));
  const Series general_phase = detail::wrap(phase_y.atan2(phase_x));

  const Series circle_y = detail::dot(nvec.cross(r, -1), h).div(
      safe_n.mul(radius).mul(safe_h));
  const Series circle_x = detail::dot(nvec, r).div(safe_n.mul(radius));
  const Series circle_phase = detail::wrap(circle_y.atan2(circle_x));

  const Series orient = hz.ge(0.0).where(pluss::series::ones_like(hz),
                                          pluss::series::full_like(hz, -1.0));
  const Series flat_peri =
      detail::wrap(evec.select(-1, 1).mul(orient).atan2(evec.select(-1, 0)));
  const Series flat_phase =
      detail::wrap(r.select(-1, 1).mul(orient).atan2(r.select(-1, 0)));

  const Series out_node = inclined.where(node, zero);
  const Series out_peri = eccentric.where(
      inclined.where(general_peri, flat_peri), zero);
  const Series out_phase = eccentric.where(
      general_phase, inclined.where(circle_phase, flat_phase));

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
  return make_state(
      common::SeriesQuantity<common::Length>::from_si(position),
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
period(const Elements &elements,
       const common::Quantity<GravParam, Rep> &mu) {
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

[[nodiscard]] inline Series to_true(const Series &ecc,
                                    const Series &anomaly) {
  constexpr const char *op = "natsci::astro::to_true";
  detail::require_anomaly_pair(ecc, anomaly, op);
  const Series elliptic = ecc.lt(1.0);
  const Series one = pluss::series::ones_like(ecc);
  const Series zero = pluss::series::zeros_like(anomaly);
  const Series ellipse_anomaly = elliptic.where(anomaly, zero);
  const Series hyper_anomaly = elliptic.where(zero, anomaly);
  const Series ellipse_y = one.sub(ecc.square())
                               .clamp_min(0.0)
                               .sqrt()
                               .mul(ellipse_anomaly.sin());
  const Series ellipse_x = ellipse_anomaly.cos().sub(ecc);
  const Series hyper_y = ecc.square()
                             .sub(1.0)
                             .clamp_min(0.0)
                             .sqrt()
                             .mul(hyper_anomaly.sinh());
  const Series hyper_x = ecc.sub(hyper_anomaly.cosh());
  return detail::wrap(
      elliptic.where(ellipse_y, hyper_y).atan2(
          elliptic.where(ellipse_x, hyper_x)));
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
  const Series sign = centered.ge(0.0).where(
      pluss::series::ones_like(centered),
      pluss::series::full_like(centered, -1.0));
  const Series ellipse_start = centered.add(sign.mul(ecc).mul(0.71));
  const Series safe_hyper_e = elliptic.where(pluss::series::ones_like(ecc), ecc);
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
  const Series anomaly = elliptic.where(
      solved.root.add(mean.sub(centered)), solved.root);
  const Series residual = function(solved.root);
  const Series residual_reference =
      elliptic.where(centered.abs(), mean.abs());
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
  detail::require_same_layout(ecc, rate, "natsci::astro::advance",
                              "mean rate");
  const Series increment = rate.mul(time);
  detail::require_same_layout(ecc, increment, "natsci::astro::advance",
                              "mean increment");
  const Series mean = initial_mean.add(increment);
  KeplerResult solved = solve_mean(ecc, mean, options);
  Elements next = make_elem(
      elements.axis(), elements.ecc(), elements.tilt(), elements.node(),
      elements.peri(),
      common::SeriesQuantity<common::Dimensionless>::from_si(
          to_true(ecc, solved.anomaly)));
  return {std::move(next), std::move(solved)};
}

} // namespace pluss::subsp::natsci::physics::astro
