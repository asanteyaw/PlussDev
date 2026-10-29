#pragma once

#include "pluss/subsp/natsci/common/poisson.h"
#include "pluss/subsp/natsci/common/solve.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::natsci::physics::fluids::gas {

namespace common = ::pluss::subsp::natsci::common;
namespace spatial = ::pluss::subsp::natsci::common::spatial;

using Density = common::DimensionQuotient<common::Mass, common::Volume>;
using MomDensity = common::DimensionProduct<Density, common::Speed>;
using EnergyDensity = common::Pressure;
using DynamicViscosity = common::DimensionProduct<
    Density, common::DimensionQuotient<common::Area, common::Time>>;
using SpecificGasConstant = common::DimensionQuotient<
    common::Energy,
    common::DimensionProduct<common::Mass, common::Temperature>>;
using ThermalConductivity = common::DimensionQuotient<
    common::Power,
    common::DimensionProduct<common::Length, common::Temperature>>;
using HeatFlux = common::DimensionQuotient<common::Power, common::Area>;
using DensityRate = common::DimensionQuotient<Density, common::Time>;
using MomRate = common::DimensionQuotient<MomDensity, common::Time>;
using EnergyRate = common::DimensionQuotient<EnergyDensity, common::Time>;

struct Model final {
  double gamma{1.4};
  double rho_floor{1e-12};
  double p_floor{1e-12};
  common::ScalarQuantity<SpecificGasConstant> gas_constant{
      common::ScalarQuantity<SpecificGasConstant>::from_si(287.052874)};
};

// Newtonian/Fourier transport coefficients. `bulk` is the physical bulk
// viscosity zeta; the Stokes correction -2 mu div(u) / 3 is applied by the
// constitutive kernel rather than encoded as a negative user coefficient.
struct Transport final {
  common::ScalarQuantity<DynamicViscosity> shear{
      common::ScalarQuantity<DynamicViscosity>::from_si(0.0)};
  common::ScalarQuantity<DynamicViscosity> bulk{
      common::ScalarQuantity<DynamicViscosity>::from_si(0.0)};
  common::ScalarQuantity<ThermalConductivity> heat{
      common::ScalarQuantity<ThermalConductivity>::from_si(0.0)};
};

struct TransportField final {
  common::SeriesQuantity<DynamicViscosity> shear;
  common::SeriesQuantity<DynamicViscosity> bulk;
  common::SeriesQuantity<ThermalConductivity> heat;
};

struct SutherlandLaw final {
  common::ScalarQuantity<DynamicViscosity> reference_viscosity{
      common::ScalarQuantity<DynamicViscosity>::from_si(1.716e-5)};
  common::ScalarQuantity<common::Temperature> reference_temperature{
      common::ScalarQuantity<common::Temperature>::from_si(273.15)};
  common::ScalarQuantity<common::Temperature> sutherland_temperature{
      common::ScalarQuantity<common::Temperature>::from_si(110.4)};
  double prandtl{0.71};
  double bulk_ratio{0.0};
};

enum class Recon : std::uint8_t {
  Cell = 0,
  Minmod = 1,
  Mc = 2,
  VanLeer = 3,
  Weno5 = 4,
};
enum class Rule : std::uint8_t { Euler = 0, Rk2 = 1, Rk3 = 2 };
enum class FluxScheme : std::uint8_t { Rusanov = 0, Hllc = 1, Roe = 2 };
enum class RoeMach : std::uint8_t { Classic = 0, Rieper = 1 };
enum class RoeShock : std::uint8_t { Off = 0, HFix = 1 };

struct RoePolicy final {
  // A zero coefficient retains only the transonic Harten--Hyman correction.
  // Positive values add a scale-relative eigenvalue floor.
  double entropy{0.05};
  RoeMach mach{RoeMach::Classic};
  double mach_floor{0.05};
  RoeShock shock{RoeShock::Off};
};

struct Options final {
  Rule rule{Rule::Rk2};
  Recon recon{Recon::Minmod};
  double courant{0.45};
  bool enforce{true};
  double diffusion{0.5};
  Transport transport{};
  FluxScheme flux{FluxScheme::Rusanov};
  RoePolicy roe{};
  std::optional<SutherlandLaw> sutherland{};
};

