#pragma once

#include "pluss/subsp/natsci/physics/mechanics/hamiltonian.h"

#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

namespace pluss::subsp::natsci::physics::mechanics {

namespace lagrangian_detail {

namespace math_diff = ::pluss::subsp::math::diff;

template <class Fn, common::DimensionType CoordinateDimension>
[[nodiscard]] common::SeriesQuantity<common::Energy>
invoke_energy(Fn &fn, const Series &coordinate, const Series &velocity,
              const Series &time) {
  using Coordinate = common::SeriesQuantity<CoordinateDimension>;
  using Velocity = common::SeriesQuantity<CoordinateRate<CoordinateDimension>>;
  using Time = common::SeriesQuantity<common::Time>;
  using Result = std::invoke_result_t<Fn &, const Coordinate &,
                                      const Velocity &, const Time &>;
  using Traits =
      hamiltonian_detail::quantity_result<std::remove_cvref_t<Result>>;
  static_assert(Traits::valid, "Lagrangian callback must return a Quantity");
  static_assert(std::same_as<typename Traits::dimension, common::Energy>,
                "Lagrangian callback must return energy");
  static_assert(std::same_as<typename Traits::rep, Series>,
                "Lagrangian callback must return a Series quantity");
  return std::invoke(fn, Coordinate::from_si(coordinate),
                     Velocity::from_si(velocity), Time::from_si(time));
}

inline Series batch_square_block(const Series &block, std::int64_t batches,
                                 std::int64_t dof, const Series &like) {
  const Series mask = pluss::series::eye(
      batches,
      Config{}.with_dtype(like.scalar_type()).with_platform(like.platform()));
  return block.reshape({batches, dof, batches, dof})
      .mul(mask.reshape({batches, 1, batches, 1}))
      .sum(2, false);
}

inline Series batch_time_block(const Series &block, std::int64_t batches,
                               std::int64_t dof, const Series &like) {
  const Series mask = pluss::series::eye(
      batches,
      Config{}.with_dtype(like.scalar_type()).with_platform(like.platform()));
  return block.reshape({batches, dof, batches})
      .mul(mask.reshape({batches, 1, batches}))
      .sum(2, false);
}

inline std::vector<std::int64_t> matrix_shape(const Series &coordinate,
                                              std::int64_t dof) {
  auto shape = detail::batch_shape(coordinate);
  shape.push_back(dof);
  shape.push_back(dof);
  return shape;
}

} // namespace lagrangian_detail

template <common::DimensionType CoordinateDimension>
struct LagrangeEvaluation final {
  common::SeriesQuantity<common::Energy> value;
  common::SeriesQuantity<CanonicalMomentum<CoordinateDimension>> momentum;
  common::SeriesQuantity<GeneralizedMass<CoordinateDimension>> mass;
  TangentRate<CoordinateDimension> flow;
};

template <common::DimensionType CoordinateDimension, class Fn>
[[nodiscard]] LagrangeEvaluation<CoordinateDimension>
lagrange(Fn &&fn, const TangentState<CoordinateDimension> &state,
         common::ScalarQuantity<common::Time> time =
             common::ScalarQuantity<common::Time>::from_si(0.0),
         const VariationalOptions &options = {}) {
  using FnType = std::decay_t<Fn>;
  FnType fn_copy(std::forward<Fn>(fn));
  const Series &q = state.coordinate().si_value();
  const Series &v = state.velocity().si_value();
  const Series t = hamiltonian_detail::batch_time(q, time);
  Series displayed = pluss::series::undefined();

  const auto policy = hamiltonian_detail::derivative_policy(options);
  auto objective = [&](const std::vector<Series> &variables) {
    return lagrangian_detail::invoke_energy<FnType, CoordinateDimension>(
               fn_copy, variables[0], variables[1], variables[2])
        .si_value()
        .sum();
  };
  auto trace_objective =
      [&](const std::vector<lagrangian_detail::math_diff::Trace> &variables) {
        displayed =
            lagrangian_detail::invoke_energy<FnType, CoordinateDimension>(
                fn_copy, variables[0].value(), variables[1].value(),
                variables[2].value())
                .si_value();
        return lagrangian_detail::math_diff::Trace(displayed.sum());
      };

  auto [total, gradients] = lagrangian_detail::math_diff::value_and_grad(
      trace_objective, std::vector<Series>{q, v, t}, policy);
  (void)total;
  hamiltonian_detail::require_energy_layout(displayed, q,
                                            "natsci::mechanics::lagrange");
  const Series hessian = lagrangian_detail::math_diff::nth_diff(
      objective, std::vector<Series>{q, v, t}, 2, {}, policy);

  const std::int64_t dof = state.dof();
  const std::int64_t batches = q.numel() / dof;
  const std::int64_t q_count = q.numel();
  const std::int64_t v_begin = q_count;
  const std::int64_t v_end = v_begin + v.numel();
  const std::int64_t t_begin = v_end;
  const std::int64_t t_end = t_begin + t.numel();

  const Series mass_flat = lagrangian_detail::batch_square_block(
      hessian.slice(0, v_begin, v_end).slice(1, v_begin, v_end), batches, dof,
      q);
  const Series mixed_coordinate = lagrangian_detail::batch_square_block(
      hessian.slice(0, v_begin, v_end).slice(1, 0, q_count), batches, dof, q);
  const Series mixed_time = lagrangian_detail::batch_time_block(
      hessian.slice(0, v_begin, v_end).slice(1, t_begin, t_end), batches, dof,
      q);

  const Series flat_velocity = v.reshape({batches, dof});
  const Series coordinate_term =
      mixed_coordinate.matmul(flat_velocity.unsqueeze(-1)).squeeze(-1);
  const Series rhs =
      gradients[0].reshape({batches, dof}).sub(coordinate_term).sub(mixed_time);
  const Series acceleration = mass_flat.linearalg.inverse()
                                  .matmul(rhs.unsqueeze(-1))
                                  .squeeze(-1)
                                  .reshape(q.sizes());

  return {
      common::SeriesQuantity<common::Energy>::from_si(std::move(displayed)),
      common::SeriesQuantity<CanonicalMomentum<CoordinateDimension>>::from_si(
          gradients[1]),
      common::SeriesQuantity<GeneralizedMass<CoordinateDimension>>::from_si(
          mass_flat.reshape(lagrangian_detail::matrix_shape(q, dof))),
      {state.velocity(),
       common::SeriesQuantity<CoordinateAcceleration<CoordinateDimension>>::
           from_si(acceleration)}};
}

} // namespace pluss::subsp::natsci::physics::mechanics
