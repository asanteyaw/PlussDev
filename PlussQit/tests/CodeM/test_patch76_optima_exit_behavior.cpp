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
using pluss::subsp::math::optim::NonlinearConstraintEvaluation;
using pluss::subsp::math::optim::OptimaAlgorithm;
using pluss::subsp::math::optim::OptimaExitFlag;
using pluss::subsp::math::optim::OptimaOptions;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::optima;
using pluss::subsp::math::optim::square;
using pluss::subsp::math::optim::sum;
using pluss::subsp::math::optim::trace;

Series make_vec2(double x0, double x1) {
  return make_series(std::vector<double>{x0, x1}, {2}).to(ScalarType::Float64);
}

Series make_scalar(double x) {
  return make_series(x).to(ScalarType::Float64);
}

ConstrainedProblem make_objective_limit_problem() {
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

ConstrainedProblem make_constraint_stationary_problem() {
  ConstrainedProblem problem;
  problem.objective.analytic = [](const ParameterPack<Series>&) {
    return std::pair{
        make_scalar(0.0),
        ParameterPack<Series>({
            {"x", make_vec2(0.0, 0.0)},
        })};
  };
  problem.nonlinear.analytic = [](const ParameterPack<Series>&) {
    NonlinearConstraintEvaluation eval;
    eval.equality = make_scalar(1.0).reshape({1});
    eval.inequality = pluss::series::empty({0}, ScalarType::Float64);
    eval.equality_jacobian = ParameterPack<Series>({
        {"x", pluss::series::zeros({1, 2}, ScalarType::Float64)},
    });
    eval.inequality_jacobian = ParameterPack<Series>({
        {"x", pluss::series::zeros({0, 2}, ScalarType::Float64)},
    });
    return eval;
  };
  return problem;
}

} // namespace

TEST_CASE("optima sequential qp honors objective limit on feasible points",
          "[optim][optima][exit][objective-limit][sqp]") {
  const auto problem = make_objective_limit_problem();
  const ParameterPack<Series> initial({
      {"x", make_vec2(1.0, 1.0)},
  });

  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::SequentialQP;
  options.sequential_qp.max_iterations = 40;
  options.sequential_qp.objective_limit = 1.2;

  const auto result = optima(problem, initial, options);

  REQUIRE(result.exit_flag == OptimaExitFlag::ObjectiveTargetReached);
  REQUIRE(result.evaluation.feasible);
  REQUIRE(result.evaluation.value.item<double>() <= Approx(1.2).margin(1e-9));
  REQUIRE(result.output.detail_message.find("objective limit reached") != std::string::npos);
}

TEST_CASE("optima interior point honors objective limit on feasible points",
          "[optim][optima][exit][objective-limit][ip]") {
  const auto problem = make_objective_limit_problem();
  const ParameterPack<Series> initial({
      {"x", make_vec2(0.60, 0.60)},
  });

  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::InternalProtocol;
  options.internal_protocol.max_iterations = 80;
  options.internal_protocol.initial_barrier_parameter = 0.1;
  options.internal_protocol.minimum_barrier_parameter = 1e-7;
  options.internal_protocol.objective_limit = 1.2;

  const auto result = optima(problem, initial, options);

  REQUIRE(result.exit_flag == OptimaExitFlag::ObjectiveTargetReached);
  REQUIRE(result.evaluation.feasible);
  REQUIRE(result.evaluation.value.item<double>() <= Approx(1.2).margin(1e-9));
  REQUIRE(result.output.detail_message.find("objective limit reached") != std::string::npos);
}

TEST_CASE("optima sequential qp reports locally stationary infeasible constraint violation",
          "[optim][optima][exit][constraint-stationary][sqp]") {
  const auto problem = make_constraint_stationary_problem();
  const ParameterPack<Series> initial({
      {"x", make_vec2(0.0, 0.0)},
  });

  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::SequentialQP;
  options.sequential_qp.max_iterations = 5;

  const auto result = optima(problem, initial, options);

  REQUIRE(result.exit_flag == OptimaExitFlag::ConstraintStationary);
  REQUIRE(result.evaluation.max_constraint_violation > 1e-6);
  REQUIRE(result.output.detail_message.find("constraint violation locally stationary") != std::string::npos);
}

TEST_CASE("optima interior point reports locally stationary infeasible constraint violation",
          "[optim][optima][exit][constraint-stationary][ip]") {
  const auto problem = make_constraint_stationary_problem();
  const ParameterPack<Series> initial({
      {"x", make_vec2(0.0, 0.0)},
  });

  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::InternalProtocol;
  options.internal_protocol.max_iterations = 20;
  options.internal_protocol.initial_barrier_parameter = 0.1;
  options.internal_protocol.minimum_barrier_parameter = 1e-7;

  const auto result = optima(problem, initial, options);

  REQUIRE(result.exit_flag == OptimaExitFlag::ConstraintStationary);
  REQUIRE(result.evaluation.max_constraint_violation > 1e-6);
  REQUIRE(result.output.detail_message.find("constraint violation locally stationary") != std::string::npos);
}