struct RunOptions final {
  double safety{0.9};
  common::ScalarQuantity<common::Time> min_step{
      common::ScalarQuantity<common::Time>::from_si(0.0)};
  common::ScalarQuantity<common::Time> max_step{
      common::ScalarQuantity<common::Time>::from_si(
          std::numeric_limits<double>::infinity())};
  std::int64_t max_steps{100'000};
  Options step{};
};

namespace detail {

inline bool same_layout(const Series &lhs, const Series &rhs) noexcept {
  if (lhs.dim() != rhs.dim() || lhs.scalar_type() != rhs.scalar_type() ||
      lhs.platform() != rhs.platform())
    return false;
  for (std::int64_t axis = 0; axis < lhs.dim(); ++axis)
    if (lhs.size(axis) != rhs.size(axis))
      return false;
  return true;
}

inline void validate_model(const Model &model, const char *operation) {
  if (!(model.gamma > 1.0) || !std::isfinite(model.gamma))
    throw std::invalid_argument(std::string(operation) +
                                ": gamma must be finite and greater than one");
  if (!(model.rho_floor >= 0.0) || !std::isfinite(model.rho_floor) ||
      !(model.p_floor >= 0.0) || !std::isfinite(model.p_floor))
    throw std::invalid_argument(std::string(operation) +
                                ": floors must be finite and non-negative");
  if (!(model.gas_constant.si_value() > 0.0) ||
      !std::isfinite(model.gas_constant.si_value()))
    throw std::invalid_argument(std::string(operation) +
                                ": gas_constant must be finite and positive");
}

inline void validate_transport(const Transport &transport,
                               const char *operation) {
  const auto valid = [](double value) {
    return value >= 0.0 && std::isfinite(value);
  };
  if (!valid(transport.shear.si_value()) || !valid(transport.bulk.si_value()) ||
      !valid(transport.heat.si_value()))
    throw std::invalid_argument(std::string(operation) +
                                ": transport coefficients must be finite and "
                                "non-negative");
}

inline void validate_sutherland(const SutherlandLaw &law,
                                const char *operation) {
  if (!(law.reference_viscosity.si_value() > 0.0) ||
      !std::isfinite(law.reference_viscosity.si_value()) ||
      !(law.reference_temperature.si_value() > 0.0) ||
      !std::isfinite(law.reference_temperature.si_value()) ||
      !(law.sutherland_temperature.si_value() >= 0.0) ||
      !std::isfinite(law.sutherland_temperature.si_value()) ||
      !(law.prandtl > 0.0) || !std::isfinite(law.prandtl) ||
      !(law.bulk_ratio >= 0.0) || !std::isfinite(law.bulk_ratio))
    throw std::invalid_argument(
        std::string(operation) +
        ": Sutherland parameters must be finite, with positive reference "
        "viscosity, temperature, and Prandtl number and non-negative "
        "Sutherland temperature and bulk ratio");
}

inline void validate_roe(const RoePolicy &policy, const char *operation) {
  if (!(policy.entropy >= 0.0) || !(policy.entropy <= 1.0) ||
      !std::isfinite(policy.entropy))
    throw std::invalid_argument(std::string(operation) +
                                ": Roe entropy must be finite and in [0, 1]");
  switch (policy.mach) {
  case RoeMach::Classic:
  case RoeMach::Rieper:
    break;
  default:
    throw std::invalid_argument(std::string(operation) +
                                ": Roe Mach policy is not recognized");
  }
  if (!(policy.mach_floor > 0.0) || !(policy.mach_floor <= 1.0) ||
      !std::isfinite(policy.mach_floor))
    throw std::invalid_argument(
        std::string(operation) +
        ": Roe Mach floor must be finite and in (0, 1]");
  switch (policy.shock) {
  case RoeShock::Off:
  case RoeShock::HFix:
    break;
  default:
    throw std::invalid_argument(std::string(operation) +
                                ": Roe shock policy is not recognized");
  }
}

inline std::int64_t stages(Rule rule, const char *operation) {
  switch (rule) {
  case Rule::Euler:
    return 1;
  case Rule::Rk2:
    return 2;
  case Rule::Rk3:
    return 3;
  }
  throw std::invalid_argument(std::string(operation) +
                              ": rule is not a recognized integrator");
}

inline void validate_options(const Options &options, const char *operation) {
  (void)stages(options.rule, operation);
  switch (options.recon) {
  case Recon::Cell:
  case Recon::Minmod:
  case Recon::Mc:
  case Recon::VanLeer:
  case Recon::Weno5:
    break;
  default:
    throw std::invalid_argument(std::string(operation) +
                                ": recon is not a recognized reconstruction");
  }
  switch (options.flux) {
  case FluxScheme::Rusanov:
  case FluxScheme::Hllc:
  case FluxScheme::Roe:
    break;
  default:
    throw std::invalid_argument(std::string(operation) +
                                ": flux is not a recognized scheme");
  }
  validate_roe(options.roe, operation);
  if (!(options.courant > 0.0) || !(options.courant <= 1.0) ||
      !std::isfinite(options.courant))
    throw std::invalid_argument(std::string(operation) +
                                ": courant must be finite and in (0, 1]");
  if (!(options.diffusion > 0.0) || !(options.diffusion <= 0.5) ||
      !std::isfinite(options.diffusion))
    throw std::invalid_argument(std::string(operation) +
                                ": diffusion must be finite and in (0, 0.5]");
  validate_transport(options.transport, operation);
  if (options.sutherland) {
    validate_sutherland(*options.sutherland, operation);
    if (options.transport.shear.si_value() != 0.0 ||
        options.transport.bulk.si_value() != 0.0 ||
        options.transport.heat.si_value() != 0.0)
      throw std::invalid_argument(
          std::string(operation) +
          ": constant transport and Sutherland transport are mutually "
          "exclusive");
  }
}

inline void validate_run(const RunOptions &options) {
  constexpr const char *operation = "natsci::fluids::gas::evolve_to";
  validate_options(options.step, operation);
  if (!(options.safety > 0.0) || !(options.safety <= 1.0) ||
      !std::isfinite(options.safety))
    throw std::invalid_argument(
        "natsci::fluids::gas::evolve_to: safety must be finite and in (0, 1]");
  if (!(options.min_step.si_value() >= 0.0) ||
      !std::isfinite(options.min_step.si_value()) ||
      !(options.max_step.si_value() > 0.0) ||
      options.min_step.si_value() > options.max_step.si_value())
    throw std::invalid_argument(
        "natsci::fluids::gas::evolve_to: step bounds are invalid");
  if (options.max_steps <= 0)
    throw std::invalid_argument(
        "natsci::fluids::gas::evolve_to: max_steps must be positive");
}

struct Raw final {
  Series rho;
  Series mom;
  Series energy;
};

template <class Grid>
inline std::int64_t component_axis(const Raw &state, const Grid &grid,
                                   const char *operation) {
  spatial::detail::require_scalar_layout(state.rho, grid, operation);
  const std::int64_t axis =
      spatial::detail::require_vector_layout(state.mom, grid, operation);
  spatial::detail::require_scalar_layout(state.energy, grid, operation);
  if (!same_layout(state.rho, state.energy))
    throw std::invalid_argument(std::string(operation) +
                                ": density and energy layouts must match");
  const Series first = state.mom.select(axis, 0);
  if (!same_layout(first, state.rho))
    throw std::invalid_argument(std::string(operation) +
                                ": momentum must add only its component axis");
  return axis;
}

template <class Grid>
inline Series pressure(const Raw &state, const Grid &grid, const Model &model) {
  const std::int64_t axis =
      component_axis(state, grid, "natsci::fluids::gas::pressure");
  const Series kinetic =
      state.mom.square().sum(axis, false).div(state.rho).mul(0.5);
  return state.energy.sub(kinetic).mul(model.gamma - 1.0);
}

template <class Grid>
inline Series valid(const Raw &state, const Grid &grid, const Model &model) {
  return state.rho.gt(model.rho_floor)
      .logical_and(pressure(state, grid, model).gt(model.p_floor))
      .logical_and(state.rho.isfinite())
      .logical_and(
          state.mom.isfinite().all(state.mom.dim() - grid.dim() - 1, false))
      .logical_and(state.energy.isfinite());
}

inline Raw convex_mix(const Raw &low, const Raw &high, const Series &weight,
                      std::int64_t component) {
  const Series vector_weight = weight.unsqueeze(component);
  const Series mixed_rho = low.rho.add(high.rho.sub(low.rho).mul(weight));
  const Series mixed_mom =
      low.mom.add(high.mom.sub(low.mom).mul(vector_weight));
  const Series mixed_energy =
      low.energy.add(high.energy.sub(low.energy).mul(weight));
  const Series use_low = weight.eq(0.0);
  const Series use_high = weight.eq(1.0);
  const Series vector_low = use_low.unsqueeze(component);
  const Series vector_high = use_high.unsqueeze(component);
  return {use_low.where(low.rho, use_high.where(high.rho, mixed_rho)),
          vector_low.where(low.mom, vector_high.where(high.mom, mixed_mom)),
          use_low.where(low.energy, use_high.where(high.energy, mixed_energy))};
}

template <class Grid>
inline Series admissible_fraction(const Raw &low, const Raw &high,
                                  const Grid &grid, const Model &model) {
  const std::int64_t component = low.mom.dim() - grid.dim() - 1;
  const Series high_valid = valid(high, grid, model);
  if (high_valid.all().template item<bool>())
    return pluss::series::ones_like(high.rho);

  Series lower = high_valid.where(pluss::series::ones_like(high.rho),
                                  pluss::series::zeros_like(high.rho));
  Series upper = pluss::series::ones_like(high.rho);
  const int iterations =
      high.rho.scalar_type() == ScalarType::Float32 ? 24 : 52;
  for (int iteration = 0; iteration < iterations; ++iteration) {
    const Series middle = lower.add(upper).mul(0.5);
    const Series middle_valid =
        valid(convex_mix(low, high, middle, component), grid, model);
    lower = middle_valid.where(middle, lower);
    upper = middle_valid.where(upper, middle);
  }
  return lower;
}

template <class Grid>
inline Raw limit_toward(const Raw &mean, const Raw &candidate, const Grid &grid,
                        const Model &model) {
  const Series fraction = admissible_fraction(mean, candidate, grid, model);
  return convex_mix(mean, candidate, fraction, mean.mom.dim() - grid.dim() - 1);
}

template <class Grid>
inline void require_valid(const Raw &state, const Grid &grid,
                          const Model &model, const char *operation) {
  validate_model(model, operation);
  component_axis(state, grid, operation);
  if (!valid(state, grid, model).all().template item<bool>())
    throw std::domain_error(std::string(operation) +
                            ": every cell must have finite positive density "
                            "and pressure");
}

inline Series minmod(const Series &back, const Series &forward) {
  const Series same_sign = back.mul(forward).gt(0.0);
  return same_sign.where(back.sgn().mul(back.abs().minimum(forward.abs())),
                         pluss::series::zeros_like(back));
}

inline Series limited_slope(const Series &back, const Series &forward,
                            Recon recon) {
  switch (recon) {
  case Recon::Cell:
    return pluss::series::zeros_like(back);
  case Recon::Minmod:
    return minmod(back, forward);
  case Recon::Mc:
    return minmod(minmod(back.mul(2.0), forward.mul(2.0)),
                  back.add(forward).mul(0.5));
  case Recon::VanLeer: {
    const Series same_sign = back.mul(forward).gt(0.0);
    const Series denominator = back.add(forward);
    const Series safe =
        same_sign.where(denominator, pluss::series::ones_like(denominator));
    return same_sign.where(back.mul(forward).mul(2.0).div(safe),
                           pluss::series::zeros_like(back));
  }
  case Recon::Weno5:
    throw std::invalid_argument(
        "natsci::fluids::gas: WENO5 uses face reconstruction, not a slope");
  }
  throw std::invalid_argument(
      "natsci::fluids::gas: recon is not a recognized reconstruction");
}

inline Series weno5_left(const Series &m2, const Series &m1,
                         const Series &center, const Series &p1,
                         const Series &p2) {
  const Series q0 = m2.mul(2.0).sub(m1.mul(7.0)).add(center.mul(11.0)).div(6.0);
  const Series q1 = m1.neg().add(center.mul(5.0)).add(p1.mul(2.0)).div(6.0);
  const Series q2 = center.mul(2.0).add(p1.mul(5.0)).sub(p2).div(6.0);
  const Series beta0 =
      m2.sub(m1.mul(2.0))
          .add(center)
          .square()
          .mul(13.0 / 12.0)
          .add(m2.sub(m1.mul(4.0)).add(center.mul(3.0)).square().mul(0.25));
  const Series beta1 = m1.sub(center.mul(2.0))
                           .add(p1)
                           .square()
                           .mul(13.0 / 12.0)
                           .add(m1.sub(p1).square().mul(0.25));
  const Series beta2 =
      center.sub(p1.mul(2.0))
          .add(p2)
          .square()
          .mul(13.0 / 12.0)
          .add(center.mul(3.0).sub(p1.mul(4.0)).add(p2).square().mul(0.25));
  const Series scale = m2.abs()
                           .maximum(m1.abs())
                           .maximum(center.abs())
                           .maximum(p1.abs())
                           .maximum(p2.abs())
                           .square()
                           .clamp_min(1.0);
  const double relative =
      center.scalar_type() == ScalarType::Float32 ? 1e-6 : 1e-12;
  const Series epsilon = scale.mul(relative);
  const Series alpha0 = beta0.add(epsilon).square().reciprocal().mul(0.1);
  const Series alpha1 = beta1.add(epsilon).square().reciprocal().mul(0.6);
  const Series alpha2 = beta2.add(epsilon).square().reciprocal().mul(0.3);
  const Series total = alpha0.add(alpha1).add(alpha2);
  return q0.mul(alpha0).add(q1.mul(alpha1)).add(q2.mul(alpha2)).div(total);
}

inline std::pair<Series, Series> weno5_faces(const Series &value,
                                             std::int64_t axis) {
  const Series m2 = value.roll(2, axis);
  const Series m1 = value.roll(1, axis);
  const Series p1 = value.roll(-1, axis);
  const Series p2 = value.roll(-2, axis);
  const Series p3 = value.roll(-3, axis);
  return {weno5_left(m2, m1, value, p1, p2), weno5_left(p3, p2, p1, value, m1)};
}

inline Series slope(const Series &value, const spatial::PeriodicGrid &grid,
                    std::int64_t axis, Recon recon) {
  const std::int64_t series_axis = value.dim() - grid.dim() + axis;
  return limited_slope(value.sub(value.roll(1, series_axis)),
                       value.roll(-1, series_axis).sub(value), recon);
}

struct Face final {
  Raw left;
  Raw right;
};

inline Face face(const Raw &state, const spatial::PeriodicGrid &grid,
                 std::int64_t axis, Recon recon, const Model &model) {
  const std::int64_t rho_axis = state.rho.dim() - grid.dim() + axis;
  const std::int64_t mom_axis = state.mom.dim() - grid.dim() + axis;
  Raw left;
  Raw right;
  if (recon == Recon::Weno5) {
    auto rho = weno5_faces(state.rho, rho_axis);
    auto mom = weno5_faces(state.mom, mom_axis);
    auto energy = weno5_faces(state.energy, rho_axis);
    left = {std::move(rho.first), std::move(mom.first),
            std::move(energy.first)};
    right = {std::move(rho.second), std::move(mom.second),
             std::move(energy.second)};
  } else {
    const Series rho_slope = slope(state.rho, grid, axis, recon);
    const Series mom_slope = slope(state.mom, grid, axis, recon);
    const Series energy_slope = slope(state.energy, grid, axis, recon);
    left = {state.rho.add(rho_slope.mul(0.5)),
            state.mom.add(mom_slope.mul(0.5)),
            state.energy.add(energy_slope.mul(0.5))};
    right = {
        state.rho.roll(-1, rho_axis).sub(rho_slope.roll(-1, rho_axis).mul(0.5)),
        state.mom.roll(-1, mom_axis).sub(mom_slope.roll(-1, mom_axis).mul(0.5)),
        state.energy.roll(-1, rho_axis)
            .sub(energy_slope.roll(-1, rho_axis).mul(0.5))};
  }

  const Raw first_left{state.rho, state.mom, state.energy};
  const Raw first_right{state.rho.roll(-1, rho_axis),
                        state.mom.roll(-1, mom_axis),
                        state.energy.roll(-1, rho_axis)};
  left = limit_toward(first_left, left, grid, model);
  right = limit_toward(first_right, right, grid, model);
  return {std::move(left), std::move(right)};
}

struct Flux final {
  Series mass;
  Series momentum;
  Series energy;
  Series wave;
};

inline Series face_pressure(const Raw &state, std::int64_t component,
                            const Model &model) {
  const Series kinetic =
      state.mom.square().sum(component, false).div(state.rho).mul(0.5);
  return state.energy.sub(kinetic).mul(model.gamma - 1.0);
}

inline Flux physical_flux(const Raw &state, std::int64_t component,
                          std::int64_t axis, const Model &model) {
  const Series rho_vector = state.rho.unsqueeze(component);
  const Series velocity = state.mom.div(rho_vector);
  const Series normal = velocity.select(component, axis);
  const Series p = face_pressure(state, component, model);
  std::vector<Series> momentum;
  momentum.reserve(static_cast<std::size_t>(velocity.size(component)));
  for (std::int64_t row = 0; row < velocity.size(component); ++row) {
    Series value = state.mom.select(component, row).mul(normal);
    if (row == axis)
      value = value.add(p);
    momentum.push_back(std::move(value));
  }
  const Series sound = p.mul(model.gamma).div(state.rho).sqrt();
  return {state.mom.select(component, axis),
          spatial::detail::stack_components(momentum, component),
          state.energy.add(p).mul(normal), normal.abs().add(sound)};
}

inline Flux rusanov_flux(const Raw &left_state, const Raw &right_state,
                         std::int64_t component, std::int64_t axis,
                         const Model &model) {
  Flux left = physical_flux(left_state, component, axis, model);
  Flux right = physical_flux(right_state, component, axis, model);
  const Series wave = left.wave.maximum(right.wave);
  return {left.mass.add(right.mass)
              .sub(right_state.rho.sub(left_state.rho).mul(wave))
              .mul(0.5),
          left.momentum.add(right.momentum)
              .sub(right_state.mom.sub(left_state.mom)
                       .mul(wave.unsqueeze(component)))
              .mul(0.5),
          left.energy.add(right.energy)
              .sub(right_state.energy.sub(left_state.energy).mul(wave))
              .mul(0.5),
          std::move(wave)};
}

inline Raw hllc_star(const Raw &state, const Series &pressure,
                     const Series &normal, const Series &wave,
                     const Series &contact, const Series &safe_gap,
                     std::int64_t component, std::int64_t axis) {
  const Series rho_star = state.rho.mul(wave.sub(normal)).div(safe_gap);
  const Series velocity = state.mom.div(state.rho.unsqueeze(component));
  std::vector<Series> star_velocity;
  star_velocity.reserve(static_cast<std::size_t>(velocity.size(component)));
  for (std::int64_t row = 0; row < velocity.size(component); ++row)
    star_velocity.push_back(row == axis ? contact
                                        : velocity.select(component, row));
  const Series vector =
      spatial::detail::stack_components(star_velocity, component);
  const Series specific_energy =
      state.energy.div(state.rho).add(contact.sub(normal).mul(
          contact.add(pressure.div(state.rho.mul(wave.sub(normal))))));
  return {rho_star, vector.mul(rho_star.unsqueeze(component)),
          rho_star.mul(specific_energy)};
}

inline Flux hllc_flux(const Raw &left_state, const Raw &right_state,
                      std::int64_t component, std::int64_t axis,
                      const Model &model) {
  Flux left = physical_flux(left_state, component, axis, model);
  Flux right = physical_flux(right_state, component, axis, model);
  const Series left_pressure = face_pressure(left_state, component, model);
  const Series right_pressure = face_pressure(right_state, component, model);
  const Series left_normal =
      left_state.mom.select(component, axis).div(left_state.rho);
  const Series right_normal =
      right_state.mom.select(component, axis).div(right_state.rho);
  const Series left_sound =
      left_pressure.mul(model.gamma).div(left_state.rho).sqrt();
  const Series right_sound =
      right_pressure.mul(model.gamma).div(right_state.rho).sqrt();
  const Series left_wave =
      left_normal.sub(left_sound).minimum(right_normal.sub(right_sound));
  const Series right_wave =
      left_normal.add(left_sound).maximum(right_normal.add(right_sound));

  const Series left_term = left_state.rho.mul(left_wave.sub(left_normal));
  const Series right_term = right_state.rho.mul(right_wave.sub(right_normal));
  const Series denominator = left_term.sub(right_term);
  const double tolerance =
      left_state.rho.scalar_type() == ScalarType::Float32 ? 1e-6 : 1e-12;
  const Series scale = left_term.abs().add(right_term.abs()).clamp_min(1.0);
  Series valid = denominator.abs().gt(scale.mul(tolerance));
  const Series one = pluss::series::ones_like(denominator);
  const Series safe_denominator = valid.where(denominator, one);
  const Series contact = right_pressure.sub(left_pressure)
                             .add(left_term.mul(left_normal))
                             .sub(right_term.mul(right_normal))
                             .div(safe_denominator);
  const Series left_gap = left_wave.sub(contact);
  const Series right_gap = right_wave.sub(contact);
  valid = valid.logical_and(left_gap.abs().gt(scale.mul(tolerance)))
              .logical_and(right_gap.abs().gt(scale.mul(tolerance)))
              .logical_and(contact.isfinite());
  const Series safe_left_gap = valid.where(left_gap, one);
  const Series safe_right_gap = valid.where(right_gap, one);
  const Raw left_star =
      hllc_star(left_state, left_pressure, left_normal, left_wave, contact,
                safe_left_gap, component, axis);
  const Raw right_star =
      hllc_star(right_state, right_pressure, right_normal, right_wave, contact,
                safe_right_gap, component, axis);
  const Series left_star_pressure =
      left_pressure.add(left_term.mul(contact.sub(left_normal)));
  const Series right_star_pressure =
      right_pressure.add(right_term.mul(contact.sub(right_normal)));
  valid = valid.logical_and(left_star.rho.gt(model.rho_floor))
              .logical_and(right_star.rho.gt(model.rho_floor))
              .logical_and(left_star_pressure.gt(model.p_floor))
              .logical_and(right_star_pressure.gt(model.p_floor))
              .logical_and(left_star.energy.isfinite())
              .logical_and(right_star.energy.isfinite());

  const Flux left_star_flux{
      left.mass.add(left_star.rho.sub(left_state.rho).mul(left_wave)),
      left.momentum.add(left_star.mom.sub(left_state.mom)
                            .mul(left_wave.unsqueeze(component))),
      left.energy.add(left_star.energy.sub(left_state.energy).mul(left_wave)),
      left.wave.maximum(right.wave)};
  const Flux right_star_flux{
      right.mass.add(right_star.rho.sub(right_state.rho).mul(right_wave)),
      right.momentum.add(right_star.mom.sub(right_state.mom)
                             .mul(right_wave.unsqueeze(component))),
      right.energy.add(
          right_star.energy.sub(right_state.energy).mul(right_wave)),
      left.wave.maximum(right.wave)};
  const Series use_left = left_wave.ge(0.0);
  const Series use_left_star = left_wave.lt(0.0).logical_and(contact.ge(0.0));
  const Series use_right_star = contact.lt(0.0).logical_and(right_wave.gt(0.0));
  const auto select_scalar = [&](const Series &a, const Series &b,
                                 const Series &c, const Series &d) {
    return use_left.where(a,
                          use_left_star.where(b, use_right_star.where(c, d)));
  };
  const Series vector_left = use_left.unsqueeze(component);
  const Series vector_left_star = use_left_star.unsqueeze(component);
  const Series vector_right_star = use_right_star.unsqueeze(component);
  Flux result{select_scalar(left.mass, left_star_flux.mass,
                            right_star_flux.mass, right.mass),
              vector_left.where(left.momentum, vector_left_star.where(
                                                   left_star_flux.momentum,
                                                   vector_right_star.where(
                                                       right_star_flux.momentum,
                                                       right.momentum))),
              select_scalar(left.energy, left_star_flux.energy,
                            right_star_flux.energy, right.energy),
              left.wave.maximum(right.wave)};
  const Flux fallback =
      rusanov_flux(left_state, right_state, component, axis, model);
  return {valid.where(result.mass, fallback.mass),
          valid.unsqueeze(component).where(result.momentum, fallback.momentum),
          valid.where(result.energy, fallback.energy), result.wave};
}

inline Series roe_abs(const Series &value, const Series &left,
                      const Series &right, const Series &scale,
                      double entropy) {
  const Series magnitude = value.abs();
  const Series transonic =
      value.sub(left).maximum(right.sub(value)).clamp_min(0.0);
  const Series width = transonic.maximum(scale.mul(entropy));
  const Series smooth = width.gt(0.0).logical_and(magnitude.lt(width));
  const Series safe = smooth.where(width, pluss::series::ones_like(width));
  const Series repaired = value.square().add(safe.square()).div(safe.mul(2.0));
  return smooth.where(repaired, magnitude);
}

inline Flux roe_flux(const Raw &left_state, const Raw &right_state,
                     std::int64_t component, std::int64_t axis,
                     const Model &model, const RoePolicy &policy,
                     const Series *wave_floor = nullptr) {
  const Flux left = physical_flux(left_state, component, axis, model);
  const Flux right = physical_flux(right_state, component, axis, model);
  const Flux fallback =
      rusanov_flux(left_state, right_state, component, axis, model);

  const Series left_pressure = face_pressure(left_state, component, model);
  const Series right_pressure = face_pressure(right_state, component, model);
  const Series left_velocity =
      left_state.mom.div(left_state.rho.unsqueeze(component));
  const Series right_velocity =
      right_state.mom.div(right_state.rho.unsqueeze(component));
  const Series left_normal = left_velocity.select(component, axis);
  const Series right_normal = right_velocity.select(component, axis);
  const Series left_sound =
      left_pressure.mul(model.gamma).div(left_state.rho).sqrt();
  const Series right_sound =
      right_pressure.mul(model.gamma).div(right_state.rho).sqrt();

  const Series left_root = left_state.rho.sqrt();
  const Series right_root = right_state.rho.sqrt();
  const Series root_sum = left_root.add(right_root);
  const Series roe_velocity =
      left_velocity.mul(left_root.unsqueeze(component))
          .add(right_velocity.mul(right_root.unsqueeze(component)))
          .div(root_sum.unsqueeze(component));
  const Series left_enthalpy =
      left_state.energy.add(left_pressure).div(left_state.rho);
  const Series right_enthalpy =
      right_state.energy.add(right_pressure).div(right_state.rho);
  const Series roe_enthalpy = left_enthalpy.mul(left_root)
                                  .add(right_enthalpy.mul(right_root))
                                  .div(root_sum);
  const Series speed2 = roe_velocity.square().sum(component, false);
  const Series sound2 =
      roe_enthalpy.sub(speed2.mul(0.5)).mul(model.gamma - 1.0);
  Series valid = sound2.gt(0.0).logical_and(sound2.isfinite());
  const Series safe_sound2 =
      valid.where(sound2, pluss::series::ones_like(sound2));
  const Series sound = safe_sound2.sqrt();
  const Series normal = roe_velocity.select(component, axis);
  const Series roe_density = left_root.mul(right_root);

  const Series max_wave = normal.abs().add(sound);
  // The contact and shear fields are linearly degenerate. Applying an acoustic
  // entropy floor to them would destroy Roe's exact stationary-contact
  // property without repairing a transonic rarefaction.
  Series lambda_zero = normal.abs();
  Series lambda_minus =
      roe_abs(normal.sub(sound), left_normal.sub(left_sound),
              right_normal.sub(right_sound), max_wave, policy.entropy);
  Series lambda_plus =
      roe_abs(normal.add(sound), left_normal.add(left_sound),
              right_normal.add(right_sound), max_wave, policy.entropy);
  if (wave_floor != nullptr) {
    lambda_zero = lambda_zero.maximum(*wave_floor);
    lambda_minus = lambda_minus.maximum(*wave_floor);
    lambda_plus = lambda_plus.maximum(*wave_floor);
  }

  const Series delta_pressure = right_pressure.sub(left_pressure);
  const Series delta_density = right_state.rho.sub(left_state.rho);
  Series delta_normal = right_normal.sub(left_normal);
  if (policy.mach == RoeMach::Rieper) {
    const Series left_mach =
        left_velocity.square().sum(component, false).sqrt().div(left_sound);
    const Series right_mach =
        right_velocity.square().sum(component, false).sqrt().div(right_sound);
    const Series zeta =
        left_mach.maximum(right_mach).maximum(policy.mach_floor).minimum(1.0);
    delta_normal = delta_normal.mul(zeta);
  }
  const Series pressure_wave = delta_pressure.div(safe_sound2);
  const Series acoustic = roe_density.mul(delta_normal).div(sound);
  const Series alpha_zero = delta_density.sub(pressure_wave);
  const Series alpha_minus = pressure_wave.sub(acoustic).mul(0.5);
  const Series alpha_plus = pressure_wave.add(acoustic).mul(0.5);
  const Series contact_diss = lambda_zero.mul(alpha_zero);
  const Series minus_diss = lambda_minus.mul(alpha_minus);
  const Series plus_diss = lambda_plus.mul(alpha_plus);

  std::vector<Series> momentum_dissipation;
  momentum_dissipation.reserve(
      static_cast<std::size_t>(roe_velocity.size(component)));
  Series shear_energy = pluss::series::zeros_like(sound);
  for (std::int64_t row = 0; row < roe_velocity.size(component); ++row) {
    const Series mean = roe_velocity.select(component, row);
    Series entry = contact_diss.mul(mean);
    if (row == axis) {
      entry = entry.add(minus_diss.mul(mean.sub(sound)))
                  .add(plus_diss.mul(mean.add(sound)));
    } else {
      const Series shear =
          lambda_zero.mul(roe_density)
              .mul(right_velocity.select(component, row)
                       .sub(left_velocity.select(component, row)));
      entry =
          entry.add(minus_diss.mul(mean)).add(plus_diss.mul(mean)).add(shear);
      shear_energy = shear_energy.add(shear.mul(mean));
    }
    momentum_dissipation.push_back(std::move(entry));
  }
  const Series mass_diss = contact_diss.add(minus_diss).add(plus_diss);
  const Series energy_diss =
      contact_diss.mul(speed2.mul(0.5))
          .add(minus_diss.mul(roe_enthalpy.sub(normal.mul(sound))))
          .add(plus_diss.mul(roe_enthalpy.add(normal.mul(sound))))
          .add(shear_energy);
  const Series momentum_diss =
      spatial::detail::stack_components(momentum_dissipation, component);
  Flux result{left.mass.add(right.mass).sub(mass_diss).mul(0.5),
              left.momentum.add(right.momentum).sub(momentum_diss).mul(0.5),
              left.energy.add(right.energy).sub(energy_diss).mul(0.5),
              left.wave.maximum(right.wave)};
  valid = valid.logical_and(result.mass.isfinite())
              .logical_and(result.momentum.isfinite().all(component, false))
              .logical_and(result.energy.isfinite());
  return {valid.where(result.mass, fallback.mass),
          valid.unsqueeze(component).where(result.momentum, fallback.momentum),
          valid.where(result.energy, fallback.energy), result.wave};
}

inline Series roe_eta(const Face &states, std::int64_t component,
                      std::int64_t axis, const Model &model) {
  const Series left_pressure = face_pressure(states.left, component, model);
  const Series right_pressure = face_pressure(states.right, component, model);
  const Series left_normal =
      states.left.mom.div(states.left.rho.unsqueeze(component))
          .select(component, axis);
  const Series right_normal =
      states.right.mom.div(states.right.rho.unsqueeze(component))
          .select(component, axis);
  const Series left_sound =
      left_pressure.mul(model.gamma).div(states.left.rho).sqrt();
  const Series right_sound =
      right_pressure.mul(model.gamma).div(states.right.rho).sqrt();
  return right_normal.add(right_sound)
      .sub(left_normal.sub(left_sound))
      .abs()
      .mul(0.5);
}

inline Series h_floor(const std::vector<Series> &eta,
                      const spatial::PeriodicGrid &grid, std::int64_t axis) {
  Series result = eta[static_cast<std::size_t>(axis)];
  const std::int64_t offset = result.dim() - grid.dim();
  for (std::int64_t transverse = 0; transverse < grid.dim(); ++transverse) {
    if (transverse == axis)
      continue;
    const Series along =
        eta[static_cast<std::size_t>(transverse)].roll(-1, offset + axis);
    result = result.maximum(eta[static_cast<std::size_t>(transverse)])
                 .maximum(eta[static_cast<std::size_t>(transverse)].roll(
                     1, offset + transverse))
                 .maximum(along)
                 .maximum(along.roll(1, offset + transverse));
  }
  return result;
}

inline Flux numerical_flux(const Raw &left, const Raw &right,
                           std::int64_t component, std::int64_t axis,
                           const Model &model, FluxScheme scheme,
                           const RoePolicy &roe,
                           const Series *wave_floor = nullptr) {
  switch (scheme) {
  case FluxScheme::Rusanov:
    return rusanov_flux(left, right, component, axis, model);
  case FluxScheme::Hllc:
    return hllc_flux(left, right, component, axis, model);
  case FluxScheme::Roe:
    return roe_flux(left, right, component, axis, model, roe, wave_floor);
  }
  throw std::invalid_argument(
      "natsci::fluids::gas: flux is not a recognized scheme");
}

inline Flux num_flux(const Raw &state, const spatial::PeriodicGrid &grid,
                     std::int64_t axis, const Model &model, Recon recon,
                     FluxScheme scheme, const RoePolicy &roe) {
  Face states = face(state, grid, axis, recon, model);
  const std::int64_t component = state.mom.dim() - grid.dim() - 1;
  return numerical_flux(states.left, states.right, component, axis, model,
                        scheme, roe);
}

struct AxisFlux final {
  Face states;
  Flux flux;
};

// Reconstruct every axis once. Coupled conservative systems (species,
// radiation, or other carried fields) consume this same mass flux rather than
// solving a second, subtly different Riemann problem.
inline std::vector<AxisFlux> axis_fluxes(const Raw &state,
                                         const spatial::PeriodicGrid &grid,
                                         const Model &model, Recon recon,
                                         FluxScheme scheme,
                                         const RoePolicy &roe) {
  const bool h_correction = scheme == FluxScheme::Roe &&
                            roe.shock == RoeShock::HFix && grid.dim() > 1;
  std::vector<Face> states;
  std::vector<Series> eta;
  states.reserve(static_cast<std::size_t>(grid.dim()));
  if (h_correction)
    eta.reserve(static_cast<std::size_t>(grid.dim()));

  const std::int64_t component = state.mom.dim() - grid.dim() - 1;
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    states.push_back(face(state, grid, axis, recon, model));
    if (h_correction)
      eta.push_back(roe_eta(states.back(), component, axis, model));
  }

