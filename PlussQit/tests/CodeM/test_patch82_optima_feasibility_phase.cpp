#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>
#include <pluss/subsp/math/optim.h>

#include <string>
#include <vector>

namespace {

using Catch::Approx;
using pluss::ScalarType;
using pluss::Series;
using pluss::make_series;
using pluss::subsp::math::optim::ConstrainedProblem;
using pluss::subsp::math::optim::OptimaAlgorithm;
using pluss::subsp::math::optim::OptimaOptions;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::optima;

Series make_vec2(double x0, double x1) {
  return make_series(std::vector<double>{x0, x1}, {2}).to(ScalarType::Float64).to(pluss::pCPU);
}

} // namespace

TEST_CASE("interior-point feasibility phase improves an infeasible start",
          "[feasibility][optim][optima][interior-point]") {
  ConstrainedProblem problem;
  problem.objective.autodiff = [](const ParameterPack<Series>& p) {
    return (p.at("x") * p.at("x")).sum();
  };
  problem.nonlinear.inequality = [](const ParameterPack<Series>& p) {
    return 1.0 - p.at("x").sum();
  };
  problem.bounds.lower = ParameterPack<Series>({
      {"x", make_vec2(0.0, 0.0)},
  });

  const ParameterPack<Series> initial({
      {"x", make_vec2(0.1, 0.1)},
  });

  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::InternalProtocol;
  options.internal_protocol.max_iterations = 80;
  options.internal_protocol.enable_feasibility_phase = true;
  options.internal_protocol.constraint_tolerance = 1e-6;
  options.internal_protocol.optimality_tolerance = 1e-6;

  const auto result = optima(problem, initial, options);
  const auto x = result.params.at("x").host<double>();
  const bool mentioned_feasibility =
      result.message.find("feasibility") != std::string::npos ||
      result.output.detail_message.find("feasibility") != std::string::npos;

  REQUIRE(result.evaluation.max_constraint_violation <= 1e-4);
  REQUIRE(x[0] + x[1] == Approx(1.0).margin(1e-2));
  const bool terminated_normally = result.exit_flag == pluss::subsp::math::optim::OptimaExitFlag::Converged ||
                                   result.exit_flag == pluss::subsp::math::optim::OptimaExitFlag::MaxIterations;
  REQUIRE((mentioned_feasibility || terminated_normally));
}
