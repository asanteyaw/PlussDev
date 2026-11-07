#pragma once

// Conservative, nonreacting species transport for the calorically-perfect gas
// solver. Species partial densities share the gas Riemann mass flux, while
// mixture-averaged diffusion delegates molecular properties to chemistry.
// Finite-rate chemistry and species-enthalpy energy transport are intentionally
// separate contracts; this layer does not manufacture an incomplete reacting
// flow model.

#include "pluss/subsp/natsci/chemistry/transport.h"
#include "pluss/subsp/natsci/physics/fluids/compressible.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace pluss::subsp::natsci::physics::fluids::gas::species {

namespace chem_transport = ::pluss::subsp::natsci::chemistry::transport;

using DiffusiveMassFlux = common::DimensionProduct<Density, common::Speed>;

namespace detail {

inline std::int64_t species_axis(const Series &value,
                                 const spatial::PeriodicGrid &grid) {
  return value.dim() - grid.dim() - 1;
}

inline std::vector<std::int64_t>
trailing_species_permutation(const Series &value,
                             const spatial::PeriodicGrid &grid) {
  const std::int64_t species = species_axis(value, grid);
  std::vector<std::int64_t> order;
  order.reserve(static_cast<std::size_t>(value.dim()));
  for (std::int64_t axis = 0; axis < value.dim(); ++axis)
    if (axis != species)
      order.push_back(axis);
  order.push_back(species);
  return order;
}

inline Series to_trailing_species(const Series &value,
                                  const spatial::PeriodicGrid &grid) {
  return value.permute(trailing_species_permutation(value, grid));
}

inline Series from_trailing_species(const Series &value,
                                    const spatial::PeriodicGrid &grid) {
  const std::int64_t rank = value.dim();
  const std::int64_t batches = rank - grid.dim() - 1;
  std::vector<std::int64_t> order;
  order.reserve(static_cast<std::size_t>(rank));
  for (std::int64_t axis = 0; axis < batches; ++axis)
    order.push_back(axis);
  order.push_back(rank - 1);
  for (std::int64_t axis = batches; axis < rank - 1; ++axis)
    order.push_back(axis);
  return value.permute(order);
}

inline void require_layout(const Series &partial, const Series &rho,
                           const spatial::PeriodicGrid &grid,
                           const char *operation) {
  if (!partial.defined() || partial.dim() < grid.dim() + 1)
    throw std::invalid_argument(std::string(operation) +
                                ": species density must add one species axis "
                                "before the spatial axes");
  const std::int64_t axis = species_axis(partial, grid);
  if (partial.size(axis) < 1)
    throw std::invalid_argument(std::string(operation) +
                                ": at least one species is required");
  if (!gas::detail::same_layout(partial.select(axis, 0), rho))
    throw std::invalid_argument(std::string(operation) +
                                ": each species lane must match gas density "
                                "shape, dtype, and platform");
}

inline void require_partial(const Series &partial, const Series &rho,
                            const spatial::PeriodicGrid &grid,
                            const char *operation) {
  require_layout(partial, rho, grid, operation);
  if (!partial.isfinite().all().item<bool>() ||
      !partial.ge(0.0).all().item<bool>())
    throw std::domain_error(std::string(operation) +
                            ": species partial densities must be finite and "
                            "non-negative");
  const double tolerance =
      partial.scalar_type() == ScalarType::Float32 ? 2e-5 : 2e-12;
  if (!partial.sum(species_axis(partial, grid), false)
           .allclose(rho, tolerance, tolerance))
    throw std::domain_error(std::string(operation) +
                            ": species partial densities must sum to total "
                            "density in every cell");
}

inline Series fractions(const Series &partial, const Series &rho,
                        const spatial::PeriodicGrid &grid) {
  return partial.div(rho.unsqueeze(species_axis(partial, grid)));
}

inline std::pair<Series, Series> reconstruct(const Series &value,
                                             const spatial::PeriodicGrid &grid,
                                             std::int64_t axis, Recon recon) {
  const std::int64_t spatial_axis = value.dim() - grid.dim() + axis;
  if (recon == Recon::Weno5)
    return gas::detail::weno5_faces(value, spatial_axis);
  const Series gradient = gas::detail::slope(value, grid, axis, recon);
  return {value.add(gradient.mul(0.5)),
          value.roll(-1, spatial_axis)
              .sub(gradient.roll(-1, spatial_axis).mul(0.5))};
}

inline Series normalize_face(const Series &candidate, const Series &fallback,
                             std::int64_t axis) {
  const Series clipped = candidate.clamp(0.0, 1.0);
  const Series total = clipped.sum(axis, true);
  const Series valid = total.gt(0.0).logical_and(total.isfinite());
  const Series safe = valid.where(total, pluss::series::ones_like(total));
  return valid.where(clipped.div(safe), fallback);
}

inline Series advective_rate(const Series &partial, const Series &rho,
                             const spatial::PeriodicGrid &grid, Recon recon,
                             const std::vector<gas::detail::AxisFlux> &fluxes) {
  const std::int64_t component = species_axis(partial, grid);
  const Series y = fractions(partial, rho, grid);
  Series result = pluss::series::zeros_like(partial);
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    auto faces = reconstruct(y, grid, axis, recon);
    const std::int64_t spatial_axis = partial.dim() - grid.dim() + axis;
    const Series left_fallback = y;
    const Series right_fallback = y.roll(-1, spatial_axis);
    const Series left = normalize_face(faces.first, left_fallback, component);
    const Series right =
        normalize_face(faces.second, right_fallback, component);
    const Series &mass = fluxes[static_cast<std::size_t>(axis)].flux.mass;
    const Series upwind = mass.ge(0.0).unsqueeze(component).where(left, right);
    const Series face_flux = upwind.mul(mass.unsqueeze(component));
    result = result.sub(face_flux.sub(face_flux.roll(1, spatial_axis))
                            .div(grid.spacing(axis).si_value()));
  }
  return result;
}