  std::vector<AxisFlux> result;
  result.reserve(static_cast<std::size_t>(grid.dim()));
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    const Series floor =
        h_correction ? h_floor(eta, grid, axis) : pluss::series::undefined();
    Face &at = states[static_cast<std::size_t>(axis)];
    Flux flux = numerical_flux(at.left, at.right, component, axis, model,
                               scheme, roe, h_correction ? &floor : nullptr);
    result.push_back({std::move(at), std::move(flux)});
  }
  return result;
}

inline Raw conv_rate(const Raw &state, const spatial::PeriodicGrid &grid,
                     const std::vector<AxisFlux> &fluxes) {
  if (static_cast<std::int64_t>(fluxes.size()) != grid.dim())
    throw std::invalid_argument(
        "natsci::fluids::gas: axis flux count must match grid rank");
  Raw result{pluss::series::zeros_like(state.rho),
             pluss::series::zeros_like(state.mom),
             pluss::series::zeros_like(state.energy)};
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    const Flux &high = fluxes[static_cast<std::size_t>(axis)].flux;
    const std::int64_t scalar_axis = state.rho.dim() - grid.dim() + axis;
    const std::int64_t vector_axis = state.mom.dim() - grid.dim() + axis;
    const double width = grid.spacing(axis).si_value();
    result.rho = result.rho.sub(
        high.mass.sub(high.mass.roll(1, scalar_axis)).div(width));
    result.mom = result.mom.sub(
        high.momentum.sub(high.momentum.roll(1, vector_axis)).div(width));
    result.energy = result.energy.sub(
        high.energy.sub(high.energy.roll(1, scalar_axis)).div(width));
  }
  return result;
}

