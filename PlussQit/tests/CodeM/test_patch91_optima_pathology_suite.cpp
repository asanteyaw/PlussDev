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
using pluss::subsp::math::optim::OptimaAlgorithm;
using pluss::subsp::math::optim::OptimaExitFlag;
using pluss::subsp::math::optim::OptimaOptions;
using pluss::subsp::math::optim::OptimaStallCategory;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::Trace;
using pluss::subsp::math::optim::optima;
using pluss::subsp::math::optim::square;
using pluss::subsp::math::optim::sum;
using pluss::subsp::math::optim::trace;

Series scalar(double x) {
  return make_series(x).to(ScalarType::Float64).reshape({1});
}

Series make_vec2(double x0, double x1) {
  return make_series(std::vector<double>{x0, x1}, {2}).to(ScalarType::Float64);
}

double as_f64(const Series& x) {
  return x.to(pluss::pCPU, ScalarType::Float64).item<double>();
}

ConstrainedProblem make_nonfinite_region_problem() {
  ConstrainedProblem problem;
  problem.objective.analytic = [](const ParameterPack<Series>& p) {
    const double x = as_f64(p.at("x"));
    if (!(x > 0.0)) {
      const double nan = std::numeric_limits<double>::quiet_NaN();
      return std::pair{
          make_series(nan).to(ScalarType::Float64),
          ParameterPack<Series>({{"x", make_series(nan).to(ScalarType::Float64).reshape({1})}})};
    }
    return std::pair{
        make_series((x - 0.25) * (x - 0.25)).to(ScalarType::Float64),
        ParameterPack<Series>({{"x", scalar(2.0 * (x - 0.25))}})};
  };
  return problem;
}

ConstrainedProblem make_boundary_limited_problem() {
  ConstrainedProblem problem;
  problem.objective.autodiff = [](const ParameterPack<Trace>& p) {
    return square(p.at("x") + 1.0);
  };
  problem.bounds.lower = ParameterPack<Series>({
      {"x", scalar(0.0)},
  });
  return problem;
}

ConstrainedProblem make_simple_quadratic_problem() {
  ConstrainedProblem problem;
  problem.objective.autodiff = [](const ParameterPack<Trace>& p) {
    return square(p.at("x") - 1.0);
  };
  return problem;
}

ConstrainedProblem make_recovery_failure_problem() {
  ConstrainedProblem problem;
  problem.objective.analytic = [](const ParameterPack<Series>& p)
      -> std::pair<Series, ParameterPack<Series>> {
    const double x = as_f64(p.at("x"));
    if (!(x > 0.0)) {
      const double nan = std::numeric_limits<double>::quiet_NaN();
      return std::pair{
          make_series(nan).to(ScalarType::Float64),
          ParameterPack<Series>({{"x", make_series(nan).to(ScalarType::Float64).reshape({1})}})};
    }
    return std::pair{
        make_series((x - 0.25) * (x - 0.25)).to(ScalarType::Float64),
        ParameterPack<Series>({{"x", scalar(2.0 * (x - 0.25))}})};
  };
  problem.nonlinear.inequality = [](const ParameterPack<Trace>& p) {
    return p.at("x") - 0.2;
  };
  return problem;
}

ConstrainedProblem make_feasibility_phase_problem() {
  ConstrainedProblem problem;
  problem.objective.autodiff = [](const ParameterPack<Trace>& p) {
    return square(p.at("x") * 0.0);
  };
  problem.nonlinear.inequality = [](const ParameterPack<Trace>& p) {
    return p.at("x") - 0.1;
  };
  problem.bounds.lower = ParameterPack<Series>({
      {"x", scalar(0.0)},
  });
  return problem;
}

ConstrainedProblem make_complementarity_probe_problem() {
  ConstrainedProblem problem;
  const Series target = make_vec2(1.0, 2.0);
  problem.objective.autodiff = [target](const ParameterPack<Trace>& p) {
    const auto err = p.at("x") - trace(target, false);
    return sum(square(err));
  };
  problem.nonlinear.inequality = [](const ParameterPack<Trace>& p) {
    return sum(p.at("x")) - 1.5;
  };
  problem.bounds.lower = ParameterPack<Series>({
      {"x", make_vec2(0.0, 0.0)},
  });
  return problem;
}

} // namespace

