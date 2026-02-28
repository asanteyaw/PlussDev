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
using pluss::subsp::math::optim::InteriorPointOptions;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::solve_interior_point;
using pluss::subsp::math::optim::square;
using pluss::subsp::math::optim::sum;
using pluss::subsp::math::optim::trace;

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

TEST_CASE("phase3 interior point approaches the active constrained solution from the interior",
          "[optim][phase3][ip]") {
  const auto problem = make_problem();
  const ParameterPack<Series> initial({
      {"x", make_vec2(0.60, 0.60)},
  });

  InteriorPointOptions options;
  options.max_iterations = 80;
  options.initial_barrier_parameter = 0.1;
  options.minimum_barrier_parameter = 1e-7;
  options.barrier_decay = 0.2;
  options.constraint_tolerance = 1e-6;
  options.optimality_tolerance = 1e-5;

  const auto result = solve_interior_point(problem, initial, options);
  const auto x = result.params.at("x").to_vec<double>();

  REQUIRE(result.function_evaluations >= 1);
  REQUIRE(result.evaluation.max_constraint_violation <= 1e-4);
  REQUIRE(result.evaluation.value.item<double>() == Approx(1.125).margin(8e-2));
  REQUIRE(x[0] >= -1e-10);
  REQUIRE(x[1] >= -1e-10);
  REQUIRE(x[0] + x[1] <= Approx(1.5).margin(5e-4));
  REQUIRE(x[0] == Approx(0.25).margin(7e-2));
  REQUIRE(x[1] == Approx(1.25).margin(7e-2));
}