inline Raw conv_rate(const Raw &state, const spatial::PeriodicGrid &grid,
                     const Model &model, Recon recon, FluxScheme scheme,
                     const RoePolicy &roe) {
  return conv_rate(state, grid,
                   axis_fluxes(state, grid, model, recon, scheme, roe));
}

template <class Grid>
inline Series temperature(const Raw &state, const Grid &grid,
                          const Model &model) {
  return pressure(state, grid, model)
      .div(state.rho)
      .div(model.gas_constant.si_value());
}

inline TransportField sutherland_transport(const Series &value,
                                           const Model &model,
                                           const SutherlandLaw &law) {
  const double reference = law.reference_temperature.si_value();
  const double offset = law.sutherland_temperature.si_value();
  const Series reference_value =
      pluss::codem::make_scalar_like(value, reference);
  const Series offset_value = pluss::codem::make_scalar_like(value, offset);
  const Series exponent = pluss::codem::make_scalar_like(value, 1.5);
  const Series reference_viscosity =
      pluss::codem::make_scalar_like(value, law.reference_viscosity.si_value());
  const Series shear =
      value.div(reference_value)
          .pow(exponent)
          .mul(reference_value.add(offset_value).mul(reference_viscosity))
          .div(value.add(offset_value));
  const Series bulk = shear.mul(law.bulk_ratio);
  const double cp =
      model.gamma * model.gas_constant.si_value() / (model.gamma - 1.0);
  const Series heat =
      shear.mul(pluss::codem::make_scalar_like(shear, cp / law.prandtl));
  return {common::SeriesQuantity<DynamicViscosity>::from_si(shear),
          common::SeriesQuantity<DynamicViscosity>::from_si(bulk),
          common::SeriesQuantity<ThermalConductivity>::from_si(heat)};
}