TEST_CASE("optima classifies live non-finite initial regions", "[optim][optima][pathology][nonfinite]") {
  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::SequentialQP;

  const auto result = optima(make_nonfinite_region_problem(), ParameterPack<Series>({{"x", scalar(-0.5)}}), options);

  REQUIRE(result.exit_flag == OptimaExitFlag::NonFiniteEvaluation);
  REQUIRE(result.output.stall_category == OptimaStallCategory::NonFiniteCallbackRegion);
}

TEST_CASE("optima classifies live boundary-limited SQP exits", "[optim][optima][pathology][boundary]") {
  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::SequentialQP;
  options.sequential_qp.max_iterations = 5;

  const auto result = optima(make_boundary_limited_problem(), ParameterPack<Series>({{"x", scalar(0.0)}}), options);

  REQUIRE(result.exit_flag == OptimaExitFlag::NoProgress);
  REQUIRE(result.output.stall_category == OptimaStallCategory::BoundaryLimited);
}

TEST_CASE("optima classifies live SQP search-direction collapse exits", "[optim][optima][pathology][collapse]") {
  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::SequentialQP;
  options.sequential_qp.max_iterations = 5;
  options.sequential_qp.max_line_search_iterations = 0;
  options.sequential_qp.enable_restoration = false;

  const auto result = optima(make_simple_quadratic_problem(), ParameterPack<Series>({{"x", scalar(0.0)}}), options);

  REQUIRE(result.exit_flag == OptimaExitFlag::NoProgress);
  REQUIRE(result.output.stall_category == OptimaStallCategory::SearchDirectionCollapse);
}

TEST_CASE("optima classifies live SQP recovery failures", "[optim][optima][pathology][recovery]") {
  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::SequentialQP;
  options.sequential_qp.max_iterations = 5;
  options.sequential_qp.max_line_search_iterations = 0;
  options.sequential_qp.max_restoration_iterations = 0;
  options.sequential_qp.enable_restoration = true;

  const auto result = optima(make_recovery_failure_problem(), ParameterPack<Series>({{"x", scalar(1.0)}}), options);

  REQUIRE(result.exit_flag == OptimaExitFlag::NoProgress);
  REQUIRE(result.output.stall_category == OptimaStallCategory::RecoveryFailure);
}

TEST_CASE("optima classifies live interior-point feasibility failures", "[optim][optima][pathology][feasibility]") {
  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::InternalProtocol;
  options.internal_protocol.max_iterations = 5;
  options.internal_protocol.max_line_search_iterations = 0;
  options.internal_protocol.enable_restoration = false;
  options.internal_protocol.enable_feasibility_phase = true;
  options.internal_protocol.max_restoration_iterations = 0;
  options.internal_protocol.initial_barrier_parameter = 0.01;
  options.internal_protocol.minimum_barrier_parameter = 1e-6;

  const auto result = optima(make_feasibility_phase_problem(), ParameterPack<Series>({{"x", scalar(1.0)}}), options);
  CAPTURE(static_cast<int>(result.exit_flag), result.message,
          static_cast<int>(result.output.stall_category),
          result.evaluation.max_constraint_violation);

  REQUIRE(result.exit_flag == OptimaExitFlag::NoProgress);
  REQUIRE(result.output.stall_category == OptimaStallCategory::FeasibilityFailure);
}

TEST_CASE("optima can expose live complementarity-dominated interior-point stalls",
          "[optim][optima][pathology][complementarity]") {
  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::InternalProtocol;
  options.internal_protocol.max_iterations = 1;
  options.internal_protocol.initial_barrier_parameter = 0.1;
  options.internal_protocol.minimum_barrier_parameter = 1e-10;
  options.internal_protocol.constraint_tolerance = 1e-6;
  options.internal_protocol.optimality_tolerance = 1e-6;
  options.internal_protocol.dual_recentering_fraction = 0.0;

  const auto result = optima(
      make_complementarity_probe_problem(),
      ParameterPack<Series>({{"x", make_vec2(0.7, 0.7)}}),
      options);

  REQUIRE(result.exit_flag == OptimaExitFlag::MaxIterations);
  REQUIRE(result.output.stall_category == OptimaStallCategory::ComplementarityStagnation);
  REQUIRE(result.output.complementarity > result.output.first_order_optimality * 1e-2);
}
