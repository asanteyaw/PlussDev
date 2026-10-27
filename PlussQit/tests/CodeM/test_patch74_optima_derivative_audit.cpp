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
using pluss::subsp::math::optim::DerivativeAuditOptions;
using pluss::subsp::math::optim::NonlinearConstraintEvaluation;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::audit_optima_derivatives;

Series make_vec2(double x0, double x1) {
  return make_series(std::vector<double>{x0, x1}, {2}).to(ScalarType::Float64);
}

ConstrainedProblem make_consistent_problem() {
  ConstrainedProblem problem;
  const Series target = make_vec2(1.0, -0.5);
  problem.objective.autodiff = [target](const ParameterPack<Series>& p) {
    const auto err = p.at("x") - target;
    return err.square().sum();
  };
  problem.objective.analytic = [](const ParameterPack<Series>& p) {
    const auto x = p.at("x").host<double>();
    return std::pair{
        make_series((x[0] - 1.0) * (x[0] - 1.0) + (x[1] + 0.5) * (x[1] + 0.5))
            .to(ScalarType::Float64),
        ParameterPack<Series>({
            {"x", make_vec2(2.0 * (x[0] - 1.0), 2.0 * (x[1] + 0.5))},
        })};
  };
  problem.nonlinear.inequality = [](const ParameterPack<Series>& p) {
    return p.at("x").sum() - 1.0;
  };
  problem.nonlinear.analytic = [](const ParameterPack<Series>& p) {
    NonlinearConstraintEvaluation eval;
    const auto x = p.at("x").host<double>();
    eval.equality = pluss::series::empty({0}, ScalarType::Float64);
    eval.inequality = make_series(x[0] + x[1] - 1.0).reshape({1}).to(ScalarType::Float64);
    eval.equality_jacobian = ParameterPack<Series>({
        {"x", pluss::series::zeros({0, 2}, ScalarType::Float64)},
    });
    eval.inequality_jacobian = ParameterPack<Series>({
        {"x", make_series(std::vector<double>{1.0, 1.0}, {1, 2}).to(ScalarType::Float64)},
    });
    return eval;
  };
  return problem;
}

ConstrainedProblem make_inconsistent_problem() {
  auto problem = make_consistent_problem();
  problem.objective.analytic = [](const ParameterPack<Series>& p) {
    const auto x = p.at("x").host<double>();
    return std::pair{
        make_series((x[0] - 1.0) * (x[0] - 1.0) + (x[1] + 0.5) * (x[1] + 0.5))
            .to(ScalarType::Float64),
        ParameterPack<Series>({
            {"x", make_vec2(2.0 * (x[0] - 1.0) + 0.25, 2.0 * (x[1] + 0.5))},
        })};
  };
  return problem;
}

} // namespace

TEST_CASE("optima derivative audit passes for matching analytic and autodiff callbacks",
          "[optim][optima][audit]") {
  const auto problem = make_consistent_problem();
  const ParameterPack<Series> params({
      {"x", make_vec2(0.25, 0.75)},
  });

  const auto report = audit_optima_derivatives(problem, params, DerivativeAuditOptions{1e-10, 1e-8});

  REQUIRE(report.objective.available);
  REQUIRE(report.objective.passed);
  REQUIRE(report.nonlinear_inequalities.available);
  REQUIRE(report.nonlinear_inequalities.passed);
  REQUIRE(report.passed);
  REQUIRE(report.objective.derivative_abs_diff == Approx(0.0).margin(1e-12));
}

TEST_CASE("optima derivative audit detects analytic gradient mismatches",
          "[optim][optima][audit][mismatch]") {
  const auto problem = make_inconsistent_problem();
  const ParameterPack<Series> params({
      {"x", make_vec2(0.25, 0.75)},
  });

  const auto report = audit_optima_derivatives(problem, params);

  REQUIRE(report.objective.available);
  REQUIRE_FALSE(report.objective.passed);
  REQUIRE(report.objective.derivative_abs_diff > 1e-3);
  REQUIRE_FALSE(report.passed);
}