template <class Grid>
inline TransportField sutherland_transport(const Raw &state, const Grid &grid,
                                           const Model &model,
                                           const SutherlandLaw &law) {
  return sutherland_transport(temperature(state, grid, model), model, law);
}

inline void validate_transport_field(const TransportField &transport,
                                     const Raw &state, const char *operation) {
  const Series &shear = transport.shear.si_value();
  const Series &bulk = transport.bulk.si_value();
  const Series &heat = transport.heat.si_value();
  if (!same_layout(shear, state.rho) || !same_layout(bulk, state.rho) ||
      !same_layout(heat, state.rho))
    throw std::invalid_argument(std::string(operation) +
                                ": transport fields must match density "
                                "shape, dtype, and platform");
  if (!shear.ge(0.0).logical_and(shear.isfinite()).all().item<bool>() ||
      !bulk.ge(0.0).logical_and(bulk.isfinite()).all().item<bool>() ||
      !heat.ge(0.0).logical_and(heat.isfinite()).all().item<bool>())
    throw std::invalid_argument(std::string(operation) +
                                ": transport fields must be finite and "
                                "non-negative");
}

inline Series newton_stress(const Raw &state, const spatial::PeriodicGrid &grid,
                            const Transport &transport) {
  const std::int64_t component =
      component_axis(state, grid, "natsci::fluids::gas::stress");
  const Series velocity = state.mom.div(state.rho.unsqueeze(component));
  const Series strain = spatial::sym_grad(velocity, grid);
  const Series div = spatial::divergence(velocity, grid);
  const double diagonal_scale =
      transport.bulk.si_value() - (2.0 / 3.0) * transport.shear.si_value();
  std::vector<Series> rows;
  rows.reserve(static_cast<std::size_t>(grid.dim()));
  for (std::int64_t row = 0; row < grid.dim(); ++row) {
    std::vector<Series> columns;
    columns.reserve(static_cast<std::size_t>(grid.dim()));
    for (std::int64_t column = 0; column < grid.dim(); ++column) {
      Series entry = strain.select(component, row)
                         .select(component, column)
                         .mul(2.0 * transport.shear.si_value());
      if (row == column)
        entry = entry.add(div.mul(diagonal_scale));
      columns.push_back(std::move(entry));
    }
    rows.push_back(spatial::detail::stack_components(columns, component));
  }
  return spatial::detail::stack_components(rows, component);
}

