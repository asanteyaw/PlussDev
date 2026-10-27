#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>
#include <pluss/subsp/math/optim.h>

#include <limits>
#include <vector>

namespace {

using pluss::ScalarType;
using pluss::Series;
using pluss::make_series;
using pluss::subsp::math::optim::ConstrainedProblem;
using pluss::subsp::math::optim::EvalStatus;
using pluss::subsp::math::optim::NonlinearConstraintEvaluation;
using pluss::subsp::math::optim::OptimaAlgorithm;
using pluss::subsp::math::optim::OptimaExitFlag;
using pluss::subsp::math::optim::OptimaOptions;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::evaluate_problem;
using pluss::subsp::math::optim::optima;

Series make_scalar(double x) {
  return make_series(x).to(ScalarType::Float64);
}

Series make_vec(std::initializer_list<double> xs) {
  return make_series(std::vector<double>(xs), {static_cast<std::int64_t>(xs.size())})
      .to(ScalarType::Float64);
}

} // namespace

TEST_CASE("optima rejects non-scalar objective outputs", "[optim][optima][validation][objective]") {
  ConstrainedProblem problem;
  problem.objective.analytic = [](const ParameterPack<Series>&) {
    return std::pair{
        make_vec({1.0, 2.0}),
        ParameterPack<Series>({
            {"x", make_vec({0.0, 0.0})},
        })};
  };

  const ParameterPack<Series> params({
      {"x", make_vec({1.0, 1.0})},
  });

  const auto eval = evaluate_problem(problem, params);
  REQUIRE(eval.status == EvalStatus::InvalidProblem);
  REQUIRE(eval.message.find("scalar-valued") != std::string::npos);

  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::SequentialQP;
  const auto result = optima(problem, params, options);
  REQUIRE(result.exit_flag == OptimaExitFlag::InvalidProblem);
}

TEST_CASE("optima rejects nonlinear jacobian shape mismatches", "[optim][optima][validation][jacobian-shape]") {
  ConstrainedProblem problem;
  problem.objective.analytic = [](const ParameterPack<Series>& p) {
    const auto x = p.at("x").host<double>();
    return std::pair{
        make_scalar(x[0] * x[0] + x[1] * x[1]),
        ParameterPack<Series>({
            {"x", make_vec({2.0 * x[0], 2.0 * x[1]})},
        })};
  };
  problem.nonlinear.analytic = [](const ParameterPack<Series>&) {
    NonlinearConstraintEvaluation eval;
    eval.equality = make_vec({});
    eval.inequality = make_scalar(0.25).reshape({1});
    eval.equality_jacobian = ParameterPack<Series>({
        {"x", pluss::series::zeros({0, 2}, ScalarType::Float64)},
    });
    eval.inequality_jacobian = ParameterPack<Series>({
        {"x", pluss::series::zeros({2, 2}, ScalarType::Float64)},
    });
    return eval;
  };

  const ParameterPack<Series> params({
      {"x", make_vec({1.0, 1.0})},
  });

  const auto eval = evaluate_problem(problem, params);
  REQUIRE(eval.status == EvalStatus::InvalidProblem);
  REQUIRE(eval.message.find("shape") != std::string::npos);
}

TEST_CASE("optima reports non-finite nonlinear jacobians", "[optim][optima][validation][jacobian-finite]") {
  ConstrainedProblem problem;
  problem.objective.analytic = [](const ParameterPack<Series>& p) {
    const auto x = p.at("x").host<double>();
    return std::pair{
        make_scalar(x[0] * x[0] + x[1] * x[1]),
        ParameterPack<Series>({
            {"x", make_vec({2.0 * x[0], 2.0 * x[1]})},
        })};
  };
  problem.nonlinear.analytic = [](const ParameterPack<Series>&) {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    NonlinearConstraintEvaluation eval;
    eval.equality = make_vec({});
    eval.inequality = make_scalar(0.25).reshape({1});
    eval.equality_jacobian = ParameterPack<Series>({
        {"x", pluss::series::zeros({0, 2}, ScalarType::Float64)},
    });
    eval.inequality_jacobian = ParameterPack<Series>({
        {"x", make_series(std::vector<double>{nan, 1.0}, {1, 2}).to(ScalarType::Float64)},
    });
    return eval;
  };

  const ParameterPack<Series> params({
      {"x", make_vec({1.0, 1.0})},
  });

  const auto eval = evaluate_problem(problem, params);
  REQUIRE(eval.status == EvalStatus::NonFiniteJacobian);
  REQUIRE(eval.message.find("jacobian") != std::string::npos);
}

TEST_CASE("optima turns thrown callbacks into invalid-problem diagnostics",
          "[optim][optima][validation][callback-throw]") {
  ConstrainedProblem problem;
  problem.objective.analytic = [](const ParameterPack<Series>&) -> std::pair<Series, ParameterPack<Series>> {
    throw std::runtime_error("boom");
  };

  const ParameterPack<Series> params({
      {"x", make_vec({1.0, 1.0})},
  });

  const auto eval = evaluate_problem(problem, params);
  REQUIRE(eval.status == EvalStatus::InvalidProblem);
  REQUIRE(eval.message.find("objective analytic callback failed") != std::string::npos);

  const auto result = optima(problem, params);
  REQUIRE(result.exit_flag == OptimaExitFlag::InvalidProblem);
  REQUIRE(result.output.detail_message.find("objective analytic callback failed") != std::string::npos);
}
