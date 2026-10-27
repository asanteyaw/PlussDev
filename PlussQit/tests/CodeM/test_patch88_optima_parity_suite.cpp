#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>
#include <pluss/subsp/math/optim.h>

#include <array>
#include <vector>

namespace {

using Catch::Approx;
using pluss::ScalarType;
using pluss::Series;
using pluss::make_series;
using pluss::subsp::math::optim::ConstrainedProblem;
using pluss::subsp::math::optim::OptimaAlgorithm;
using pluss::subsp::math::optim::OptimaOptions;
using pluss::subsp::math::optim::OptimaProblemScaling;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::evaluate_problem;
using pluss::subsp::math::optim::optima;

Series make_vec2(double x0, double x1) {
  return make_series(std::vector<double>{x0, x1}, {2}).to(ScalarType::Float64);
}

OptimaOptions make_options(OptimaAlgorithm algorithm) {
  OptimaOptions options;
  options.algorithm = algorithm;
  options.sequential_qp.max_iterations = 120;
  options.sequential_qp.constraint_tolerance = 1e-6;
  options.sequential_qp.optimality_tolerance = 1e-5;
  options.internal_protocol.max_iterations = 180;
  options.internal_protocol.initial_barrier_parameter = 0.1;
  options.internal_protocol.minimum_barrier_parameter = 1e-7;
  options.internal_protocol.constraint_tolerance = 1e-6;
  options.internal_protocol.optimality_tolerance = 1e-5;
  return options;
}

ConstrainedProblem make_mixed_nonlinear_problem() {
  const Series target = make_vec2(0.2, 0.2);
  const Series equality_weights = make_vec2(1.0, -1.0);

  ConstrainedProblem problem;
  problem.objective.autodiff = [target](const ParameterPack<Series>& p) {
    const auto err = p.at("x") - target;
    return err.square().sum();
  };
  problem.nonlinear.equality = [equality_weights](const ParameterPack<Series>& p) {
    return (p.at("x") * equality_weights).sum();
  };
  problem.nonlinear.inequality = [](const ParameterPack<Series>& p) {
    return p.at("x").sum() - 1.0;
  };
  problem.bounds.lower = ParameterPack<Series>({
      {"x", make_vec2(0.0, 0.0)},
  });
  return problem;
}

ConstrainedProblem make_bound_active_problem() {
  const Series target = make_vec2(-1.0, 2.0);

  ConstrainedProblem problem;
  problem.objective.autodiff = [target](const ParameterPack<Series>& p) {
    const auto err = p.at("x") - target;
    return err.square().sum();
  };
  problem.nonlinear.inequality = [](const ParameterPack<Series>& p) {
    return p.at("x").sum() - 1.5;
  };
  problem.bounds.lower = ParameterPack<Series>({
      {"x", make_vec2(0.0, 0.0)},
  });
  problem.bounds.upper = ParameterPack<Series>({
      {"x", make_vec2(2.0, 1.5)},
  });
  return problem;
}

ConstrainedProblem make_ill_conditioned_problem() {
  const Series target = make_vec2(1e-1, 1e1);
  const Series objective_scale = make_vec2(1e1, 1e-1);

  ConstrainedProblem problem;
  problem.objective.autodiff = [target, objective_scale](
                                   const ParameterPack<Series>& p) {
    const auto err = (p.at("x") - target) * objective_scale;
    return err.square().sum();
  };
  problem.linear.Aineq = make_series(std::vector<double>{1e1, 1e-1}, {1, 2}).to(ScalarType::Float64);
  problem.linear.bineq = make_series(std::vector<double>{2.0}, {1}).to(ScalarType::Float64);
  problem.bounds.lower = ParameterPack<Series>({
      {"x", make_vec2(0.0, 0.0)},
  });
  return problem;
}

} // namespace

TEST_CASE("optima handles infeasible mixed nonlinear starts across sqp and interior point",
          "[optim][optima][parity-suite][mixed]") {
  const auto problem = make_mixed_nonlinear_problem();
  const ParameterPack<Series> initial({
      {"x", make_vec2(0.9, 0.4)},
  });

  for (const auto algorithm :
       std::array{OptimaAlgorithm::SequentialQP, OptimaAlgorithm::InternalProtocol}) {
    auto options = make_options(algorithm);
    const auto result = optima(problem, initial, options);
    const auto x = result.params.at("x").host<double>();

    REQUIRE(result.evaluation.max_constraint_violation <= 1e-4);
    REQUIRE(x[0] == Approx(0.2).margin(8e-2));
    REQUIRE(x[1] == Approx(0.2).margin(8e-2));
  }
}

TEST_CASE("optima handles active bounds and active inequalities across sqp and interior point",
          "[optim][optima][parity-suite][active-bounds]") {
  const auto problem = make_bound_active_problem();
  const ParameterPack<Series> initial({
      {"x", make_vec2(1.2, 1.2)},
  });

  for (const auto algorithm :
       std::array{OptimaAlgorithm::SequentialQP, OptimaAlgorithm::InternalProtocol}) {
    auto options = make_options(algorithm);
    const auto result = optima(problem, initial, options);
    const auto x = result.params.at("x").host<double>();

    REQUIRE(result.evaluation.max_constraint_violation <= 1e-4);
    REQUIRE(result.output.active_multiplier_count > 0);
    REQUIRE(x[0] == Approx(0.0).margin(8e-2));
    REQUIRE(x[1] == Approx(1.5).margin(8e-2));
  }
}

TEST_CASE("optima handles ill-conditioned scaling with normalization across sqp and interior point",
          "[optim][optima][parity-suite][ill-conditioned]") {
  const auto problem = make_ill_conditioned_problem();
  const ParameterPack<Series> initial({
      {"x", make_vec2(0.6, 1.0)},
  });
  const auto initial_eval = evaluate_problem(problem, initial);

  for (const auto algorithm :
       std::array{OptimaAlgorithm::SequentialQP, OptimaAlgorithm::InternalProtocol}) {
    auto options = make_options(algorithm);
    options.sequential_qp.problem_scaling = OptimaProblemScaling::ObjectiveAndConstraints;
    options.internal_protocol.problem_scaling = OptimaProblemScaling::ObjectiveAndConstraints;

    const auto result = optima(problem, initial, options);

    REQUIRE(result.evaluation.max_constraint_violation <= 1e-3);
    REQUIRE(result.evaluation.value.item<double>() <
            0.1 * initial_eval.value.item<double>());
  }
}