struct Diffusion final {
  std::vector<Series> fluxes;
  Series maximum;
};

inline Diffusion diffusion(const gas::detail::Raw &flow, const Series &partial,
                           const spatial::PeriodicGrid &grid,
                           const Model &model,
                           const chem_transport::GasTransport &transport) {
  const std::int64_t component = species_axis(partial, grid);
  if (transport.species() != partial.size(component))
    throw std::invalid_argument(
        "natsci::fluids::gas::species: transport species count does not "
        "match the state");

  const Series y = fractions(partial, flow.rho, grid);
  const Series y_last = to_trailing_species(y, grid);
  const Series x_last = transport.mole_fractions(y_last);
  const Series temperature = gas::detail::temperature(flow, grid, model);
  const Series pressure = gas::detail::pressure(flow, grid, model);
  const Series d_last =
      transport.mixture_diffusion_chemkin(temperature, x_last, pressure);
  const Series x = from_trailing_species(x_last, grid);
  const Series d = from_trailing_species(d_last, grid);

  std::vector<std::int64_t> mass_shape(static_cast<std::size_t>(partial.dim()),
                                       1);
  mass_shape[static_cast<std::size_t>(component)] = transport.species();
  const Series molar_mass = transport.molar_masses().reshape(mass_shape);
  const Series density = flow.rho.unsqueeze(component);

  Diffusion result;
  result.maximum = d.amax(component, false);
  result.fluxes.reserve(static_cast<std::size_t>(grid.dim()));
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    const std::int64_t spatial_axis = x.dim() - grid.dim() + axis;
    const Series next_x = x.roll(-1, spatial_axis);
    const Series face_x = x.add(next_x).mul(0.5);
    const Series mean_mass = face_x.mul(molar_mass).sum(component, true);
    const Series face_y = face_x.mul(molar_mass).div(mean_mass);
    const Series next_d = d.roll(-1, spatial_axis);
    const Series d_sum = d.add(next_d);
    const Series positive = d_sum.gt(0.0);
    const Series face_d =
        positive.where(d.mul(next_d).mul(2.0).div(positive.where(
                           d_sum, pluss::series::ones_like(d_sum))),
                       pluss::series::zeros_like(d_sum));
    const Series face_density =
        density.add(density.roll(-1, spatial_axis)).mul(0.5);
    Series flux = next_x.sub(x)
                      .div(grid.spacing(axis).si_value())
                      .mul(face_d)
                      .mul(molar_mass.div(mean_mass))
                      .mul(face_density)
                      .neg();
    flux = flux.sub(face_y.mul(flux.sum(component, true)));
    result.fluxes.push_back(std::move(flux));
  }
  return result;
}

