#pragma once

#include "pluss/codem/series_factory.h"
#include "pluss/subsp/math/diff.h"
#include "pluss/subsp/natsci/physics/mechanics/state.h"

#include <cmath>
#include <concepts>
#include <functional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace pluss::subsp::natsci::physics::mechanics {

namespace hamiltonian_detail {

namespace math_diff = ::pluss::subsp::math::diff;

template <class T> struct quantity_result final {
  static constexpr bool valid = false;
};

template <common::DimensionType D, common::QuantityRep Rep>
struct quantity_result<common::Quantity<D, Rep>> final {
  static constexpr bool valid = true;
  using dimension = D;
  using rep = Rep;
};

inline math_diff::DerivativePolicy
derivative_policy(const VariationalOptions &options) {
  math_diff::DerivativePolicy policy;
  // A free Hamiltonian may be independent of q, while an autonomous
  // Lagrangian is independent of time. Those are mathematical zeros.
  policy.strict_dependency = false;
  policy.warn_on_disconnected = false;
  policy.keep_graph = options.keep_graph;
  policy.track_higher_order = options.track_higher_order;
  return policy;
}

inline Series batch_time(const Series &coordinate,
                         common::ScalarQuantity<common::Time> time) {
  return pluss::series::full(detail::batch_shape(coordinate), time.si_value(),
                             Config{}
                                 .with_dtype(coordinate.scalar_type())
                                 .with_platform(coordinate.platform()));
}

template <class Fn, common::DimensionType CoordinateDimension>
[[nodiscard]] common::SeriesQuantity<common::Energy>
invoke_energy(Fn &fn, const Series &coordinate, const Series &momentum,
              const Series &time) {
  using Coordinate = common::SeriesQuantity<CoordinateDimension>;
  using Momentum =
      common::SeriesQuantity<CanonicalMomentum<CoordinateDimension>>;
  using Time = common::SeriesQuantity<common::Time>;
  using Result = std::invoke_result_t<Fn &, const Coordinate &,
                                      const Momentum &, const Time &>;
  using Traits = quantity_result<std::remove_cvref_t<Result>>;
  static_assert(Traits::valid, "Hamiltonian callback must return a Quantity");
  static_assert(std::same_as<typename Traits::dimension, common::Energy>,
                "Hamiltonian callback must return energy");
  static_assert(std::same_as<typename Traits::rep, Series>,
                "Hamiltonian callback must return a Series quantity");
  return std::invoke(fn, Coordinate::from_si(coordinate),
                     Momentum::from_si(momentum), Time::from_si(time));
}

template <class Fn, common::DimensionType InputDimension>
[[nodiscard]] common::SeriesQuantity<common::Energy>
invoke_single_energy(Fn &fn, const Series &input, const Series &time) {
  using Input = common::SeriesQuantity<InputDimension>;
  using Time = common::SeriesQuantity<common::Time>;
  using Result = std::invoke_result_t<Fn &, const Input &, const Time &>;
  using Traits = quantity_result<std::remove_cvref_t<Result>>;
  static_assert(Traits::valid,
                "separable Hamiltonian callback must return a Quantity");
  static_assert(std::same_as<typename Traits::dimension, common::Energy>,
                "separable Hamiltonian callback must return energy");
  static_assert(std::same_as<typename Traits::rep, Series>,
                "separable Hamiltonian callback must return a Series quantity");
  return std::invoke(fn, Input::from_si(input), Time::from_si(time));
}

inline void require_energy_layout(const Series &energy,
                                  const Series &coordinate,
                                  const char *operation) {
  detail::require_variational_value(energy, operation, "callback output");
  if (energy.scalar_type() != coordinate.scalar_type())
    throw std::invalid_argument(std::string(operation) +
                                ": callback output dtype must match state");
  if (energy.platform() != coordinate.platform())
    throw std::invalid_argument(std::string(operation) +
                                ": callback output platform must match state");

  const auto batch = detail::batch_shape(coordinate);
  if (energy.dim() == 0)
    return;
  if (energy.dim() != static_cast<std::int64_t>(batch.size()))
    throw std::invalid_argument(
        std::string(operation) +
        ": callback output must be scalar or have the state batch shape");
  for (std::int64_t axis = 0; axis < energy.dim(); ++axis)
    if (energy.size(axis) != batch[static_cast<std::size_t>(axis)])
      throw std::invalid_argument(
          std::string(operation) +
          ": callback output must be scalar or have the state batch shape");
}

template <class Fn, common::DimensionType InputDimension>
[[nodiscard]] Series
energy_gradient(Fn &fn, const Series &input, const Series &time,
                const VariationalOptions &options, const char *operation) {
  Series displayed = pluss::series::undefined();
  auto [value, gradients] = math_diff::value_and_grad(
      [&](const std::vector<math_diff::Trace> &variables) {
        const auto result = invoke_single_energy<Fn, InputDimension>(
            fn, variables.front().value(), time);
        displayed = result.si_value();
        return math_diff::Trace(displayed.sum());
      },
      std::vector<Series>{input}, derivative_policy(options));
  (void)value;
  require_energy_layout(displayed, input, operation);
  return gradients.front();
}

} // namespace hamiltonian_detail

