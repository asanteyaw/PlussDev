#pragma once

#include "pluss/codem/series_factory.h"
#include "pluss/subsp/natsci/common/constants.h"
#include "pluss/subsp/natsci/common/quantity.h"

#include <cmath>
#include <concepts>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace pluss::subsp::natsci::physics::mechanics {

namespace common = ::pluss::subsp::natsci::common;

class Bodies final {
public:
  [[nodiscard]] const common::SeriesQuantity<common::Mass> &mass() const
      noexcept {
    return mass_;
  }

  [[nodiscard]] const common::SeriesQuantity<common::Length> &position() const
      noexcept {
    return position_;
  }

  [[nodiscard]] const common::SeriesQuantity<common::Speed> &velocity() const
      noexcept {
    return velocity_;
  }

  [[nodiscard]] std::int64_t count() const noexcept {
    return position_.si_value().size(-2);
  }

  [[nodiscard]] std::int64_t axes() const noexcept {
    return position_.si_value().size(-1);
  }

private:
  Bodies(common::SeriesQuantity<common::Mass> mass,
         common::SeriesQuantity<common::Length> position,
         common::SeriesQuantity<common::Speed> velocity)
      : mass_(std::move(mass)), position_(std::move(position)),
        velocity_(std::move(velocity)) {}

  common::SeriesQuantity<common::Mass> mass_;
  common::SeriesQuantity<common::Length> position_;
  common::SeriesQuantity<common::Speed> velocity_;

  friend Bodies make(const common::SeriesQuantity<common::Mass> &,
                     const common::SeriesQuantity<common::Length> &,
                     const common::SeriesQuantity<common::Speed> &);
};

namespace detail {

inline bool same_shape(const Series &lhs, const Series &rhs) {
  if (lhs.dim() != rhs.dim())
    return false;
  for (std::int64_t axis = 0; axis < lhs.dim(); ++axis)
    if (lhs.size(axis) != rhs.size(axis))
      return false;
  return true;
}

inline void require_real_float(const Series &value, const char *name) {
  if (!value.dtype().is_floating())
    throw std::invalid_argument(std::string("natsci::mechanics::make: ") +
                                name + " must have a real floating dtype");
  if (!value.isfinite().all().host<bool>().front())
    throw std::invalid_argument(std::string("natsci::mechanics::make: ") +
                                name + " must be finite");
}

inline Series body_mass(const Bodies &bodies) {
  return bodies.mass().si_value().unsqueeze(-1);
}

inline Series pair_identity(const Bodies &bodies) {
  const Series &position = bodies.position().si_value();
  return pluss::series::eye(bodies.count(),
                            Config{}.with_dtype(position.scalar_type())
                                .with_platform(position.platform()));
}

struct PairGeometry final {
  Series displacement;
  Series distance2;
  Series identity;
};

inline PairGeometry pairs(const Bodies &bodies, double softening) {
  const Series &position = bodies.position().si_value();
  const Series source = position.unsqueeze(-3);
  const Series target = position.unsqueeze(-2);
  const Series displacement = source.sub(target);
  const Series identity = pair_identity(bodies);
  const Series distance2 =
      displacement.square().sum(-1, true).add(softening * softening);
  const Series safe_distance2 = distance2.add(identity.unsqueeze(-1));
  if (!safe_distance2.gt(0.0).all().host<bool>().front())
    throw std::domain_error(
        "natsci::mechanics: distinct bodies occupy the same position; "
        "provide positive softening or separate them");
  return {std::move(displacement), std::move(safe_distance2),
          std::move(identity)};
}

inline void require_softening(double softening) {
  if (!(softening >= 0.0) || !std::isfinite(softening))
    throw std::invalid_argument(
        "natsci::mechanics: softening must be finite and non-negative");
}

} // namespace detail

