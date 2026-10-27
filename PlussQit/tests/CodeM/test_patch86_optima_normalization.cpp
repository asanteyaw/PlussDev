#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>
#include <pluss/subsp/math/optim.h>

#include <vector>


namespace {

using Catch::Approx;
using pluss::ScalarType;
using pluss::Series;
using pluss::series::full;
using pluss::make_series;
using pluss::subsp::math::optim::ConstrainedProblem;
using pluss::subsp::math::optim::OptimaAlgorithm;
using pluss::subsp::math::optim::OptimaOptions;
using pluss::subsp::math::optim::OptimaProblemScaling;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::SequentialQPOptions;
using pluss::subsp::math::optim::init_internal_protocol_state;
using pluss::subsp::math::optim::init_sequential_qp_state;
using pluss::subsp::math::optim::optima;
using pluss::subsp::math::diff::square;

Series scalar(double value) {
  return full({}, value, ScalarType::Float64).to(pluss::pCPU);
}

Series vec2(double x0, double x1) {
  return make_series(std::vector<double>{x0, x1}, {2}).to(ScalarType::Float64).to(pluss::pCPU);
}

} // namespace

TEST_CASE("sequential qp computes objective and inequality normalization on an ill-scaled problem",
          "[optim][normalization][sqp]") {
  ConstrainedProblem problem;
  const Series target = vec2(0.25, 1.25);
  problem.objective.autodiff = [target](const ParameterPack<Series>& p) -> Series {
    return 1.0e6 * ((p.at("x") - target).square()).sum();
  };
  problem.nonlinear.inequality = [](const ParameterPack<Series>& p) -> Series {
    return 1.0e-6 * (p.at("x").sum() - 1.5);
  };

  const ParameterPack<Series> initial({
      {"x", vec2(1.0, 1.0)},
  });

  SequentialQPOptions options;
  options.problem_scaling = OptimaProblemScaling::ObjectiveAndConstraints;

  const auto state = init_sequential_qp_state(problem, initial, options);

  REQUIRE(state.normalization.objective_scale < 1e-3);
  REQUIRE(state.normalization.inequality_row_scale.defined());
  REQUIRE(state.normalization.inequality_row_scale.numel() >= 1);
  REQUIRE(as_f64(state.normalization.inequality_row_scale.max()) > 1e3);
}

TEST_CASE("interior point computes objective and equality normalization on an ill-scaled problem",
          "[optim][normalization][ip]") {
  ConstrainedProblem problem;
  problem.objective.autodiff = [](const ParameterPack<Series>& p) -> Series {
    return 1.0e6 * ((p.at("x0") - 0.5).square() + (p.at("x1") - 0.5).square());
  };
  problem.nonlinear.equality = [](const ParameterPack<Series>& p) -> Series {
    return 1.0e-6 * (p.at("x0") + p.at("x1") - 1.0);
  };

  const ParameterPack<Series> initial({
      {"x0", scalar(0.2)},
      {"x1", scalar(0.8)},
  });

  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::InternalProtocol;
  options.internal_protocol.problem_scaling = OptimaProblemScaling::ObjectiveAndConstraints;

  const auto state = init_internal_protocol_state(problem, initial, options.internal_protocol);

  REQUIRE(state.normalization.objective_scale < 1e-3);
  REQUIRE(state.normalization.equality_row_scale.defined());
  REQUIRE(state.normalization.equality_row_scale.numel() == 1);
  REQUIRE(as_f64(state.normalization.equality_row_scale) > 1e3);
}

TEST_CASE("optima uses normalization to solve ill-scaled sqp and interior-point problems",
          "[optim][normalization][solve]") {
  ConstrainedProblem sqp_problem;
  const Series sqp_target = vec2(0.25, 1.25);
  sqp_problem.objective.autodiff = [sqp_target](const ParameterPack<Series>& p) -> Series {
    return 1.0e6 * ((p.at("x") - sqp_target).square()).sum();
  };
  sqp_problem.nonlinear.inequality = [](const ParameterPack<Series>& p) -> Series {
    return 1.0e-6 * (p.at("x").sum() - 1.5);
  };
  sqp_problem.bounds.lower = ParameterPack<Series>({
      {"x", vec2(0.0, 0.0)},
  });

  OptimaOptions sqp_options;
  sqp_options.algorithm = OptimaAlgorithm::SequentialQP;
  sqp_options.sequential_qp.max_iterations = 60;
  sqp_options.sequential_qp.problem_scaling = OptimaProblemScaling::ObjectiveAndConstraints;

  const auto sqp_result = optima(
      sqp_problem,
      ParameterPack<Series>({
          {"x", vec2(1.0, 1.0)},
      }),
      sqp_options);
  const auto sqp_x = sqp_result.params.at("x").host<double>();
  REQUIRE(sqp_result.evaluation.max_constraint_violation <= 1e-6);
  REQUIRE(sqp_x[0] == Approx(0.25).margin(5e-2));
  REQUIRE(sqp_x[1] == Approx(1.25).margin(5e-2));

  ConstrainedProblem ip_problem;
  ip_problem.objective.autodiff = [](const ParameterPack<Series>& p) -> Series {
    return 1.0e6 * ((p.at("x0") - 0.5).square() + (p.at("x1") - 0.5).square());
  };
  ip_problem.nonlinear.equality = [](const ParameterPack<Series>& p) -> Series {
    return 1.0e-6 * (p.at("x0") + p.at("x1") - 1.0);
  };

  OptimaOptions ip_options;
  ip_options.algorithm = OptimaAlgorithm::InternalProtocol;
  ip_options.internal_protocol.max_iterations = 80;
  ip_options.internal_protocol.problem_scaling = OptimaProblemScaling::ObjectiveAndConstraints;
  ip_options.internal_protocol.minimum_barrier_parameter = 1e-10;

  const auto ip_result = optima(ip_problem,
                                ParameterPack<Series>({
                                    {"x0", scalar(0.2)},
                                    {"x1", scalar(0.8)},
                                }),
                                ip_options);
  CAPTURE(static_cast<int>(ip_result.exit_flag), ip_result.message,
          as_f64(ip_result.evaluation.value),
          as_f64(ip_result.params.at("x0")),
          as_f64(ip_result.params.at("x1")),
          ip_result.output.first_order_optimality,
          as_f64(ip_result.evaluation.gradient.at("x0")),
          as_f64(ip_result.evaluation.gradient.at("x1")));

  REQUIRE(ip_result.evaluation.max_constraint_violation <= 1e-5);
  REQUIRE(as_f64(ip_result.params.at("x0")) == Approx(0.5).margin(1e-2));
  REQUIRE(as_f64(ip_result.params.at("x1")) == Approx(0.5).margin(1e-2));
}
