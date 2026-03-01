#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>

#include <vector>

namespace {

using Catch::Approx;
using pluss::ScalarType;
using pluss::Series;
using pluss::make_series;
using pluss::subsp::math::optim::BestFeasiblePoint;
using pluss::subsp::math::optim::ConstrainedProblem;
using pluss::subsp::math::optim::EvalStatus;
using pluss::subsp::math::optim::OptimaAlgorithm;
using pluss::subsp::math::optim::OptimaOptions;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::PenaltyMethodState;
using pluss::subsp::math::optim::ProblemEvaluation;
using pluss::subsp::math::optim::optima;
using pluss::subsp::math::optim::square;
using pluss::subsp::math::optim::sum;
using pluss::subsp::math::optim::to_optima_state;
using pluss::subsp::math::optim::trace;

Series make_vec2(double x0, double x1) {
  return make_series(std::vector<double>{x0, x1}, {2}).to(ScalarType::Float64);
}

Series make_scalar(double x) {
  return make_series(x).to(ScalarType::Float64);
}

ConstrainedProblem make_constrained_problem() {
  const Series target = make_vec2(1.0, 2.0);

  ConstrainedProblem problem;
  problem.objective.autodiff = [target](const ParameterPack<pluss::subsp::math::optim::Trace>& p) {
    const auto err = p.at("x") - trace(target, false);
    return sum(square(err));
  };
  problem.nonlinear.inequality = [](const ParameterPack<pluss::subsp::math::optim::Trace>& p) {
    return sum(p.at("x")) - 1.5;
  };
  problem.bounds.lower = ParameterPack<Series>({
      {"x", make_vec2(0.0, 0.0)},
  });
  return problem;
}

} // namespace

TEST_CASE("optima output exposes best feasible and multiplier summaries",
          "[optim][optima][output][summary]") {
  const auto problem = make_constrained_problem();
  const ParameterPack<Series> initial({
      {"x", make_vec2(1.0, 1.0)},
  });

  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::SequentialQP;
  options.sequential_qp.max_iterations = 40;
  options.sequential_qp.constraint_tolerance = 1e-6;
  options.sequential_qp.optimality_tolerance = 1e-5;

  const auto result = optima(problem, initial, options);

  REQUIRE(result.output.best_feasible_available);
  REQUIRE(result.output.returned_best_feasible);
  REQUIRE(result.output.best_feasible_value == Approx(result.evaluation.value.item<double>()).margin(1e-8));
  REQUIRE(result.output.best_feasible_constraint_violation ==
          Approx(result.evaluation.max_constraint_violation).margin(1e-8));
  REQUIRE(result.output.best_feasible_first_order_optimality >= 0.0);
  REQUIRE(result.output.multiplier_max_norm > 0.0);
  REQUIRE(result.output.active_multiplier_count > 0);
}

TEST_CASE("optima state reports when best feasible point differs from returned iterate",
          "[optim][optima][output][best-feasible]") {
  const ParameterPack<Series> params({
      {"x", make_vec2(0.8, 0.8)},
  });

  PenaltyMethodState state;
  state.params = params;
  state.iteration = 3;
  state.function_evaluations = 7;
  state.last_step_size = 0.25;
  state.current_penalty = 10.0;
  state.converged = true;
  state.message = "maximum iterations reached";
  state.evaluation.status = EvalStatus::Ok;
  state.evaluation.value = make_scalar(1.6);
  state.evaluation.gradient = ParameterPack<Series>({
      {"x", make_vec2(0.0, 0.0)},
  });
  state.evaluation.max_constraint_violation = 0.2;
  state.evaluation.first_order_opt = 0.3;
  state.evaluation.feasible = false;
  state.best_feasible = BestFeasiblePoint{
      params,
      make_scalar(1.25),
      0.0,
      0.05,
  };

  const auto result = to_optima_state(ConstrainedProblem{}, state);

  REQUIRE(result.output.best_feasible_available);
  REQUIRE_FALSE(result.output.returned_best_feasible);
  REQUIRE(result.output.best_feasible_value == Approx(1.25).margin(1e-12));
  REQUIRE(result.output.best_feasible_constraint_violation == Approx(0.0).margin(1e-12));
  REQUIRE(result.output.best_feasible_first_order_optimality == Approx(0.05).margin(1e-12));
  REQUIRE(result.output.detail_message.find("best feasible point retained separately") != std::string::npos);
  REQUIRE(result.message.find("best feasible point retained separately") != std::string::npos);
}
