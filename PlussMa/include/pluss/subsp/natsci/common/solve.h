#pragma once

#include "pluss/subsp/math/integ.h"
#include "pluss/subsp/math/interp.h"
#include "pluss/subsp/math/ode.h"
#include "pluss/subsp/math/optim.h"
#include "pluss/subsp/natsci/common/record.h"
#include "pluss/subsp/natsci/common/trajectory.h"

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace pluss::subsp::natsci::common::solve {

namespace optim = pluss::subsp::math::optim;

namespace detail {

inline bool same_block_layout(const optim::ParameterBlockLayout &lhs,
                              const optim::ParameterBlockLayout &rhs) {
  return lhs.name == rhs.name && lhs.sizes == rhs.sizes &&
         lhs.scalar_type == rhs.scalar_type && lhs.platform == rhs.platform;
}

struct RecordAccess final {
  struct LinearBounds final {
    std::optional<Series> matrix;
    std::optional<Series> rhs;
  };

  [[nodiscard]] static StateVector state(const StateVector &model,
                                         const Series &flat) {
    return StateVector(model.store_.repack(flat));
  }

  [[nodiscard]] static StateVector
  stack(const std::vector<StateVector> &sequence) {
    if (sequence.empty())
      throw std::invalid_argument(
          "natsci::solve::march: expected at least one state");
    std::vector<::pluss::subsp::natsci::common::detail::RecordStore> stores;
    stores.reserve(sequence.size());
    for (const auto &state : sequence)
      stores.push_back(state.store_);
    return StateVector(sequence.front().store_.stack(stores));
  }

  [[nodiscard]] static ParameterSet parameters(const ParameterSet &model,
                                               const Series &flat) {
    return ParameterSet(model.store_.repack(flat), model.bounds_, false);
  }

  [[nodiscard]] static Series scale(const ParameterSet &model,
                                    const StateVector &scales) {
    if (model.size() != scales.size())
      throw std::invalid_argument(
          "natsci::solve::fit: scale field count mismatch");
    const auto &model_blocks = model.layout().blocks();
    const auto &scale_blocks = scales.layout().blocks();
    for (std::size_t i = 0; i < model.size(); ++i) {
      if (!same_block_layout(model_blocks[i], scale_blocks[i]) ||
          model.meta()[i].dimension != scales.meta()[i].dimension)
        throw std::invalid_argument(
            "natsci::solve::fit: scales must match parameter names, "
            "dimensions, shapes, dtypes, and platforms");
    }
    const Series flat = scales.flat();
    if (!flat.isfinite().all().item<bool>() || !flat.gt(0.0).all().item<bool>())
      throw std::invalid_argument(
          "natsci::solve::fit: scales must be finite and strictly positive");
    return flat;
  }

  [[nodiscard]] static LinearBounds linear_bounds(const ParameterSet &model,
                                                  const Series &scale) {
    const std::int64_t count = model.layout().total_numel();
    const Series identity =
        pluss::series::eye(count, scale.scalar_type()).to(scale.platform());
    std::vector<Series> rows;
    std::vector<Series> rhs;
    rows.reserve(model.bounds_->size() * 2);
    rhs.reserve(model.bounds_->size() * 2);
    std::int64_t offset = 0;
    for (std::size_t i = 0; i < model.bounds_->size(); ++i) {
      const auto &bound = (*model.bounds_)[i];
      const auto width = model.layout().blocks()[i].numel;
      const Series basis = identity.slice(0, offset, offset + width, 1);
      const Series block_scale = scale.slice(0, offset, offset + width, 1);
      if (bound.lower) {
        rows.push_back(basis.neg());
        rhs.push_back(bound.lower->reshape({width}).div(block_scale).neg());
      }
      if (bound.upper) {
        rows.push_back(basis);
        rhs.push_back(bound.upper->reshape({width}).div(block_scale));
      }
      offset += width;
    }
    if (rows.empty())
      return {};
    Series matrix = std::move(rows.front());
    Series bound = std::move(rhs.front());
    if (rows.size() > 1) {
      matrix = matrix.unite(
          std::vector<Series>(std::make_move_iterator(rows.begin() + 1),
                              std::make_move_iterator(rows.end())),
          0);
      bound = bound.unite(
          std::vector<Series>(std::make_move_iterator(rhs.begin() + 1),
                              std::make_move_iterator(rhs.end())),
          0);
    }
    return {std::move(matrix), std::move(bound)};
  }
};

template <class T> struct series_quantity : std::false_type {};
template <DimensionType D>
struct series_quantity<SeriesQuantity<D>> : std::true_type {
  using dimension = D;
};
template <class T>
inline constexpr bool series_quantity_v =
    series_quantity<std::remove_cvref_t<T>>::value;

inline void require_rate(const StateVector &rate, const StateVector &state,
                         DimensionSignature coordinate) {
  if (rate.size() != state.size())
    throw std::invalid_argument(
        "natsci::solve::flow: derivative field count mismatch");
  const auto &rate_blocks = rate.layout().blocks();
  const auto &state_blocks = state.layout().blocks();
  for (std::size_t i = 0; i < state.size(); ++i) {
    if (!same_block_layout(rate_blocks[i], state_blocks[i]))
      throw std::invalid_argument(
          "natsci::solve::flow: derivative names, shapes, dtypes, and "
          "platforms must match the state");
    DimensionSignature expected = state.meta()[i].dimension;
    for (std::size_t axis = 0; axis < expected.powers.size(); ++axis)
      expected.powers[axis] -= coordinate.powers[axis];
    if (rate.meta()[i].dimension != expected)
      throw std::invalid_argument("natsci::solve::flow: derivative field has "
                                  "the wrong physical dimension");
  }
}

inline optim::ParameterPack<Series>
zero_jacobian(const optim::ParameterPack<Series> &coordinates,
              std::int64_t rows) {
  std::vector<optim::ParameterPack<Series>::Block> blocks;
  blocks.reserve(coordinates.size());
  for (const auto &block : coordinates.blocks()) {
    blocks.push_back(
        {block.name, pluss::series::zeros({rows, block.value.numel()},
                                          block.value.scalar_type())
                         .to(block.value.platform())});
  }
  return optim::ParameterPack<Series>(std::move(blocks));
}

} // namespace detail

struct FlowResult final {
  StateVector state;
  std::int64_t steps{0};
  std::int64_t evaluations{0};
  bool converged{false};
};

template <DimensionType CoordinateDimension> struct MarchResult final {
  Trajectory<CoordinateDimension> trajectory;
  std::int64_t steps{0};
};

// Sequential constrained systems (projection, contact, normalization, etc.)
// supply one state-to-state step. C3 owns coordinate construction, schema
// validation, and trajectory assembly so domain modules do not duplicate it.
template <DimensionType CoordinateDimension, class StepFn>
  requires std::invocable<StepFn &, ScalarQuantity<CoordinateDimension>,
                          const StateVector &,
                          ScalarQuantity<CoordinateDimension>> &&
           std::same_as<
               std::remove_cvref_t<std::invoke_result_t<
                   StepFn &, ScalarQuantity<CoordinateDimension>,
                   const StateVector &, ScalarQuantity<CoordinateDimension>>>,
               StateVector>
[[nodiscard]] MarchResult<CoordinateDimension>
march(StepFn step_fn, const StateVector &initial,
      ScalarQuantity<CoordinateDimension> start,
      ScalarQuantity<CoordinateDimension> step, std::int64_t steps) {
  if (!(step.si_value() > 0.0) || !std::isfinite(step.si_value()))
    throw std::invalid_argument(
        "natsci::solve::march: step must be finite and positive");
  if (!std::isfinite(start.si_value()))
    throw std::invalid_argument("natsci::solve::march: start must be finite");
  if (steps < 0)
    throw std::invalid_argument(
        "natsci::solve::march: steps must be non-negative");
  if (steps == std::numeric_limits<std::int64_t>::max())
    throw std::overflow_error("natsci::solve::march: step count overflows");

  std::vector<StateVector> states;
  states.reserve(static_cast<std::size_t>(steps) + 1);
  states.push_back(initial);
  for (std::int64_t index = 0; index < steps; ++index) {
    const auto coordinate = ScalarQuantity<CoordinateDimension>::from_si(
        start.si_value() + static_cast<double>(index) * step.si_value());
    states.push_back(std::invoke(step_fn, coordinate, states.back(), step));
  }

  const Series &prototype = initial.flat();
  Series coordinate =
      pluss::series::arange(0, steps + 1, 1, prototype.scalar_type())
          .to(prototype.platform())
          .mul(pluss::codem::make_scalar_like(prototype, step.si_value()))
          .add(pluss::codem::make_scalar_like(prototype, start.si_value()));
  return {
      ::pluss::subsp::natsci::common::path(
          SeriesQuantity<CoordinateDimension>::from_si(std::move(coordinate)),
          detail::RecordAccess::stack(states)),
      steps};
}

// March a sequential constrained system to an exact terminal coordinate while
// allowing the domain policy to choose each accepted width. The state update
// remains inherently sequential; trajectory packing is deferred until every
// accepted state has been collected.
template <DimensionType CoordinateDimension, class StepFn, class WidthFn>
  requires std::invocable<StepFn &, ScalarQuantity<CoordinateDimension>,
                          const StateVector &,
                          ScalarQuantity<CoordinateDimension>> &&
           std::same_as<
               std::remove_cvref_t<std::invoke_result_t<
                   StepFn &, ScalarQuantity<CoordinateDimension>,
                   const StateVector &, ScalarQuantity<CoordinateDimension>>>,
               StateVector> &&
           std::invocable<WidthFn &, ScalarQuantity<CoordinateDimension>,
                          const StateVector &> &&
           std::same_as<std::remove_cvref_t<std::invoke_result_t<
                            WidthFn &, ScalarQuantity<CoordinateDimension>,
                            const StateVector &>>,
                        ScalarQuantity<CoordinateDimension>>
[[nodiscard]] MarchResult<CoordinateDimension>
march_to(StepFn step_fn, WidthFn width_fn, const StateVector &initial,
         ScalarQuantity<CoordinateDimension> start,
         ScalarQuantity<CoordinateDimension> stop,
         std::int64_t max_steps = 100'000) {
  constexpr const char *operation = "natsci::solve::march_to";
  if (!std::isfinite(start.si_value()) || !std::isfinite(stop.si_value()) ||
      !(stop.si_value() > start.si_value()))
    throw std::invalid_argument(
        "natsci::solve::march_to: stop must be finite and greater than start");
  if (max_steps <= 0)
    throw std::invalid_argument(
        "natsci::solve::march_to: max_steps must be positive");

  std::vector<StateVector> states;
  std::vector<double> coordinates;
  states.reserve(static_cast<std::size_t>(std::min<std::int64_t>(
                     max_steps, static_cast<std::int64_t>(4096))) +
                 1);
  coordinates.reserve(states.capacity());
  states.push_back(initial);
  coordinates.push_back(start.si_value());

  double coordinate = start.si_value();
  std::int64_t completed = 0;
  while (coordinate < stop.si_value() && completed < max_steps) {
    const auto at = ScalarQuantity<CoordinateDimension>::from_si(coordinate);
    const auto proposed = std::invoke(width_fn, at, states.back());
    if (!(proposed.si_value() > 0.0) || !std::isfinite(proposed.si_value()))
      throw std::domain_error(std::string(operation) +
                              ": width policy returned a non-positive or "
                              "non-finite step");
    const double width =
        std::min(proposed.si_value(), stop.si_value() - coordinate);
    const double next = coordinate + width;
    if (!(next > coordinate) || !std::isfinite(next))
      throw std::overflow_error(
          "natsci::solve::march_to: coordinate cannot advance at machine "
          "precision");
    const auto accepted = ScalarQuantity<CoordinateDimension>::from_si(width);
    states.push_back(std::invoke(step_fn, at, states.back(), accepted));
    coordinate = next;
    coordinates.push_back(coordinate);
    ++completed;
  }
  if (coordinate < stop.si_value())
    throw std::runtime_error(
        "natsci::solve::march_to: max_steps reached before stop");

  const Series &prototype = initial.flat();
  Series coordinate_series =
      pluss::series::make(coordinates, prototype.scalar_type())
          .to(prototype.platform());
  return {::pluss::subsp::natsci::common::path(
              SeriesQuantity<CoordinateDimension>::from_si(
                  std::move(coordinate_series)),
              detail::RecordAccess::stack(states)),
          completed};
}

template <DimensionType CoordinateDimension> struct FlowEventResult final {
  StateVector state;
  ScalarQuantity<CoordinateDimension> coordinate;
  SeriesQuantity<Dimensionless> event_value;
  std::int64_t steps{0};
  std::int64_t evaluations{0};
  std::int64_t event_evaluations{0};
  bool triggered{false};
  bool converged{false};
};

template <DimensionType CoordinateDimension, class Fn>
  requires std::invocable<Fn &, ScalarQuantity<CoordinateDimension>,
                          const StateVector &>
[[nodiscard]] FlowResult
flow(Fn fn, const StateVector &initial,
     ScalarQuantity<CoordinateDimension> start,
     ScalarQuantity<CoordinateDimension> stop,
     const pluss::subsp::math::ode::OdeOptions &options = {}) {
  const auto coordinate_dimension =
      DimensionSignature::of<CoordinateDimension>();
  const auto state_model = std::make_shared<const StateVector>(initial);
  const auto raw = pluss::subsp::math::ode::fluxa(
      [fn = std::move(fn), state_model, coordinate_dimension](
          double coordinate, const Series &flat, const Series &) mutable {
        const StateVector state =
            detail::RecordAccess::state(*state_model, flat);
        StateVector rate = std::invoke(
            fn, ScalarQuantity<CoordinateDimension>::from_si(coordinate),
            state);
        detail::require_rate(rate, state, coordinate_dimension);
        return rate.flat();
      },
      initial.flat(), start.si_value(), stop.si_value(),
      pluss::series::undefined(), options);
  return {detail::RecordAccess::state(initial, raw.y_final), raw.steps,
          raw.function_evaluations, raw.converged};
}

template <DimensionType CoordinateDimension, class Fn>
  requires std::invocable<Fn &, ScalarQuantity<CoordinateDimension>,
                          const StateVector &, const ParameterSet &>
[[nodiscard]] FlowResult
flow(Fn fn, const StateVector &initial, const ParameterSet &parameters,
     ScalarQuantity<CoordinateDimension> start,
     ScalarQuantity<CoordinateDimension> stop,
     const pluss::subsp::math::ode::OdeOptions &options = {}) {
  const auto coordinate_dimension =
      DimensionSignature::of<CoordinateDimension>();
  const auto state_model = std::make_shared<const StateVector>(initial);
  const auto parameter_model = std::make_shared<const ParameterSet>(parameters);
  const auto raw = pluss::subsp::math::ode::fluxa(
      [fn = std::move(fn), state_model, parameter_model, coordinate_dimension](
          double coordinate, const Series &flat, const Series &theta) mutable {
        const StateVector state =
            detail::RecordAccess::state(*state_model, flat);
        const ParameterSet current_parameters =
            detail::RecordAccess::parameters(*parameter_model, theta);
        StateVector rate = std::invoke(
            fn, ScalarQuantity<CoordinateDimension>::from_si(coordinate), state,
            current_parameters);
        detail::require_rate(rate, state, coordinate_dimension);
        return rate.flat();
      },
      initial.flat(), start.si_value(), stop.si_value(), parameters.flat(),
      options);
  return {detail::RecordAccess::state(initial, raw.y_final), raw.steps,
          raw.function_evaluations, raw.converged};
}

template <DimensionType CoordinateDimension, class Fn, class EventFn>
  requires std::invocable<Fn &, ScalarQuantity<CoordinateDimension>,
                          const StateVector &> &&
           std::invocable<EventFn &, ScalarQuantity<CoordinateDimension>,
                          const StateVector &> &&
           detail::series_quantity_v<std::invoke_result_t<
               EventFn &, ScalarQuantity<CoordinateDimension>,
               const StateVector &>> &&
           std::same_as<typename detail::series_quantity<
                            std::remove_cvref_t<std::invoke_result_t<
                                EventFn &, ScalarQuantity<CoordinateDimension>,
                                const StateVector &>>>::dimension,
                        Dimensionless>
[[nodiscard]] FlowEventResult<CoordinateDimension>
flow_until(Fn fn, EventFn event, const StateVector &initial,
           ScalarQuantity<CoordinateDimension> start,
           ScalarQuantity<CoordinateDimension> stop,
           const pluss::subsp::math::ode::OdeOptions &options = {},
           const pluss::subsp::math::ode::OdeEventOptions &event_options = {}) {
  const auto coordinate_dimension =
      DimensionSignature::of<CoordinateDimension>();
  const auto state_model = std::make_shared<const StateVector>(initial);
  const auto raw = pluss::subsp::math::ode::fluxa_until(
      [fn = std::move(fn), state_model, coordinate_dimension](
          double coordinate, const Series &flat, const Series &) mutable {
        const StateVector state =
            detail::RecordAccess::state(*state_model, flat);
        StateVector rate = std::invoke(
            fn, ScalarQuantity<CoordinateDimension>::from_si(coordinate),
            state);
        detail::require_rate(rate, state, coordinate_dimension);
        return rate.flat();
      },
      [event = std::move(event), state_model](
          double coordinate, const Series &flat, const Series &) mutable {
        const StateVector state =
            detail::RecordAccess::state(*state_model, flat);
        return std::invoke(
                   event,
                   ScalarQuantity<CoordinateDimension>::from_si(coordinate),
                   state)
            .si_value();
      },
      initial.flat(), start.si_value(), stop.si_value(),
      pluss::series::undefined(), options, event_options);
  return {detail::RecordAccess::state(initial, raw.y_final),
          ScalarQuantity<CoordinateDimension>::from_si(raw.t_final),
          SeriesQuantity<Dimensionless>::from_si(raw.event_value),
          raw.steps,
          raw.function_evaluations,
          raw.event_evaluations,
          raw.triggered,
          raw.converged};
}

template <DimensionType CoordinateDimension, class Fn, class EventFn>
  requires std::invocable<Fn &, ScalarQuantity<CoordinateDimension>,
                          const StateVector &, const ParameterSet &> &&
           std::invocable<EventFn &, ScalarQuantity<CoordinateDimension>,
                          const StateVector &, const ParameterSet &> &&
           detail::series_quantity_v<std::invoke_result_t<
               EventFn &, ScalarQuantity<CoordinateDimension>,
               const StateVector &, const ParameterSet &>> &&
           std::same_as<
               typename detail::series_quantity<
                   std::remove_cvref_t<std::invoke_result_t<
                       EventFn &, ScalarQuantity<CoordinateDimension>,
                       const StateVector &, const ParameterSet &>>>::dimension,
               Dimensionless>
[[nodiscard]] FlowEventResult<CoordinateDimension>
flow_until(Fn fn, EventFn event, const StateVector &initial,
           const ParameterSet &parameters,
           ScalarQuantity<CoordinateDimension> start,
           ScalarQuantity<CoordinateDimension> stop,
           const pluss::subsp::math::ode::OdeOptions &options = {},
           const pluss::subsp::math::ode::OdeEventOptions &event_options = {}) {
  const auto coordinate_dimension =
      DimensionSignature::of<CoordinateDimension>();
  const auto state_model = std::make_shared<const StateVector>(initial);
  const auto parameter_model = std::make_shared<const ParameterSet>(parameters);
  const auto raw = pluss::subsp::math::ode::fluxa_until(
      [fn = std::move(fn), state_model, parameter_model, coordinate_dimension](
          double coordinate, const Series &flat, const Series &theta) mutable {
        const StateVector state =
            detail::RecordAccess::state(*state_model, flat);
        const ParameterSet current =
            detail::RecordAccess::parameters(*parameter_model, theta);
        StateVector rate = std::invoke(
            fn, ScalarQuantity<CoordinateDimension>::from_si(coordinate), state,
            current);
        detail::require_rate(rate, state, coordinate_dimension);
        return rate.flat();
      },
      [event = std::move(event), state_model, parameter_model](
          double coordinate, const Series &flat, const Series &theta) mutable {
        const StateVector state =
            detail::RecordAccess::state(*state_model, flat);
        const ParameterSet current =
            detail::RecordAccess::parameters(*parameter_model, theta);
        return std::invoke(
                   event,
                   ScalarQuantity<CoordinateDimension>::from_si(coordinate),
                   state, current)
            .si_value();
      },
      initial.flat(), start.si_value(), stop.si_value(), parameters.flat(),
      options, event_options);
  return {detail::RecordAccess::state(initial, raw.y_final),
          ScalarQuantity<CoordinateDimension>::from_si(raw.t_final),
          SeriesQuantity<Dimensionless>::from_si(raw.event_value),
          raw.steps,
          raw.function_evaluations,
          raw.event_evaluations,
          raw.triggered,
          raw.converged};
}

template <DimensionType ResultDimension> struct QuadResult final {
  SeriesQuantity<ResultDimension> value;
  double error{0.0};
  std::int64_t evaluations{0};
  std::int64_t refinements{0};
  bool converged{false};
  pluss::subsp::math::integ::QuadraRule rule;
};

template <DimensionType CoordinateDimension, class Fn>
  requires std::invocable<Fn &, SeriesQuantity<CoordinateDimension>> &&
           detail::series_quantity_v<
               std::invoke_result_t<Fn &, SeriesQuantity<CoordinateDimension>>>
[[nodiscard]] auto
quad(Fn fn, ScalarQuantity<CoordinateDimension> lower,
     ScalarQuantity<CoordinateDimension> upper,
     const pluss::subsp::math::integ::QuadraOptions &options = {}) {
  using IntegrandQuantity = std::remove_cvref_t<
      std::invoke_result_t<Fn &, SeriesQuantity<CoordinateDimension>>>;
  using IntegrandDimension =
      typename detail::series_quantity<IntegrandQuantity>::dimension;
  using ResultDimension =
      DimensionProduct<CoordinateDimension, IntegrandDimension>;

  const auto raw = pluss::subsp::math::integ::quadra(
      [fn = std::move(fn)](const Series &coordinate) mutable {
        return std::invoke(
                   fn, SeriesQuantity<CoordinateDimension>::from_si(coordinate))
            .si_value();
      },
      lower.si_value(), upper.si_value(), options);
  return QuadResult<ResultDimension>{
      SeriesQuantity<ResultDimension>::from_si(raw.value),
      raw.error_estimate,
      raw.evaluations,
      raw.refinements,
      raw.converged,
      raw.rule_used};
}

template <DimensionType CoordinateDimension, DimensionType ValueDimension>
[[nodiscard]] SeriesQuantity<ValueDimension>
interp(const SeriesQuantity<CoordinateDimension> &coordinate,
       const SeriesQuantity<ValueDimension> &values,
       const SeriesQuantity<CoordinateDimension> &query,
       pluss::subsp::math::interp::EdgeMode edge =
           pluss::subsp::math::interp::EdgeMode::Clamp) {
  return SeriesQuantity<ValueDimension>::from_si(
      pluss::subsp::math::interp::linear(
          coordinate.si_value(), values.si_value(), query.si_value(), edge));
}

using ConstraintFn =
    std::function<SeriesQuantity<Dimensionless>(const ParameterSet &)>;

struct FitConstraints final {
  ConstraintFn equality;
  ConstraintFn inequality;
};

struct FitOptions final {
  pluss::subsp::math::optim::OptimaOptions solver{};
  FitConstraints constraints{};
};

template <DimensionType ObjectiveDimension> struct FitResult final {
  ParameterSet parameters;
  SeriesQuantity<ObjectiveDimension> objective;
  pluss::subsp::math::optim::OptimaState solver;
};

template <class Fn>
  requires std::invocable<Fn &, const ParameterSet &> &&
           detail::series_quantity_v<
               std::invoke_result_t<Fn &, const ParameterSet &>>
[[nodiscard]] auto fit(Fn fn, const ParameterSet &initial,
                       const StateVector &scales,
                       const FitOptions &options = {}) {
  using ObjectiveQuantity =
      std::remove_cvref_t<std::invoke_result_t<Fn &, const ParameterSet &>>;
  using ObjectiveDimension =
      typename detail::series_quantity<ObjectiveQuantity>::dimension;
  const Series scale = detail::RecordAccess::scale(initial, scales);
  const auto normalized =
      optim::unflatten_values(initial.layout(), initial.flat().div(scale));
  const auto model = std::make_shared<const ParameterSet>(initial);

  optim::ConstrainedProblem problem;
  const auto bounds = detail::RecordAccess::linear_bounds(initial, scale);
  problem.linear.Aineq = bounds.matrix;
  problem.linear.bineq = bounds.rhs;
  problem.objective.analytic =
      [fn = std::move(fn), model,
       scale](const optim::ParameterPack<Series> &coordinates) mutable {
        const Series flat = optim::flatten_values(coordinates).detach();
        auto [value, gradient] = pluss::subsp::math::diff::value_and_grad(
            [&](const pluss::subsp::math::diff::Trace &normalized_coordinate) {
              const ParameterSet physical = detail::RecordAccess::parameters(
                  *model, normalized_coordinate.value().mul(scale));
              const Series objective = std::invoke(fn, physical).si_value();
              if (objective.numel() != 1)
                throw std::invalid_argument(
                    "natsci::solve::fit: objective must return one value");
              return pluss::subsp::math::diff::Trace(objective.reshape({}));
            },
            flat);
        return std::pair{
            std::move(value),
            optim::unflatten_values(coordinates.layout(), std::move(gradient))};
      };

  if (options.constraints.equality || options.constraints.inequality) {
    problem.nonlinear
        .analytic = [equality = options.constraints.equality,
                     inequality = options.constraints.inequality, model,
                     scale](const optim::ParameterPack<Series> &coordinates) {
      const auto detached = optim::unflatten_values(
          coordinates.layout(), optim::flatten_values(coordinates).detach());
      optim::NonlinearConstraintEvaluation evaluation;

      const auto evaluate = [&](const ConstraintFn &callback) {
        return optim::jacrev(
            [&](const optim::ParameterPack<pluss::subsp::math::diff::Trace>
                    &normalized) {
              const ParameterSet physical = detail::RecordAccess::parameters(
                  *model, optim::flatten_values(normalized).value().mul(scale));
              Series residual = callback(physical).si_value();
              return pluss::subsp::math::diff::Trace(
                  residual.reshape({residual.numel()}));
            },
            detached);
      };

      if (equality) {
        auto [value, jacobian] = evaluate(equality);
        evaluation.equality = std::move(value);
        evaluation.equality_jacobian = std::move(jacobian);
      } else {
        evaluation.equality = pluss::series::empty(
            {0}, optim::flatten_values(detached).scalar_type());
        evaluation.equality =
            evaluation.equality.to(optim::flatten_values(detached).platform());
        evaluation.equality_jacobian = detail::zero_jacobian(detached, 0);
      }

      if (inequality) {
        auto [value, jacobian] = evaluate(inequality);
        evaluation.inequality = std::move(value);
        evaluation.inequality_jacobian = std::move(jacobian);
      } else {
        evaluation.inequality = pluss::series::empty(
            {0}, optim::flatten_values(detached).scalar_type());
        evaluation.inequality = evaluation.inequality.to(
            optim::flatten_values(detached).platform());
        evaluation.inequality_jacobian = detail::zero_jacobian(detached, 0);
      }
      return evaluation;
    };
  }

  auto solver = options.solver;
  // Physical scales are already the variable normalization. Applying the
  // optimizer's data-derived scaling again changes the scientific contract
  // and can distort constrained coordinates.
  solver.sequential_qp.problem_scaling = optim::OptimaProblemScaling::None;
  solver.working_set.problem_scaling = optim::OptimaProblemScaling::None;
  solver.internal_protocol.problem_scaling = optim::OptimaProblemScaling::None;
  auto raw = optim::optima(problem, normalized, solver);
  const Series physical_flat = optim::flatten_values(raw.params).mul(scale);
  ParameterSet parameters =
      detail::RecordAccess::parameters(initial, physical_flat);
  return FitResult<ObjectiveDimension>{
      std::move(parameters),
      SeriesQuantity<ObjectiveDimension>::from_si(raw.evaluation.value),
      std::move(raw)};
}

} // namespace pluss::subsp::natsci::common::solve
