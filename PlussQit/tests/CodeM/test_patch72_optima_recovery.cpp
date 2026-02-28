#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>

#include <limits>
#include <vector>

namespace {

using Catch::Approx;
using pluss::ScalarType;
using pluss::Series;
using pluss::make_series;
using pluss::subsp::math::optim::ConstrainedProblem;
using pluss::subsp::math::optim::InteriorPointOptions;
using pluss::subsp::math::optim::OptimaAlgorithm;
using pluss::subsp::math::optim::OptimaExitFlag;
using pluss::subsp::math::optim::OptimaOptions;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::SequentialQPOptions;
using pluss::subsp::math::optim::init_interior_point_state;
using pluss::subsp::math::optim::init_sequential_qp_state;
using pluss::subsp::math::optim::interior_point_step;
using pluss::subsp::math::optim::optima;
using pluss::subsp::math::optim::sequential_qp_step;
using pluss::subsp::math::optim::trace;
using pluss::subsp::math::optim::sum;
using pluss::subsp::math::optim::square;

Series make_vec2(double x0, double x1) {
  return make_series(std::vector<double>{x0, x1}, {2}).to(ScalarType::Float64);
}

ConstrainedProblem make_nan_recovery_problem() {
  ConstrainedProblem problem;
  problem.objective.analytic = [](const ParameterPack<Series>& p) {
    const auto x = p.at("x").to_vec<double>();
    const double x0 = x[0];
    const double x1 = x[1];
    if (!(x0 > 0.0)) {
      const double nan = std::numeric_limits<double>::quiet_NaN();
      return std::pair{
          make_series(nan).to(ScalarType::Float64),
          ParameterPack<Series>({
              {"x", make_vec2(nan, nan)},
          })};
    }
    const double value = (x0 - 0.25) * (x0 - 0.25) + (x1 - 1.25) * (x1 - 1.25);
    return std::pair{
        make_series(value).to(ScalarType::Float64),
        ParameterPack<Series>({
            {"x", make_vec2(2.0 * (x0 - 0.25), 2.0 * (x1 - 1.25))},
        })};
  };
  problem.nonlinear.inequality = [](const ParameterPack<pluss::subsp::math::optim::Trace>& p) {
    return sum(p.at("x")) - 1.5;
  };
  return problem;
}

} // namespace

TEST_CASE("optima sqp recovers from non-finite trial evaluations by shrinking the step",
          "[optim][optima][recovery][sqp]") {
  const auto problem = make_nan_recovery_problem();
  const ParameterPack<Series> initial({
      {"x", make_vec2(1.0, 1.0)},
  });

  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::SequentialQP;
  options.sequential_qp.max_iterations = 40;
  options.sequential_qp.initial_step_size = 4.0;
  options.sequential_qp.constraint_tolerance = 1e-6;
  options.sequential_qp.optimality_tolerance = 1e-5;

  const auto result = optima(problem, initial, options);
  const auto x = result.params.at("x").to_vec<double>();

  REQUIRE(result.exit_flag == OptimaExitFlag::Converged);
  REQUIRE(result.evaluation.status == pluss::subsp::math::optim::EvalStatus::Ok);
  REQUIRE(result.evaluation.max_constraint_violation <= 1e-4);
  REQUIRE(x[0] > 0.0);
  REQUIRE(x[0] == Approx(0.25).margin(5e-2));
  REQUIRE(x[1] == Approx(1.25).margin(5e-2));
}

TEST_CASE("optima interior point recovers from non-finite trial evaluations by shrinking the step",
          "[optim][optima][recovery][ip]") {
  const auto problem = make_nan_recovery_problem();
  const ParameterPack<Series> initial({
      {"x", make_vec2(0.60, 0.60)},
  });

  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::InteriorPoint;
  options.interior_point.max_iterations = 80;
  options.interior_point.initial_step_size = 4.0;
  options.interior_point.initial_barrier_parameter = 0.1;
  options.interior_point.minimum_barrier_parameter = 1e-7;
  options.interior_point.constraint_tolerance = 1e-6;
  options.interior_point.optimality_tolerance = 1e-5;

  const auto result = optima(problem, initial, options);
  const auto x = result.params.at("x").to_vec<double>();

  REQUIRE((result.exit_flag == OptimaExitFlag::Converged ||
           result.exit_flag == OptimaExitFlag::MaxIterations));
  REQUIRE(result.evaluation.status == pluss::subsp::math::optim::EvalStatus::Ok);
  REQUIRE(result.evaluation.max_constraint_violation <= 1e-4);
  REQUIRE(x[0] > 0.0);
  REQUIRE(x[0] == Approx(0.25).margin(8e-2));
  REQUIRE(x[1] == Approx(1.25).margin(8e-2));
}

TEST_CASE("sequential qp uses restoration when the main line search path is unavailable",
          "[optim][optima][recovery][sqp][restoration]") {
  const auto problem = make_nan_recovery_problem();
  const ParameterPack<Series> initial({
      {"x", make_vec2(1.0, 1.0)},
  });

  SequentialQPOptions options;
  options.max_line_search_iterations = 0;
  options.max_restoration_iterations = 4;
  options.max_restoration_line_search_iterations = 20;
  options.initial_step_size = 1.0;

  const auto state = init_sequential_qp_state(problem, initial, options);
  const auto next = sequential_qp_step(problem, state, options);

  REQUIRE(state.evaluation.status == pluss::subsp::math::optim::EvalStatus::Ok);
  REQUIRE(next.iteration == state.iteration + 1);
  REQUIRE(next.message == "restoration step accepted");
  REQUIRE(next.evaluation.status == pluss::subsp::math::optim::EvalStatus::Ok);
  REQUIRE(next.evaluation.max_constraint_violation < state.evaluation.max_constraint_violation);
}

TEST_CASE("interior point uses barrier recovery when the main line search path is unavailable",
          "[optim][optima][recovery][ip][restoration]") {
  const auto problem = make_nan_recovery_problem();
  const ParameterPack<Series> initial({
      {"x", make_vec2(0.60, 0.60)},
  });

  InteriorPointOptions options;
  options.max_line_search_iterations = 0;
  options.max_restoration_iterations = 4;
  options.max_restoration_line_search_iterations = 20;
  options.initial_barrier_parameter = 0.1;
  options.minimum_barrier_parameter = 1e-7;
  options.initialization_margin = 1e-4;

  const auto state = init_interior_point_state(problem, initial, options);
  const auto next = interior_point_step(problem, state, options);

  REQUIRE(state.evaluation.status == pluss::subsp::math::optim::EvalStatus::Ok);
  REQUIRE(next.iteration == state.iteration + 1);
  REQUIRE(next.message == "recovery step accepted");
  REQUIRE(next.evaluation.status == pluss::subsp::math::optim::EvalStatus::Ok);
  REQUIRE(next.barrier_value < state.barrier_value);
}
