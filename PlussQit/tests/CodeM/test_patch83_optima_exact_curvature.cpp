#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>
#include <pluss/subsp/math/optim.h>

#include <string>


namespace {

using Catch::Approx;
using pluss::ScalarType;
using pluss::Series;
using pluss::series::full;
using pluss::subsp::math::optim::ConstrainedProblem;
using pluss::subsp::math::optim::OptimaAlgorithm;
using pluss::subsp::math::optim::OptimaExitFlag;
using pluss::subsp::math::optim::OptimaHessianApproximation;
using pluss::subsp::math::optim::OptimaLinearSystemSolver;
using pluss::subsp::math::optim::OptimaOptions;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::optima;
using pluss::subsp::math::diff::square;

Series scalar(double value) {
  return full({}, value, ScalarType::Float64).to(pluss::pCPU);
}

ConstrainedProblem make_problem() {
  ConstrainedProblem problem;
  problem.objective.autodiff = [](const ParameterPack<Series>& p) -> Series {
    return (p.at("x0") - 0.8).square() + (p.at("x1") - 0.6).square();
  };
  problem.nonlinear.inequality = [](const ParameterPack<Series>& p) -> Series {
    return p.at("x0").square() + p.at("x1") - 1.0;
  };
  return problem;
}

ParameterPack<Series> make_initial() {
  return ParameterPack<Series>({
      {"x0", scalar(0.2)},
      {"x1", scalar(0.2)},
  });
}

} // namespace

TEST_CASE("interior-point exact curvature converges on nonlinear barrier objective",
          "[exact][interior-point][optim][optima]") {
  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::InternalProtocol;
  options.internal_protocol.max_iterations = 100;
  options.internal_protocol.constraint_tolerance = 1e-7;
  options.internal_protocol.optimality_tolerance = 1e-7;
  options.internal_protocol.minimum_barrier_parameter = 1e-10;
  options.internal_protocol.hessian_approximation = OptimaHessianApproximation::Exact;
  options.internal_protocol.linear_system_solver = OptimaLinearSystemSolver::Cholesky;

  const auto result = optima(make_problem(), make_initial(), options);
  CAPTURE(static_cast<int>(result.exit_flag));
  CAPTURE(result.message);
  CAPTURE(result.output.detail_message);
  CAPTURE(result.evaluation.max_constraint_violation);
  CAPTURE(result.output.complementarity);
  CAPTURE(result.output.first_order_optimality);
  CAPTURE(result.evaluation.first_order_opt);
  CAPTURE(as_f64(result.evaluation.gradient.at("x0")));
  CAPTURE(as_f64(result.evaluation.gradient.at("x1")));
  CAPTURE(as_f64(result.evaluation.value));
  CAPTURE(as_f64(result.params.at("x0")));
  CAPTURE(as_f64(result.params.at("x1")));

  REQUIRE((result.exit_flag == OptimaExitFlag::Converged ||
           result.exit_flag == OptimaExitFlag::MaxIterations));
  REQUIRE(result.evaluation.max_constraint_violation <= 1e-4);
  REQUIRE(result.output.complementarity < 1e-3);
  REQUIRE(as_f64(result.evaluation.value) < 0.2);
  REQUIRE(as_f64(result.params.at("x0")) == Approx(0.691).margin(5e-2));
  REQUIRE(as_f64(result.params.at("x1")) == Approx(0.522).margin(5e-2));
}

TEST_CASE("interior-point HessianVector mode works with conjugate gradient",
          "[hvp][cg][interior-point][optim][optima]") {
  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::InternalProtocol;
  options.internal_protocol.max_iterations = 100;
  options.internal_protocol.constraint_tolerance = 1e-6;
  options.internal_protocol.optimality_tolerance = 1e-6;
  options.internal_protocol.minimum_barrier_parameter = 1e-10;
  options.internal_protocol.hessian_approximation = OptimaHessianApproximation::HessianVector;
  options.internal_protocol.linear_system_solver = OptimaLinearSystemSolver::ConjugateGradient;
  options.internal_protocol.max_krylov_iterations = 20;

  const auto result = optima(make_problem(), make_initial(), options);

  REQUIRE(result.converged);
  REQUIRE(result.evaluation.max_constraint_violation <= 1e-4);
  REQUIRE(result.output.krylov_iterations >= 0);
  REQUIRE(result.output.complementarity < 1e-3);
  REQUIRE(as_f64(result.evaluation.value) < 0.25);
}

TEST_CASE("interior-point exact curvature needs an objective written over Series",
          "[exact][interior-point][optim][optima][validation]") {
  ConstrainedProblem problem;
  problem.objective.analytic = [](const ParameterPack<Series>& p) {
    const Series x = p.at("x");
    return std::pair<Series, ParameterPack<Series>>{
        (x - 1.0).square().sum(), ParameterPack<Series>({{"x", (x - 1.0) * 2.0}})};
  };

  const ParameterPack<Series> initial({
      {"x", scalar(0.0)},
  });

  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::InternalProtocol;
  options.internal_protocol.hessian_approximation = OptimaHessianApproximation::Exact;

  const auto result = optima(problem, initial, options);

  REQUIRE(result.exit_flag == OptimaExitFlag::InvalidProblem);
  REQUIRE(result.message.find("autodiff objective") != std::string::npos);
}