[[nodiscard]] inline Bodies
make(const common::SeriesQuantity<common::Mass> &mass,
     const common::SeriesQuantity<common::Length> &position,
     const common::SeriesQuantity<common::Speed> &velocity) {
  const Series &raw_position = position.si_value();
  const Series &raw_velocity = velocity.si_value();
  const Series &raw_mass = mass.si_value();
  detail::require_real_float(raw_position, "position");
  detail::require_real_float(raw_velocity, "velocity");
  detail::require_real_float(raw_mass, "mass");
  if (raw_position.dim() < 2)
    throw std::invalid_argument(
        "natsci::mechanics::make: position must have shape "
        "[..., bodies, axes]");
  if (!detail::same_shape(raw_position, raw_velocity))
    throw std::invalid_argument(
        "natsci::mechanics::make: position and velocity shapes must match");
  if (raw_position.scalar_type() != raw_velocity.scalar_type() ||
      raw_position.scalar_type() != raw_mass.scalar_type())
    throw std::invalid_argument(
        "natsci::mechanics::make: mass, position, and velocity dtypes must "
        "match");
  if (raw_position.platform() != raw_velocity.platform() ||
      raw_position.platform() != raw_mass.platform())
    throw std::invalid_argument(
        "natsci::mechanics::make: mass, position, and velocity platforms "
        "must match");
  if (raw_position.size(-2) <= 0 || raw_position.size(-1) <= 0 ||
      raw_position.size(-1) > 3)
    throw std::invalid_argument(
        "natsci::mechanics::make: expected at least one body and one to "
        "three spatial axes");

  std::vector<std::int64_t> mass_shape(raw_position.sizes().begin(),
                                       raw_position.sizes().end() - 1);
  Series aligned_mass = pluss::series::undefined();
  try {
    aligned_mass = raw_mass.expand(mass_shape);
  } catch (const std::exception &) {
    throw std::invalid_argument(
        "natsci::mechanics::make: mass must broadcast to [..., bodies]");
  }
  if (!aligned_mass.ge(0.0).all().host<bool>().front())
    throw std::invalid_argument(
        "natsci::mechanics::make: mass must be non-negative");
  if (!aligned_mass.sum(-1, false).gt(0.0).all().host<bool>().front())
    throw std::invalid_argument(
        "natsci::mechanics::make: every batch must have positive total mass");

  return Bodies(common::SeriesQuantity<common::Mass>::from_si(
                    std::move(aligned_mass)),
                position, velocity);
}

[[nodiscard]] inline common::SeriesQuantity<common::Momentum>
momentum(const Bodies &bodies) {
  return common::SeriesQuantity<common::Momentum>::from_si(
      detail::body_mass(bodies).mul(bodies.velocity().si_value()));
}

[[nodiscard]] inline common::SeriesQuantity<common::Momentum>
total_momentum(const Bodies &bodies) {
  return common::SeriesQuantity<common::Momentum>::from_si(
      momentum(bodies).si_value().sum(-2, false));
}

[[nodiscard]] inline common::SeriesQuantity<common::Mass>
total_mass(const Bodies &bodies) {
  return common::SeriesQuantity<common::Mass>::from_si(
      bodies.mass().si_value().sum(-1, false));
}

[[nodiscard]] inline common::SeriesQuantity<common::Length>
center(const Bodies &bodies) {
  const Series weighted =
      detail::body_mass(bodies).mul(bodies.position().si_value());
  return common::SeriesQuantity<common::Length>::from_si(
      weighted.sum(-2, false).div(total_mass(bodies).si_value().unsqueeze(-1)));
}

[[nodiscard]] inline common::SeriesQuantity<common::Speed>
center_speed(const Bodies &bodies) {
  return common::SeriesQuantity<common::Speed>::from_si(
      total_momentum(bodies).si_value().div(
          total_mass(bodies).si_value().unsqueeze(-1)));
}

[[nodiscard]] inline common::SeriesQuantity<common::Energy>
kinetic_energy(const Bodies &bodies) {
  const Series speed2 = bodies.velocity().si_value().square().sum(-1, false);
  return common::SeriesQuantity<common::Energy>::from_si(
      bodies.mass().si_value().mul(speed2).mul(0.5));
}

[[nodiscard]] inline common::SeriesQuantity<common::Energy>
total_kinetic(const Bodies &bodies) {
  return common::SeriesQuantity<common::Energy>::from_si(
      kinetic_energy(bodies).si_value().sum(-1, false));
}

[[nodiscard]] inline common::SeriesQuantity<common::Acceleration>
gravity(const Bodies &bodies,
        common::ScalarQuantity<common::GravitationalConstant> constant =
            common::constants::codata2022::newtonian_gravitation.measurement
                .value(),
        common::ScalarQuantity<common::Length> softening =
            common::ScalarQuantity<common::Length>::from_si(0.0)) {
  detail::require_softening(softening.si_value());
  const auto pair = detail::pairs(bodies, softening.si_value());
  const Series source_mass = bodies.mass().si_value().unsqueeze(-2).unsqueeze(-1);
  const Series scale = pair.distance2.pow(-1.5).mul(source_mass).mul(
      constant.si_value());
  return common::SeriesQuantity<common::Acceleration>::from_si(
      pair.displacement.mul(scale).sum(-2, false));
}

