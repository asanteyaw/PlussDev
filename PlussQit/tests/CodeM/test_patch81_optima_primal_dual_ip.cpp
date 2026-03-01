#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>

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
using pluss::subsp::math::optim::Trace;
using pluss::subsp::math::optim::optima;
using pluss::subsp::math::optim::square;
using pluss::subsp::math::optim::sum;
using pluss::subsp::math::optim::trace;

Series make_vec2(double x0, double x1) {
  return make_series(std::vector<double>{x0, x1}, {2}).to(ScalarType::Float64).to(pluss::pCPU);
}

} // namespace

TEST_CASE("interior-point tracks primal-dual complementarity on a constrained problem",
          "[optim][optima][interior-point][primal-dual]") {
  const Series target = make_vec2(1.0, 2.0);

  ConstrainedProblem problem;
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

  const ParameterPack<Series> initial({
      {"x", make_vec2(0.7, 0.7)},
  });

  OptimaOptions options;
  options.algorithm = OptimaAlgorithm::InteriorPoint;
  options.interior_point.max_iterations = 80;
  options.interior_point.minimum_barrier_parameter = 1e-10;
  options.interior_point.constraint_tolerance = 1e-6;
  options.interior_point.optimality_tolerance = 1e-6;

  const auto result = optima(problem, initial, options);
  const auto x = result.params.at("x").to_vec<double>();

  REQUIRE(result.evaluation.max_constraint_violation <= 1e-4);
  REQUIRE(result.output.complementarity >= 0.0);
  REQUIRE(result.output.complementarity < 1e-3);
  REQUIRE(x[0] >= -1e-12);
  REQUIRE(x[1] >= -1e-12);
  REQUIRE(x[0] + x[1] == Approx(1.5).margin(5e-2));
  REQUIRE(result.evaluation.value.item<double>() == Approx(1.125).margin(2e-1));
}
