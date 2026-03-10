#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>
#include <pluss/subsp/math/optim.h>

#include <vector>

namespace {

using Catch::Approx;
using pluss::ScalarType;
using pluss::Series;
using pluss::make_series;
using pluss::subsp::math::optim::ConstrainedProblem;
using pluss::subsp::math::optim::OptimaAlgorithm;
using pluss::subsp::math::optim::OptimaHessianApproximation;
using pluss::subsp::math::optim::OptimaLinearSystemSolver;
using pluss::subsp::math::optim::OptimaOptions;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::square;
using pluss::subsp::math::optim::sum;
using pluss::subsp::math::optim::trace;
using pluss::subsp::math::optim::optima;

Series make_vec2(double x0, double x1) {
  return make_series(std::vector<double>{x0, x1}, {2}).to(ScalarType::Float64);
}

ConstrainedProblem make_problem() {
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

TEST_CASE("optima dispatch routes sequential qp correctly", "[optim][optima][dispatch]") {
  const auto problem = make_problem();
  const ParameterPack<Series> initial({
      {"x", make_vec2(1.0, 1.0)},
  });

  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::SequentialQP;
  options.sequential_qp.max_iterations = 40;
  options.sequential_qp.constraint_tolerance = 1e-6;
  options.sequential_qp.optimality_tolerance = 1e-5;

  const auto result = optima(problem, initial, options);
  const auto x = result.params.at("x").to_vec<double>();

  REQUIRE(result.algorithm == OptimaAlgorithm::SequentialQP);
  REQUIRE(result.evaluation.max_constraint_violation <= 1e-4);
  REQUIRE(result.evaluation.value.item<double>() == Approx(1.125).margin(5e-2));
  REQUIRE(result.output.line_search_fraction == Approx(result.last_step_size));
  REQUIRE_FALSE(result.output.detail_message.empty());
  REQUIRE(result.multipliers.nonlinear_inequalities.numel() == 1);
  REQUIRE(result.multipliers.nonlinear_inequalities.item<double>() > 0.0);
  REQUIRE(x[0] == Approx(0.25).margin(5e-2));
  REQUIRE(x[1] == Approx(1.25).margin(5e-2));
}

TEST_CASE("optima dispatch routes interior point correctly", "[optim][optima][dispatch][ip]") {
  const auto problem = make_problem();
  const ParameterPack<Series> initial({
      {"x", make_vec2(0.60, 0.60)},
  });

  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::InternalProtocol;
  options.internal_protocol.max_iterations = 80;
  options.internal_protocol.initial_barrier_parameter = 0.1;
  options.internal_protocol.minimum_barrier_parameter = 1e-7;
  options.internal_protocol.constraint_tolerance = 1e-6;
  options.internal_protocol.optimality_tolerance = 1e-5;

  const auto result = optima(problem, initial, options);
  const auto x = result.params.at("x").to_vec<double>();

  REQUIRE(result.algorithm == OptimaAlgorithm::InternalProtocol);
  REQUIRE(result.evaluation.max_constraint_violation <= 1e-4);
  REQUIRE(result.evaluation.value.item<double>() == Approx(1.125).margin(8e-2));
  REQUIRE(result.output.line_search_fraction == Approx(result.last_step_size));
  REQUIRE_FALSE(result.output.detail_message.empty());
  REQUIRE(result.multipliers.nonlinear_inequalities.numel() == 1);
  REQUIRE(result.multipliers.nonlinear_inequalities.item<double>() > 0.0);
  REQUIRE(x[0] == Approx(0.25).margin(7e-2));
  REQUIRE(x[1] == Approx(1.25).margin(7e-2));
}

TEST_CASE("optima dispatch routes working set correctly", "[optim][optima][dispatch][working-set]") {
  const auto problem = make_problem();
  const ParameterPack<Series> initial({
      {"x", make_vec2(1.0, 1.0)},
  });

  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::WorkingSet;
  options.working_set.max_iterations = 40;
  options.working_set.constraint_tolerance = 1e-6;
  options.working_set.optimality_tolerance = 1e-5;

  const auto result = optima(problem, initial, options);
  const auto x = result.params.at("x").to_vec<double>();

  REQUIRE(result.algorithm == OptimaAlgorithm::WorkingSet);
  REQUIRE(result.output.algorithm == "working-set");
  REQUIRE(result.evaluation.max_constraint_violation <= 1e-4);
  REQUIRE(result.evaluation.value.item<double>() == Approx(1.125).margin(5e-2));
  REQUIRE(result.output.line_search_fraction == Approx(result.last_step_size));
  REQUIRE_FALSE(result.output.detail_message.empty());
  REQUIRE(result.multipliers.nonlinear_inequalities.numel() == 1);
  REQUIRE(result.multipliers.nonlinear_inequalities.item<double>() > 0.0);
  REQUIRE(x[0] == Approx(0.25).margin(5e-2));
  REQUIRE(x[1] == Approx(1.25).margin(5e-2));
}

TEST_CASE("optima sequential qp supports explicit inverse solve and identity Hessian mode",
          "[optim][optima][dispatch][sqp][modes]") {
  const auto problem = make_problem();
  const ParameterPack<Series> initial({
      {"x", make_vec2(1.0, 1.0)},
  });

  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::SequentialQP;
  options.sequential_qp.max_iterations = 80;
  options.sequential_qp.constraint_tolerance = 1e-6;
  options.sequential_qp.optimality_tolerance = 1e-5;
  options.sequential_qp.linear_system_solver = OptimaLinearSystemSolver::Inverse;
  options.sequential_qp.hessian_approximation = OptimaHessianApproximation::Identity;

  const auto result = optima(problem, initial, options);
  const auto x = result.params.at("x").to_vec<double>();

  REQUIRE(result.algorithm == OptimaAlgorithm::SequentialQP);
  REQUIRE(result.evaluation.max_constraint_violation <= 1e-4);
  REQUIRE(result.evaluation.value.item<double>() == Approx(1.125).margin(8e-2));
  REQUIRE(x[0] == Approx(0.25).margin(7e-2));
  REQUIRE(x[1] == Approx(1.25).margin(7e-2));
}

TEST_CASE("optima interior point supports explicit pseudoinverse solve and identity Hessian mode",
          "[optim][optima][dispatch][ip][modes]") {
  const auto problem = make_problem();
  const ParameterPack<Series> initial({
      {"x", make_vec2(0.60, 0.60)},
  });

  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::InternalProtocol;
  options.internal_protocol.max_iterations = 120;
  options.internal_protocol.initial_barrier_parameter = 0.1;
  options.internal_protocol.minimum_barrier_parameter = 1e-7;
  options.internal_protocol.constraint_tolerance = 1e-6;
  options.internal_protocol.optimality_tolerance = 1e-5;
  options.internal_protocol.linear_system_solver = OptimaLinearSystemSolver::PseudoInverse;
  options.internal_protocol.hessian_approximation = OptimaHessianApproximation::Identity;

  const auto result = optima(problem, initial, options);
  const auto x = result.params.at("x").to_vec<double>();

  REQUIRE(result.algorithm == OptimaAlgorithm::InternalProtocol);
  REQUIRE(result.evaluation.max_constraint_violation <= 1e-4);
  REQUIRE(result.evaluation.value.item<double>() == Approx(1.125).margin(1e-1));
  REQUIRE(x[0] == Approx(0.25).margin(9e-2));
  REQUIRE(x[1] == Approx(1.25).margin(9e-2));
}