inline Series diffusion_rate(const Diffusion &diffusion,
                             const spatial::PeriodicGrid &grid) {
  Series result = pluss::series::zeros_like(diffusion.fluxes.front());
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    const std::int64_t spatial_axis = result.dim() - grid.dim() + axis;
    const Series &face = diffusion.fluxes[static_cast<std::size_t>(axis)];
    result = result.sub(face.sub(face.roll(1, spatial_axis))
                            .div(grid.spacing(axis).si_value()));
  }
  return result;
}

struct Raw final {
  gas::detail::Raw flow;
  Series partial;
};

struct RawRate final {
  gas::detail::Raw flow;
  Series partial;
};

inline Raw add_rate(const Raw &state, const RawRate &rate, double step) {
  return {{state.flow.rho.add(rate.flow.rho.mul(step)),
           state.flow.mom.add(rate.flow.mom.mul(step)),
           state.flow.energy.add(rate.flow.energy.mul(step))},
          state.partial.add(rate.partial.mul(step))};
}

inline Raw blend(const Raw &left, const Raw &right, double left_weight,
                 double right_weight) {
  return {gas::detail::blend(left.flow, right.flow, left_weight, right_weight),
          left.partial.mul(left_weight).add(right.partial.mul(right_weight))};
}

inline Series reduce_trailing(Series value, std::int64_t count) {
  for (std::int64_t axis = 0; axis < count; ++axis)
    value = value.amin(-1, false);
  return value;
}

inline Series max_trailing(Series value, std::int64_t count) {
  for (std::int64_t axis = 0; axis < count; ++axis)
    value = value.amax(-1, false);
  return value;
}

inline Series expand_trailing(Series value, std::int64_t count) {
  for (std::int64_t axis = 0; axis < count; ++axis)
    value = value.unsqueeze(-1);
  return value;
}

inline Series species_fraction(const Series &low, const Series &high,
                               const spatial::PeriodicGrid &grid) {
  const Series finite = high.isfinite();
  const Series valid = finite.logical_and(high.ge(0.0));
  const Series denominator = low.sub(high);
  const Series safe = denominator.ne(0.0).where(
      denominator, pluss::series::ones_like(denominator));
  const Series candidate = low.div(safe).clamp(0.0, 1.0);
  const Series one = pluss::series::ones_like(high);
  const Series zero = pluss::series::zeros_like(high);
  const Series local = valid.where(one, finite.where(candidate, zero));
  return reduce_trailing(local, grid.dim() + 1);
}

} // namespace detail

struct Access;

class State final {
public:
  [[nodiscard]] const gas::State &flow() const noexcept { return flow_; }
  [[nodiscard]] const common::SeriesQuantity<Density> &
  partial_density() const noexcept {
    return partial_density_;
  }

private:
  State(gas::State flow, common::SeriesQuantity<Density> partial_density)
      : flow_(std::move(flow)), partial_density_(std::move(partial_density)) {}

  gas::State flow_;
  common::SeriesQuantity<Density> partial_density_;

  friend State make(const gas::State &, const Series &,
                    const spatial::PeriodicGrid &);
  friend State from_density(const gas::State &,
                            const common::SeriesQuantity<Density> &,
                            const spatial::PeriodicGrid &);
  friend struct Access;
};

struct Access final {
  static detail::Raw raw(const State &state) {
    return {gas::Access::raw(state.flow()), state.partial_density().si_value()};
  }
  static State make(detail::Raw state) {
    return State(
        gas::Access::make(std::move(state.flow)),
        common::SeriesQuantity<Density>::from_si(std::move(state.partial)));
  }
};

[[nodiscard]] inline State make(const gas::State &flow,
                                const Series &mass_fractions,
                                const spatial::PeriodicGrid &grid) {
  constexpr const char *operation = "natsci::fluids::gas::species::make";
  const Series &rho = flow.density().si_value();
  detail::require_layout(mass_fractions, rho, grid, operation);
  if (!mass_fractions.isfinite().all().item<bool>() ||
      !mass_fractions.ge(0.0).all().item<bool>())
    throw std::domain_error(std::string(operation) +
                            ": mass fractions must be finite and "
                            "non-negative");
  const std::int64_t axis = detail::species_axis(mass_fractions, grid);
  const Series sum = mass_fractions.sum(axis, false);
  const double tolerance =
      mass_fractions.scalar_type() == ScalarType::Float32 ? 2e-5 : 2e-12;
  if (!sum.allclose(pluss::series::ones_like(sum), tolerance, tolerance))
    throw std::domain_error(std::string(operation) +
                            ": mass fractions must sum to one in every cell");
  return State(flow, common::SeriesQuantity<Density>::from_si(
                         mass_fractions.mul(rho.unsqueeze(axis))));
}