inline Series newton_stress(const Raw &state, const spatial::PeriodicGrid &grid,
                            const TransportField &transport) {
  const std::int64_t component =
      component_axis(state, grid, "natsci::fluids::gas::stress");
  const Series velocity = state.mom.div(state.rho.unsqueeze(component));
  const Series strain = spatial::sym_grad(velocity, grid);
  const Series div = spatial::divergence(velocity, grid);
  const Series &shear = transport.shear.si_value();
  const Series diagonal = transport.bulk.si_value().sub(shear.mul(2.0 / 3.0));
  std::vector<Series> rows;
  rows.reserve(static_cast<std::size_t>(grid.dim()));
  for (std::int64_t row = 0; row < grid.dim(); ++row) {
    std::vector<Series> columns;
    columns.reserve(static_cast<std::size_t>(grid.dim()));
    for (std::int64_t column = 0; column < grid.dim(); ++column) {
      Series entry = strain.select(component, row)
                         .select(component, column)
                         .mul(shear)
                         .mul(2.0);
      if (row == column)
        entry = entry.add(div.mul(diagonal));
      columns.push_back(std::move(entry));
    }
    rows.push_back(spatial::detail::stack_components(columns, component));
  }
  return spatial::detail::stack_components(rows, component);
}

inline Raw visc_rate(const Raw &state, const spatial::PeriodicGrid &grid,
                     const Model &model, const Transport &transport) {
  Raw result{pluss::series::zeros_like(state.rho),
             pluss::series::zeros_like(state.mom),
             pluss::series::zeros_like(state.energy)};
  if (transport.shear.si_value() == 0.0 && transport.bulk.si_value() == 0.0 &&
      transport.heat.si_value() == 0.0)
    return result;

  const std::int64_t component = state.mom.dim() - grid.dim() - 1;
  const Series velocity = state.mom.div(state.rho.unsqueeze(component));
  const Series tau = newton_stress(state, grid, transport);
  const Series temp = temperature(state, grid, model);
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    const std::int64_t scalar_axis = state.rho.dim() - grid.dim() + axis;
    const std::int64_t vector_axis = state.mom.dim() - grid.dim() + axis;
    const double width = grid.spacing(axis).si_value();

    const Series tau_axis = tau.select(component + 1, axis);
    const Series tau_face =
        tau_axis.add(tau_axis.roll(-1, vector_axis)).mul(0.5);
    result.mom =
        result.mom.add(tau_face.sub(tau_face.roll(1, vector_axis)).div(width));

    const Series work = tau_axis.mul(velocity).sum(component, false);
    const Series heat =
        spatial::partial(temp, grid, axis).mul(transport.heat.si_value());
    const Series energy_flux = work.add(heat);
    const Series energy_face =
        energy_flux.add(energy_flux.roll(-1, scalar_axis)).mul(0.5);
    result.energy = result.energy.add(
        energy_face.sub(energy_face.roll(1, scalar_axis)).div(width));
  }
  return result;
}

inline Raw visc_rate(const Raw &state, const spatial::PeriodicGrid &grid,
                     const Model &model, const TransportField &transport) {
  validate_transport_field(transport, state, "natsci::fluids::gas::rate");
  Raw result{pluss::series::zeros_like(state.rho),
             pluss::series::zeros_like(state.mom),
             pluss::series::zeros_like(state.energy)};
  const std::int64_t component = state.mom.dim() - grid.dim() - 1;
  const Series velocity = state.mom.div(state.rho.unsqueeze(component));
  const Series tau = newton_stress(state, grid, transport);
  const Series temp = temperature(state, grid, model);
  const Series &conductivity = transport.heat.si_value();
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    const std::int64_t scalar_axis = state.rho.dim() - grid.dim() + axis;
    const std::int64_t vector_axis = state.mom.dim() - grid.dim() + axis;
    const double width = grid.spacing(axis).si_value();

    const Series tau_axis = tau.select(component + 1, axis);
    const Series tau_face =
        tau_axis.add(tau_axis.roll(-1, vector_axis)).mul(0.5);
    result.mom =
        result.mom.add(tau_face.sub(tau_face.roll(1, vector_axis)).div(width));

    const Series velocity_face =
        velocity.add(velocity.roll(-1, vector_axis)).mul(0.5);
    const Series work_face = tau_face.mul(velocity_face).sum(component, false);
    const Series next_conductivity = conductivity.roll(-1, scalar_axis);
    const Series conductivity_sum = conductivity.add(next_conductivity);
    const Series conducting = conductivity_sum.gt(0.0);
    const Series conductivity_face = conducting.where(
        conductivity.mul(next_conductivity)
            .mul(2.0)
            .div(conducting.where(conductivity_sum,
                                  pluss::series::ones_like(conductivity_sum))),
        pluss::series::zeros_like(conductivity_sum));
    const Series heat_face =
        temp.roll(-1, scalar_axis).sub(temp).div(width).mul(conductivity_face);
    const Series energy_face = work_face.add(heat_face);
    result.energy = result.energy.add(
        energy_face.sub(energy_face.roll(1, scalar_axis)).div(width));
  }
  return result;
}

inline Raw rate(const Raw &state, const spatial::PeriodicGrid &grid,
                const Model &model, Recon recon, FluxScheme scheme,
                const RoePolicy &roe, const Transport &transport,
                const std::optional<SutherlandLaw> &sutherland) {
  Raw result = conv_rate(state, grid, model, recon, scheme, roe);
  Raw diffusive =
      sutherland
          ? visc_rate(state, grid, model,
                      sutherland_transport(state, grid, model, *sutherland))
          : visc_rate(state, grid, model, transport);
  result.mom = result.mom.add(diffusive.mom);
  result.energy = result.energy.add(diffusive.energy);
  return result;
}

inline Raw add_rate(const Raw &state, const Raw &rate, double step) {
  return {state.rho.add(rate.rho.mul(step)), state.mom.add(rate.mom.mul(step)),
          state.energy.add(rate.energy.mul(step))};
}

template <class Grid, class Fallback>
inline Raw admissible_advance(const Raw &state, const Raw &rate, double step,
                              const Grid &grid, const Model &model,
                              Fallback &&fallback, const char *operation) {
  Raw high = add_rate(state, rate, step);
  Series high_valid = valid(high, grid, model);
  if (high_valid.all().template item<bool>())
    return high;

  Raw low = add_rate(state, std::forward<Fallback>(fallback)(), step);
  require_valid(low, grid, model, operation);

  Series fraction = admissible_fraction(low, high, grid, model);
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis)
    fraction = fraction.amin(-1, false);
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis)
    fraction = fraction.unsqueeze(-1);
  const std::int64_t component = high.mom.dim() - grid.dim() - 1;
  Raw limited = convex_mix(low, high, fraction, component);
  require_valid(limited, grid, model, operation);
  return limited;
}

inline Raw blend(const Raw &left, const Raw &right, double left_weight,
                 double right_weight) {
  return {left.rho.mul(left_weight).add(right.rho.mul(right_weight)),
          left.mom.mul(left_weight).add(right.mom.mul(right_weight)),
          left.energy.mul(left_weight).add(right.energy.mul(right_weight))};
}

} // namespace detail

class State final {
public:
  [[nodiscard]] const common::SeriesQuantity<Density> &
  density() const noexcept {
    return density_;
  }
  [[nodiscard]] const common::SeriesQuantity<MomDensity> &
  momentum() const noexcept {
    return momentum_;
  }
  [[nodiscard]] const common::SeriesQuantity<EnergyDensity> &
  energy() const noexcept {
    return energy_;
  }

private:
  State(common::SeriesQuantity<Density> density,
        common::SeriesQuantity<MomDensity> momentum,
        common::SeriesQuantity<EnergyDensity> energy)
      : density_(std::move(density)), momentum_(std::move(momentum)),
        energy_(std::move(energy)) {}

  common::SeriesQuantity<Density> density_;
  common::SeriesQuantity<MomDensity> momentum_;
  common::SeriesQuantity<EnergyDensity> energy_;

  friend State make(const common::SeriesQuantity<Density> &,
                    const common::SeriesQuantity<MomDensity> &,
                    const common::SeriesQuantity<EnergyDensity> &,
                    const spatial::PeriodicGrid &, const Model &);
  friend State make(const common::SeriesQuantity<Density> &,
                    const common::SeriesQuantity<MomDensity> &,
                    const common::SeriesQuantity<EnergyDensity> &,
                    const spatial::BoxGrid &, const Model &);
  friend struct Access;
};

struct Access final {
  static detail::Raw raw(const State &state) {
    return {state.density().si_value(), state.momentum().si_value(),
            state.energy().si_value()};
  }
  static State make(detail::Raw state) {
    return State(
        common::SeriesQuantity<Density>::from_si(std::move(state.rho)),
        common::SeriesQuantity<MomDensity>::from_si(std::move(state.mom)),
        common::SeriesQuantity<EnergyDensity>::from_si(
            std::move(state.energy)));
  }
};

[[nodiscard]] inline State
make(const common::SeriesQuantity<Density> &density,
     const common::SeriesQuantity<MomDensity> &momentum,
     const common::SeriesQuantity<EnergyDensity> &energy,
     const spatial::PeriodicGrid &grid, const Model &model = {}) {
  detail::Raw raw{density.si_value(), momentum.si_value(), energy.si_value()};
  detail::require_valid(raw, grid, model, "natsci::fluids::gas::make");
  return Access::make(std::move(raw));
}