[[nodiscard]] inline common::SeriesQuantity<common::Energy>
potential_energy(
    const Bodies &bodies,
    common::ScalarQuantity<common::GravitationalConstant> constant =
        common::constants::codata2022::newtonian_gravitation.measurement
            .value(),
    common::ScalarQuantity<common::Length> softening =
        common::ScalarQuantity<common::Length>::from_si(0.0)) {
  detail::require_softening(softening.si_value());
  const auto pair = detail::pairs(bodies, softening.si_value());
  const Series target_mass = bodies.mass().si_value().unsqueeze(-1);
  const Series source_mass = bodies.mass().si_value().unsqueeze(-2);
  const Series off_diagonal =
      pluss::series::ones_like(pair.identity).sub(pair.identity);
  const Series energy = target_mass.mul(source_mass)
                            .mul(off_diagonal)
                            .div(pair.distance2.sqrt().squeeze(-1))
                            .sum({-2, -1}, false)
                            .mul(-0.5 * constant.si_value());
  return common::SeriesQuantity<common::Energy>::from_si(energy);
}

[[nodiscard]] inline common::SeriesQuantity<common::Energy>
total_energy(
    const Bodies &bodies,
    common::ScalarQuantity<common::GravitationalConstant> constant =
        common::constants::codata2022::newtonian_gravitation.measurement
            .value(),
    common::ScalarQuantity<common::Length> softening =
        common::ScalarQuantity<common::Length>::from_si(0.0)) {
  return total_kinetic(bodies) +
         potential_energy(bodies, constant, softening);
}

[[nodiscard]] inline common::SeriesQuantity<common::AngularMomentum>
angular_momentum(const Bodies &bodies) {
  if (bodies.axes() != 3)
    throw std::invalid_argument(
        "natsci::mechanics::angular_momentum: expected exactly three axes");
  return common::SeriesQuantity<common::AngularMomentum>::from_si(
      bodies.position().si_value().cross(momentum(bodies).si_value(), -1).sum(
          -2, false));
}

[[nodiscard]] inline Bodies shift_to_center(const Bodies &bodies) {
  const Series shifted_position = bodies.position().si_value().sub(
      center(bodies).si_value().unsqueeze(-2));
  const Series shifted_velocity = bodies.velocity().si_value().sub(
      center_speed(bodies).si_value().unsqueeze(-2));
  return make(bodies.mass(),
              common::SeriesQuantity<common::Length>::from_si(
                  std::move(shifted_position)),
              common::SeriesQuantity<common::Speed>::from_si(
                  std::move(shifted_velocity)));
}

template <class AccelerationFn>
[[nodiscard]] Bodies leapfrog(const Bodies &bodies,
                              common::ScalarQuantity<common::Time> step,
                              AccelerationFn &&acceleration) {
  if (!(step.si_value() > 0.0) || !std::isfinite(step.si_value()))
    throw std::invalid_argument(
        "natsci::mechanics::leapfrog: step must be finite and positive");
  const double half_step = 0.5 * step.si_value();
  const Series half_position = bodies.position().si_value().add(
      bodies.velocity().si_value().mul(half_step));
  const Bodies drifted = make(
      bodies.mass(),
      common::SeriesQuantity<common::Length>::from_si(half_position),
      bodies.velocity());
  const auto force = std::forward<AccelerationFn>(acceleration)(drifted);
  static_assert(
      std::same_as<typename std::remove_cvref_t<decltype(force)>::dimension,
                   common::Acceleration>,
      "mechanics::leapfrog callback must return acceleration");
  const Series next_velocity = bodies.velocity().si_value().add(
      force.si_value().mul(step.si_value()));
  const Series next_position =
      half_position.add(next_velocity.mul(half_step));
  return make(bodies.mass(),
              common::SeriesQuantity<common::Length>::from_si(next_position),
              common::SeriesQuantity<common::Speed>::from_si(next_velocity));
}

[[nodiscard]] inline Bodies
gravity_step(
    const Bodies &bodies, common::ScalarQuantity<common::Time> step,
    common::ScalarQuantity<common::GravitationalConstant> constant =
        common::constants::codata2022::newtonian_gravitation.measurement
            .value(),
    common::ScalarQuantity<common::Length> softening =
        common::ScalarQuantity<common::Length>::from_si(0.0)) {
  return leapfrog(bodies, step, [&](const Bodies &drifted) {
    return gravity(drifted, constant, softening);
  });
}

} // namespace pluss::subsp::natsci::physics::mechanics