template <common::DimensionType CoordinateDimension>
struct HamiltonianEvaluation final {
  common::SeriesQuantity<common::Energy> value;
  PhaseRate<CoordinateDimension> flow;
};

template <common::DimensionType CoordinateDimension, class Fn>
[[nodiscard]] HamiltonianEvaluation<CoordinateDimension>
hamiltonian(Fn &&fn, const PhaseState<CoordinateDimension> &state,
            common::ScalarQuantity<common::Time> time =
                common::ScalarQuantity<common::Time>::from_si(0.0),
            const VariationalOptions &options = {}) {
  using FnType = std::decay_t<Fn>;
  FnType fn_copy(std::forward<Fn>(fn));
  const Series &q = state.coordinate().si_value();
  const Series &p = state.momentum().si_value();
  const Series t = hamiltonian_detail::batch_time(q, time);
  Series displayed = pluss::series::undefined();

  auto [total, gradients] = hamiltonian_detail::math_diff::value_and_grad(
      [&](const std::vector<hamiltonian_detail::math_diff::Trace> &variables) {
        const auto value =
            hamiltonian_detail::invoke_energy<FnType, CoordinateDimension>(
                fn_copy, variables[0].value(), variables[1].value(), t);
        displayed = value.si_value();
        return hamiltonian_detail::math_diff::Trace(displayed.sum());
      },
      std::vector<Series>{q, p},
      hamiltonian_detail::derivative_policy(options));
  (void)total;
  hamiltonian_detail::require_energy_layout(displayed, q,
                                            "natsci::mechanics::hamiltonian");

  return {common::SeriesQuantity<common::Energy>::from_si(std::move(displayed)),
          {common::SeriesQuantity<CoordinateRate<CoordinateDimension>>::from_si(
               gradients[1]),
           common::SeriesQuantity<CanonicalForce<CoordinateDimension>>::from_si(
               gradients[0].neg())}};
}

template <common::DimensionType CoordinateDimension, class PotentialFn,
          class KineticFn>
[[nodiscard]] PhaseState<CoordinateDimension>
symplectic_step(const PhaseState<CoordinateDimension> &state,
                common::ScalarQuantity<common::Time> step,
                PotentialFn &&potential, KineticFn &&kinetic,
                common::ScalarQuantity<common::Time> time =
                    common::ScalarQuantity<common::Time>::from_si(0.0),
                const VariationalOptions &options = {}) {
  if (!std::isfinite(step.si_value()))
    throw std::invalid_argument(
        "natsci::mechanics::symplectic_step: step must be finite");
  if (step.si_value() == 0.0)
    return state;

  using Potential = std::decay_t<PotentialFn>;
  using Kinetic = std::decay_t<KineticFn>;
  Potential potential_copy(std::forward<PotentialFn>(potential));
  Kinetic kinetic_copy(std::forward<KineticFn>(kinetic));

  const Series &q = state.coordinate().si_value();
  const Series &p = state.momentum().si_value();
  const double dt = step.si_value();
  const Series t0 = hamiltonian_detail::batch_time(q, time);
  const Series tmid = hamiltonian_detail::batch_time(
      q, common::ScalarQuantity<common::Time>::from_si(time.si_value() +
                                                       0.5 * dt));
  const Series t1 = hamiltonian_detail::batch_time(
      q, common::ScalarQuantity<common::Time>::from_si(time.si_value() + dt));

  const Series first_force =
      hamiltonian_detail::energy_gradient<Potential, CoordinateDimension>(
          potential_copy, q, t0, options,
          "natsci::mechanics::symplectic_step potential");
  const Series half_p = p.sub(first_force.mul(0.5 * dt));
  const Series speed = hamiltonian_detail::energy_gradient<
      Kinetic, CanonicalMomentum<CoordinateDimension>>(
      kinetic_copy, half_p, tmid, options,
      "natsci::mechanics::symplectic_step kinetic");
  const Series next_q = q.add(speed.mul(dt));
  const Series second_force =
      hamiltonian_detail::energy_gradient<Potential, CoordinateDimension>(
          potential_copy, next_q, t1, options,
          "natsci::mechanics::symplectic_step potential");
  const Series next_p = half_p.sub(second_force.mul(0.5 * dt));

  return make_phase(
      common::SeriesQuantity<CoordinateDimension>::from_si(next_q),
      common::SeriesQuantity<CanonicalMomentum<CoordinateDimension>>::from_si(
          next_p));
}

} // namespace pluss::subsp::natsci::physics::mechanics