[[nodiscard]] inline State
make(const common::SeriesQuantity<Density> &density,
     const common::SeriesQuantity<MomDensity> &momentum,
     const common::SeriesQuantity<EnergyDensity> &energy,
     const spatial::BoxGrid &grid, const Model &model = {}) {
  detail::Raw raw{density.si_value(), momentum.si_value(), energy.si_value()};
  detail::require_valid(raw, grid, model, "natsci::fluids::gas::make");
  return Access::make(std::move(raw));
}

namespace detail {

inline common::StateVector record(const State &state) {
  return common::states(common::field("density", state.density(), "rho"),
                        common::field("momentum", state.momentum(), "rho u"),
                        common::field("energy", state.energy(), "E"));
}

inline State state(const common::StateVector &record,
                   const spatial::PeriodicGrid &grid, const Model &model) {
  return make(record.get<Density>("density"),
              record.get<MomDensity>("momentum"),
              record.get<EnergyDensity>("energy"), grid, model);
}

} // namespace detail

[[nodiscard]] inline common::SeriesQuantity<common::Speed>
velocity(const State &state, const spatial::PeriodicGrid &grid) {
  const auto raw = Access::raw(state);
  const std::int64_t component =
      detail::component_axis(raw, grid, "natsci::fluids::gas::velocity");
  return common::SeriesQuantity<common::Speed>::from_si(
      raw.mom.div(raw.rho.unsqueeze(component)));
}

[[nodiscard]] inline common::SeriesQuantity<common::Speed>
velocity(const State &state, const spatial::BoxGrid &grid) {
  const auto raw = Access::raw(state);
  const std::int64_t component =
      detail::component_axis(raw, grid, "natsci::fluids::gas::velocity");
  return common::SeriesQuantity<common::Speed>::from_si(
      raw.mom.div(raw.rho.unsqueeze(component)));
}

[[nodiscard]] inline common::SeriesQuantity<common::Pressure>
pressure(const State &state, const spatial::PeriodicGrid &grid,
         const Model &model = {}) {
  detail::validate_model(model, "natsci::fluids::gas::pressure");
  return common::SeriesQuantity<common::Pressure>::from_si(
      detail::pressure(Access::raw(state), grid, model));
}

[[nodiscard]] inline common::SeriesQuantity<common::Pressure>
pressure(const State &state, const spatial::BoxGrid &grid,
         const Model &model = {}) {
  detail::validate_model(model, "natsci::fluids::gas::pressure");
  return common::SeriesQuantity<common::Pressure>::from_si(
      detail::pressure(Access::raw(state), grid, model));
}

[[nodiscard]] inline common::SeriesQuantity<common::Speed>
sound(const State &state, const spatial::PeriodicGrid &grid,
      const Model &model = {}) {
  const Series p = pressure(state, grid, model).si_value();
  return common::SeriesQuantity<common::Speed>::from_si(
      p.mul(model.gamma).div(state.density().si_value()).sqrt());
}

[[nodiscard]] inline common::SeriesQuantity<common::Speed>
sound(const State &state, const spatial::BoxGrid &grid,
      const Model &model = {}) {
  const Series p = pressure(state, grid, model).si_value();
  return common::SeriesQuantity<common::Speed>::from_si(
      p.mul(model.gamma).div(state.density().si_value()).sqrt());
}

[[nodiscard]] inline common::SeriesQuantity<common::Temperature>
temperature(const State &state, const spatial::PeriodicGrid &grid,
            const Model &model = {}) {
  detail::validate_model(model, "natsci::fluids::gas::temperature");
  return common::SeriesQuantity<common::Temperature>::from_si(
      detail::temperature(Access::raw(state), grid, model));
}

[[nodiscard]] inline common::SeriesQuantity<common::Temperature>
temperature(const State &state, const spatial::BoxGrid &grid,
            const Model &model = {}) {
  detail::validate_model(model, "natsci::fluids::gas::temperature");
  return common::SeriesQuantity<common::Temperature>::from_si(
      detail::temperature(Access::raw(state), grid, model));
}

[[nodiscard]] inline common::SeriesQuantity<common::Pressure>
stress(const State &state, const spatial::PeriodicGrid &grid,
       const Transport &transport) {
  detail::validate_transport(transport, "natsci::fluids::gas::stress");
  return common::SeriesQuantity<common::Pressure>::from_si(
      detail::newton_stress(Access::raw(state), grid, transport));
}

[[nodiscard]] inline TransportField transport(const State &state,
                                              const spatial::PeriodicGrid &grid,
                                              const Model &model,
                                              const SutherlandLaw &law) {
  constexpr const char *operation = "natsci::fluids::gas::transport";
  detail::validate_model(model, operation);
  detail::validate_sutherland(law, operation);
  const auto raw = Access::raw(state);
  detail::require_valid(raw, grid, model, operation);
  return detail::sutherland_transport(raw, grid, model, law);
}

[[nodiscard]] inline common::SeriesQuantity<common::Pressure>
stress(const State &state, const spatial::PeriodicGrid &grid,
       const TransportField &transport) {
  detail::validate_transport_field(transport, Access::raw(state),
                                   "natsci::fluids::gas::stress");
  return common::SeriesQuantity<common::Pressure>::from_si(
      detail::newton_stress(Access::raw(state), grid, transport));
}

// Fourier heat flux q = -k grad(T). The conservative viscous energy flux is
// tau.u - q, hence the internal update uses +k grad(T).
[[nodiscard]] inline common::SeriesQuantity<HeatFlux>
heat_flux(const State &state, const spatial::PeriodicGrid &grid,
          const Model &model, const Transport &transport) {
  detail::validate_model(model, "natsci::fluids::gas::heat_flux");
  detail::validate_transport(transport, "natsci::fluids::gas::heat_flux");
  return common::SeriesQuantity<HeatFlux>::from_si(
      spatial::gradient(detail::temperature(Access::raw(state), grid, model),
                        grid)
          .mul(-transport.heat.si_value()));
}

[[nodiscard]] inline common::SeriesQuantity<HeatFlux>
heat_flux(const State &state, const spatial::PeriodicGrid &grid,
          const Model &model, const TransportField &transport) {
  detail::validate_model(model, "natsci::fluids::gas::heat_flux");
  const auto raw = Access::raw(state);
  detail::validate_transport_field(transport, raw,
                                   "natsci::fluids::gas::heat_flux");
  const std::int64_t component = raw.rho.dim() - grid.dim();
  return common::SeriesQuantity<HeatFlux>::from_si(
      spatial::gradient(detail::temperature(raw, grid, model), grid)
          .mul(transport.heat.si_value().unsqueeze(component))
          .neg());
}

struct Rate final {
  common::SeriesQuantity<DensityRate> density;
  common::SeriesQuantity<MomRate> momentum;
  common::SeriesQuantity<EnergyRate> energy;
};

[[nodiscard]] inline Rate rate(const State &state,
                               const spatial::PeriodicGrid &grid,
                               const Model &model = {},
                               const Options &options = {}) {
  detail::validate_model(model, "natsci::fluids::gas::rate");
  detail::validate_options(options, "natsci::fluids::gas::rate");
  detail::require_valid(Access::raw(state), grid, model,
                        "natsci::fluids::gas::rate");
  auto value =
      detail::rate(Access::raw(state), grid, model, options.recon, options.flux,
                   options.roe, options.transport, options.sutherland);
  return {common::SeriesQuantity<DensityRate>::from_si(std::move(value.rho)),
          common::SeriesQuantity<MomRate>::from_si(std::move(value.mom)),
          common::SeriesQuantity<EnergyRate>::from_si(std::move(value.energy))};
}

struct Limits final {
  common::SeriesQuantity<common::Time> acoustic;
  common::SeriesQuantity<common::Time> diffusion;
  common::SeriesQuantity<common::Time> allowed;
};

[[nodiscard]] inline Limits
limits(const State &state, const spatial::PeriodicGrid &grid,
       const Model &model = {}, const Transport &transport = {},
       double courant = 0.45, double diffusion = 0.5) {
  constexpr const char *operation = "natsci::fluids::gas::limits";
  detail::validate_model(model, operation);
  detail::validate_transport(transport, operation);
  if (!(courant > 0.0) || !(courant <= 1.0) || !std::isfinite(courant))
    throw std::invalid_argument(
        "natsci::fluids::gas::limits: courant must be finite and in (0, 1]");
  if (!(diffusion > 0.0) || !(diffusion <= 0.5) || !std::isfinite(diffusion))
    throw std::invalid_argument(
        "natsci::fluids::gas::limits: diffusion must be finite and in (0, "
        "0.5]");

  const Series speed_value = velocity(state, grid).si_value();
  const Series sound_value = sound(state, grid, model).si_value();
  const Series &rho = state.density().si_value();
  const std::int64_t component = speed_value.dim() - grid.dim() - 1;
  const std::vector<std::int64_t> batches(rho.sizes().begin(),
                                          rho.sizes().end() - grid.dim());
  Series acoustic_rate =
      pluss::series::zeros(batches, speed_value.scalar_type())
          .to(speed_value.platform());
  double inverse_h2 = 0.0;
  const auto axes = spatial::detail::spatial_axes(rho, grid);
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    acoustic_rate = acoustic_rate.add(speed_value.select(component, axis)
                                          .abs()
                                          .add(sound_value)
                                          .amax(axes, false)
                                          .div(grid.spacing(axis).si_value()));
    const double width = grid.spacing(axis).si_value();
    inverse_h2 += 1.0 / (width * width);
  }
  const Series acoustic =
      pluss::codem::make_scalar_like(acoustic_rate, courant).div(acoustic_rate);

  const double cv = model.gas_constant.si_value() / (model.gamma - 1.0);
  const Series momentum_diff = rho.reciprocal().mul(
      (4.0 / 3.0) * transport.shear.si_value() + transport.bulk.si_value());
  const Series thermal_diff =
      rho.reciprocal().mul(transport.heat.si_value() / cv);
  const Series max_diff = momentum_diff.maximum(thermal_diff).amax(axes, false);
  const Series diff_rate = max_diff.mul(inverse_h2);
  const Series infinite = pluss::series::full_like(
      diff_rate, std::numeric_limits<double>::infinity());
  const Series diff = diff_rate.gt(0.0).where(
      pluss::codem::make_scalar_like(diff_rate, diffusion)
          .div(diff_rate.add(diff_rate.eq(0.0).to(diff_rate.scalar_type()))),
      infinite);
  const Series allowed = acoustic.minimum(diff);
  return {common::SeriesQuantity<common::Time>::from_si(acoustic),
          common::SeriesQuantity<common::Time>::from_si(diff),
          common::SeriesQuantity<common::Time>::from_si(allowed)};
}