[[nodiscard]] inline State
from_density(const gas::State &flow,
             const common::SeriesQuantity<Density> &partial_density,
             const spatial::PeriodicGrid &grid) {
  detail::require_partial(partial_density.si_value(), flow.density().si_value(),
                          grid, "natsci::fluids::gas::species::from_density");
  return State(flow, partial_density);
}

[[nodiscard]] inline Series mass_fractions(const State &state,
                                           const spatial::PeriodicGrid &grid) {
  return detail::fractions(state.partial_density().si_value(),
                           state.flow().density().si_value(), grid);
}

struct Options final {
  gas::Options flow{};
  const chem_transport::GasTransport *transport{nullptr};
  double diffusion{0.5};
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

inline void validate(const Options &options) {
  gas::detail::validate_options(options.flow, "natsci::fluids::gas::species");
  if (!(options.diffusion > 0.0) || !(options.diffusion <= 0.5) ||
      !std::isfinite(options.diffusion))
    throw std::invalid_argument(
        "natsci::fluids::gas::species: diffusion must be finite and in "
        "(0, 0.5]");
}

inline void validate(const RunOptions &options) {
  validate(options.step);
  if (!(options.safety > 0.0) || !(options.safety <= 1.0) ||
      !std::isfinite(options.safety))
    throw std::invalid_argument(
        "natsci::fluids::gas::species::evolve_to: safety must be finite and "
        "in (0, 1]");
  if (!(options.min_step.si_value() >= 0.0) ||
      !std::isfinite(options.min_step.si_value()) ||
      !(options.max_step.si_value() > 0.0) ||
      options.max_step.si_value() < options.min_step.si_value())
    throw std::invalid_argument(
        "natsci::fluids::gas::species::evolve_to: step bounds are invalid");
  if (options.max_steps <= 0)
    throw std::invalid_argument(
        "natsci::fluids::gas::species::evolve_to: max_steps must be positive");
}

[[nodiscard]] inline common::SeriesQuantity<DiffusiveMassFlux>
diffusive_flux(const State &state, const spatial::PeriodicGrid &grid,
               const Model &model,
               const chem_transport::GasTransport &transport) {
  const auto raw = Access::raw(state);
  gas::detail::require_valid(raw.flow, grid, model,
                             "natsci::fluids::gas::species::diffusive_flux");
  detail::require_partial(raw.partial, raw.flow.rho, grid,
                          "natsci::fluids::gas::species::diffusive_flux");
  auto value = detail::diffusion(raw.flow, raw.partial, grid, model, transport);
  const std::int64_t component = detail::species_axis(raw.partial, grid) + 1;
  return common::SeriesQuantity<DiffusiveMassFlux>::from_si(
      spatial::detail::stack_components(value.fluxes, component));
}

struct Rate final {
  gas::Rate flow;
  common::SeriesQuantity<DensityRate> partial_density;
};

namespace detail {

inline RawRate evaluate(const Raw &state, const spatial::PeriodicGrid &grid,
                        const Model &model, const Options &options) {
  const auto fluxes =
      gas::detail::axis_fluxes(state.flow, grid, model, options.flow.recon,
                               options.flow.flux, options.flow.roe);
  gas::detail::Raw flow_rate = gas::detail::conv_rate(state.flow, grid, fluxes);
  const gas::detail::Raw viscous =
      options.flow.sutherland
          ? gas::detail::visc_rate(
                state.flow, grid, model,
                gas::detail::sutherland_transport(state.flow, grid, model,
                                                  *options.flow.sutherland))
          : gas::detail::visc_rate(state.flow, grid, model,
                                   options.flow.transport);
  flow_rate.mom = flow_rate.mom.add(viscous.mom);
  flow_rate.energy = flow_rate.energy.add(viscous.energy);

  Series partial_rate = advective_rate(state.partial, state.flow.rho, grid,
                                       options.flow.recon, fluxes);
  if (options.transport)
    partial_rate = partial_rate.add(diffusion_rate(
        diffusion(state.flow, state.partial, grid, model, *options.transport),
        grid));
  return {std::move(flow_rate), std::move(partial_rate)};
}

inline void require_valid(const Raw &state, const spatial::PeriodicGrid &grid,
                          const Model &model, const char *operation) {
  gas::detail::require_valid(state.flow, grid, model, operation);
  require_partial(state.partial, state.flow.rho, grid, operation);
}

inline Raw advance(const Raw &state, double width,
                   const spatial::PeriodicGrid &grid, const Model &model,
                   const Options &options, std::int64_t &evaluations) {
  ++evaluations;
  const Raw high =
      add_rate(state, evaluate(state, grid, model, options), width);
  const Series gas_valid = gas::detail::valid(high.flow, grid, model);
  const Series species_valid =
      high.partial.isfinite().logical_and(high.partial.ge(0.0));
  if (gas_valid.all().item<bool>() && species_valid.all().item<bool>())
    return high;

  Options fallback = options;
  fallback.flow.recon = Recon::Cell;
  fallback.flow.flux = FluxScheme::Rusanov;
  fallback.flow.roe = RoePolicy{};
  ++evaluations;
  const Raw low =
      add_rate(state, evaluate(state, grid, model, fallback), width);
  require_valid(low, grid, model,
                "natsci::fluids::gas::species::step positivity fallback");

  Series gas_fraction =
      gas::detail::admissible_fraction(low.flow, high.flow, grid, model);
  gas_fraction = reduce_trailing(gas_fraction, grid.dim());
  const Series theta =
      gas_fraction.minimum(species_fraction(low.partial, high.partial, grid));
  const Series flow_theta = expand_trailing(theta, grid.dim());
  const Series species_theta = expand_trailing(theta, grid.dim() + 1);
  Raw limited{
      gas::detail::convex_mix(low.flow, high.flow, flow_theta,
                              high.flow.mom.dim() - grid.dim() - 1),
      low.partial.add(high.partial.sub(low.partial).mul(species_theta))};
  require_valid(limited, grid, model,
                "natsci::fluids::gas::species::step positivity limiter");
  return limited;
}

} // namespace detail

[[nodiscard]] inline Rate rate(const State &state,
                               const spatial::PeriodicGrid &grid,
                               const Model &model = {},
                               const Options &options = {}) {
  validate(options);
  const auto raw = Access::raw(state);
  detail::require_valid(raw, grid, model, "natsci::fluids::gas::species::rate");
  auto value = detail::evaluate(raw, grid, model, options);
  return {
      {common::SeriesQuantity<DensityRate>::from_si(std::move(value.flow.rho)),
       common::SeriesQuantity<MomRate>::from_si(std::move(value.flow.mom)),
       common::SeriesQuantity<EnergyRate>::from_si(
           std::move(value.flow.energy))},
      common::SeriesQuantity<DensityRate>::from_si(std::move(value.partial))};
}

struct Limits final {
  common::SeriesQuantity<common::Time> flow;
  common::SeriesQuantity<common::Time> diffusion;
  common::SeriesQuantity<common::Time> allowed;
};

[[nodiscard]] inline Limits limits(const State &state,
                                   const spatial::PeriodicGrid &grid,
                                   const Model &model = {},
                                   const Options &options = {}) {
  validate(options);
  const auto gas_limit =
      options.flow.sutherland
          ? gas::limits(state.flow(), grid, model, *options.flow.sutherland,
                        options.flow.courant, options.flow.diffusion)
                .allowed
          : gas::limits(state.flow(), grid, model, options.flow.transport,
                        options.flow.courant, options.flow.diffusion)
                .allowed;
  if (!options.transport) {
    const Series infinite = pluss::series::full_like(
        gas_limit.si_value(), std::numeric_limits<double>::infinity());
    return {gas_limit, common::SeriesQuantity<common::Time>::from_si(infinite),
            gas_limit};
  }

  const auto raw = Access::raw(state);
  const auto diff =
      detail::diffusion(raw.flow, raw.partial, grid, model, *options.transport);
  double inverse_h2 = 0.0;
  for (std::int64_t axis = 0; axis < grid.dim(); ++axis) {
    const double width = grid.spacing(axis).si_value();
    inverse_h2 += 1.0 / (width * width);
  }
  Series rate = diff.maximum.mul(inverse_h2);
  rate = detail::max_trailing(rate, grid.dim());
  const Series diff_limit =
      pluss::codem::make_scalar_like(rate, options.diffusion).div(rate);
  const Series allowed = gas_limit.si_value().minimum(diff_limit);
  return {gas_limit, common::SeriesQuantity<common::Time>::from_si(diff_limit),
          common::SeriesQuantity<common::Time>::from_si(allowed)};
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
  constexpr const char *operation = "natsci::fluids::gas::species::step";
  validate(options);
  if (!(width.si_value() > 0.0) || !std::isfinite(width.si_value()))
    throw std::invalid_argument(std::string(operation) +
                                ": width must be finite and positive");
  const auto limit = limits(state, grid, model, options).allowed;
  if (options.flow.enforce &&
      !limit.si_value().ge(width.si_value()).all().item<bool>())
    throw std::domain_error(std::string(operation) +
                            ": width exceeds the acoustic or diffusion "
                            "stability limit");

  const detail::Raw initial = Access::raw(state);
  detail::require_valid(initial, grid, model, operation);
  std::int64_t evaluations = 0;
  const auto advance = [&](const detail::Raw &at) {
    return detail::advance(at, width.si_value(), grid, model, options,
                           evaluations);
  };

  detail::Raw next;
  if (options.flow.rule == Rule::Euler) {
    next = advance(initial);
  } else if (options.flow.rule == Rule::Rk2) {
    const detail::Raw first = advance(initial);
    next = detail::blend(initial, advance(first), 0.5, 0.5);
  } else {
    const detail::Raw first = advance(initial);
    const detail::Raw second =
        detail::blend(initial, advance(first), 0.75, 0.25);
    detail::require_valid(second, grid, model,
                          "natsci::fluids::gas::species::step stage 2");
    next = detail::blend(initial, advance(second), 1.0 / 3.0, 2.0 / 3.0);
  }
  detail::require_valid(next, grid, model, operation);
  return {Access::make(std::move(next)), limit, evaluations};
}

namespace detail {

inline common::StateVector record(const State &state) {
  return common::states(
      common::field("density", state.flow().density(), "rho"),
      common::field("momentum", state.flow().momentum(), "rho u"),
      common::field("energy", state.flow().energy(), "E"),
      common::field("species_density", state.partial_density(), "rho Y"));
}

inline State state(const common::StateVector &record,
                   const spatial::PeriodicGrid &grid, const Model &model) {
  const gas::State flow = gas::make(
      record.get<Density>("density"), record.get<MomDensity>("momentum"),
      record.get<EnergyDensity>("energy"), grid, model);
  return from_density(flow, record.get<Density>("species_density"), grid);
}

} // namespace detail

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
  validate(options);
  std::int64_t evaluations = 0;
  auto marched = common::solve::march(
      [&](common::ScalarQuantity<common::Time>,
          const common::StateVector &record,
          common::ScalarQuantity<common::Time> step_width) {
        auto advanced = species::step(detail::state(record, grid, model), grid,
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
  validate(options);
  std::int64_t evaluations = 0;
  auto marched = common::solve::march_to(
      [&](common::ScalarQuantity<common::Time>,
          const common::StateVector &record,
          common::ScalarQuantity<common::Time> width) {
        auto advanced = species::step(detail::state(record, grid, model), grid,
                                      width, model, options.step);
        evaluations += advanced.flux_evaluations;
        return detail::record(advanced.state);
      },
      [&](common::ScalarQuantity<common::Time> time,
          const common::StateVector &record) {
        const State at = detail::state(record, grid, model);
        const double stable = limits(at, grid, model, options.step)
                                  .allowed.si_value()
                                  .min()
                                  .item<double>();
        const double remaining = stop.si_value() - time.si_value();
        const double width =
            std::min(remaining, std::min(options.max_step.si_value(),
                                         options.safety * stable));
        if (!(width > 0.0) || !std::isfinite(width))
          throw std::domain_error(
              "natsci::fluids::gas::species::evolve_to: stability policy "
              "produced no finite positive step");
        if (width < options.min_step.si_value() &&
            remaining > options.min_step.si_value())
          throw std::domain_error(
              "natsci::fluids::gas::species::evolve_to: stable step is below "
              "min_step");
        return common::ScalarQuantity<common::Time>::from_si(width);
      },
      detail::record(initial), start, stop, options.max_steps);
  return {std::move(marched.trajectory), marched.steps, evaluations};
}

} // namespace pluss::subsp::natsci::physics::fluids::gas::species