[[nodiscard]] inline Limits limits(const State &state,
                                   const spatial::PeriodicGrid &grid,
                                   const Model &model, const SutherlandLaw &law,
                                   double courant = 0.45,
                                   double diffusion = 0.5) {
  detail::validate_sutherland(law, "natsci::fluids::gas::limits");
  const Limits inviscid =
      limits(state, grid, model, Transport{}, courant, diffusion);
  const TransportField field = transport(state, grid, model, law);
  const Series &rho = state.density().si_value();
  const auto axes = spatial::detail::spatial_axes(rho, grid);
  double inverse_h2 = 0.0;
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    const double width = grid.spacing(axis).si_value();
    inverse_h2 += 1.0 / (width * width);
  }
  const double cv = model.gas_constant.si_value() / (model.gamma - 1.0);
  const Series momentum_diff =
      field.shear.si_value().mul(4.0 / 3.0).add(field.bulk.si_value()).div(rho);
  const Series thermal_diff = field.heat.si_value().div(rho).div(cv);
  const Series max_diff = momentum_diff.maximum(thermal_diff).amax(axes, false);
  const Series diff_rate = max_diff.mul(inverse_h2);
  const Series diff =
      pluss::codem::make_scalar_like(diff_rate, diffusion).div(diff_rate);
  const Series allowed = inviscid.acoustic.si_value().minimum(diff);
  return {inviscid.acoustic,
          common::SeriesQuantity<common::Time>::from_si(diff),
          common::SeriesQuantity<common::Time>::from_si(allowed)};
}

[[nodiscard]] inline common::SeriesQuantity<common::Time>
step_limit(const State &state, const spatial::PeriodicGrid &grid,
           const Model &model = {}, double courant = 0.45) {
  return limits(state, grid, model, Transport{}, courant).acoustic;
}

[[nodiscard]] inline common::SeriesQuantity<common::Time>
step_limit(const State &state, const spatial::PeriodicGrid &grid,
           const Model &model, const Transport &transport,
           double courant = 0.45, double diffusion = 0.5) {
  return limits(state, grid, model, transport, courant, diffusion).allowed;
}

struct StepResult final {
  State state;
  common::SeriesQuantity<common::Time> limit;
  std::int64_t flux_evaluations{0};
};

[[nodiscard]] inline StepResult step(const State &state,
                                     const spatial::PeriodicGrid &grid,
                                     common::ScalarQuantity<common::Time> width,
                                     const Model &model = {},
                                     const Options &options = {}) {
  constexpr const char *operation = "natsci::fluids::gas::step";
  detail::validate_model(model, operation);
  detail::validate_options(options, operation);
  if (!(width.si_value() > 0.0) || !std::isfinite(width.si_value()))
    throw std::invalid_argument(
        "natsci::fluids::gas::step: width must be finite and positive");
  const auto limit = options.sutherland
                         ? limits(state, grid, model, *options.sutherland,
                                  options.courant, options.diffusion)
                               .allowed
                         : step_limit(state, grid, model, options.transport,
                                      options.courant, options.diffusion);
  if (options.enforce &&
      !limit.si_value().ge(width.si_value()).all().item<bool>())
    throw std::domain_error(
        "natsci::fluids::gas::step: width exceeds the acoustic or diffusion "
        "stability limit");

  const detail::Raw initial = Access::raw(state);
  std::int64_t evaluations = 0;
  const auto eval = [&](const detail::Raw &at) {
    ++evaluations;
    return detail::rate(at, grid, model, options.recon, options.flux,
                        options.roe, options.transport, options.sutherland);
  };
  const auto advance = [&](const detail::Raw &at) {
    const detail::Raw high = eval(at);
    return detail::admissible_advance(
        at, high, width.si_value(), grid, model,
        [&] {
          ++evaluations;
          return detail::rate(at, grid, model, Recon::Cell, FluxScheme::Rusanov,
                              RoePolicy{}, options.transport,
                              options.sutherland);
        },
        "natsci::fluids::gas::step positivity fallback");
  };
  detail::Raw next;
  if (options.rule == Rule::Euler) {
    next = advance(initial);
  } else if (options.rule == Rule::Rk2) {
    const detail::Raw first = advance(initial);
    const detail::Raw second = advance(first);
    next = detail::blend(initial, second, 0.5, 0.5);
  } else {
    const detail::Raw first = advance(initial);
    const detail::Raw first_end = advance(first);
    const detail::Raw second = detail::blend(initial, first_end, 0.75, 0.25);
    detail::require_valid(second, grid, model,
                          "natsci::fluids::gas::step stage 2");
    const detail::Raw second_end = advance(second);
    next = detail::blend(initial, second_end, 1.0 / 3.0, 2.0 / 3.0);
  }
  detail::require_valid(next, grid, model, operation);
  return {Access::make(std::move(next)), limit, evaluations};
}

struct FlowPath final {
  common::Trajectory<common::Time> trajectory;
  std::int64_t steps{0};
  std::int64_t flux_evaluations{0};
};

[[nodiscard]] inline FlowPath
evolve(const State &initial, const spatial::PeriodicGrid &grid,
       common::ScalarQuantity<common::Time> start,
       common::ScalarQuantity<common::Time> width, std::int64_t steps,
       const Model &model = {}, const Options &options = {}) {
  detail::validate_model(model, "natsci::fluids::gas::evolve");
  detail::validate_options(options, "natsci::fluids::gas::evolve");
  std::int64_t evaluations = 0;
  auto marched = common::solve::march(
      [&](common::ScalarQuantity<common::Time>,
          const common::StateVector &record,
          common::ScalarQuantity<common::Time> step_width) {
        auto advanced = step(detail::state(record, grid, model), grid,
                             step_width, model, options);
        evaluations += advanced.flux_evaluations;
        return detail::record(advanced.state);
      },
      detail::record(initial), start, width, steps);
  return {std::move(marched.trajectory), marched.steps, evaluations};
}

[[nodiscard]] inline FlowPath
evolve_to(const State &initial, const spatial::PeriodicGrid &grid,
          common::ScalarQuantity<common::Time> start,
          common::ScalarQuantity<common::Time> stop, const Model &model = {},
          const RunOptions &options = {}) {
  detail::validate_model(model, "natsci::fluids::gas::evolve_to");
  detail::validate_run(options);
  std::int64_t evaluations = 0;
  auto marched = common::solve::march_to(
      [&](common::ScalarQuantity<common::Time>,
          const common::StateVector &record,
          common::ScalarQuantity<common::Time> width) {
        auto advanced = step(detail::state(record, grid, model), grid, width,
                             model, options.step);
        evaluations += advanced.flux_evaluations;
        return detail::record(advanced.state);
      },
      [&](common::ScalarQuantity<common::Time> time,
          const common::StateVector &record) {
        const State at = detail::state(record, grid, model);
        const auto stable_limit =
            options.step.sutherland
                ? limits(at, grid, model, *options.step.sutherland,
                         options.step.courant, options.step.diffusion)
                      .allowed
                : step_limit(at, grid, model, options.step.transport,
                             options.step.courant, options.step.diffusion);
        const double stable =
            stable_limit.si_value().min().template item<double>();
        const double remaining = stop.si_value() - time.si_value();
        const double width =
            std::min(remaining, std::min(options.max_step.si_value(),
                                         options.safety * stable));
        if (!(width > 0.0) || !std::isfinite(width))
          throw std::domain_error(
              "natsci::fluids::gas::evolve_to: stability policy produced no "
              "finite positive step");
        if (width < options.min_step.si_value() &&
            remaining > options.min_step.si_value())
          throw std::domain_error(
              "natsci::fluids::gas::evolve_to: stable step is below min_step");
        return common::ScalarQuantity<common::Time>::from_si(width);
      },
      detail::record(initial), start, stop, options.max_steps);
  return {std::move(marched.trajectory), marched.steps, evaluations};
}

} // namespace pluss::subsp::natsci::physics::fluids::gas
