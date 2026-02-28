#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <pluss/pluss.h>

#include <vector>

namespace {

using Catch::Approx;
using pluss::ScalarType;
using pluss::Series;
using pluss::make_series;
using pluss::subsp::math::optim::Bounds;
using pluss::subsp::math::optim::ConstrainedProblem;
using pluss::subsp::math::optim::ParameterPack;
using pluss::subsp::math::optim::PenaltyMethodOptions;
using pluss::subsp::math::optim::evaluate_problem;
using pluss::subsp::math::optim::flatten_values;
using pluss::subsp::math::optim::solve_penalty_method;
using pluss::subsp::math::optim::trace;
using pluss::subsp::math::optim::sum;
using pluss::subsp::math::optim::square;

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

TEST_CASE("phase1 constrained evaluation reports objective and violations", "[optim][phase1]") {
  const auto problem = make_problem();
  const ParameterPack<Series> params({
      {"x", make_vec2(1.0, 1.0)},
  });

  const auto eval = evaluate_problem(problem, params);
  REQUIRE(eval.message == "ok");
  REQUIRE(eval.value.item<double>() == Approx(1.0));
  REQUIRE(eval.gradient.at("x").to_vec<double>()[0] == Approx(0.0));
  REQUIRE(eval.gradient.at("x").to_vec<double>()[1] == Approx(-2.0));
  REQUIRE(eval.max_constraint_violation == Approx(0.5));
  REQUIRE_FALSE(eval.feasible);

  const auto flat = flatten_values(params);
  REQUIRE(flat.numel() == 2);
}

TEST_CASE("phase1 penalty solver moves toward a feasible constrained point", "[optim][phase1][solver]") {
  const auto problem = make_problem();
  const ParameterPack<Series> initial({
      {"x", make_vec2(1.0, 1.0)},
  });

  PenaltyMethodOptions options;
  options.max_iterations = 80;
  options.initial_step_size = 0.2;
  options.penalty = 12.0;
  options.constraint_tolerance = 5e-3;
  options.optimality_tolerance = 5e-3;

  const auto result = solve_penalty_method(problem, initial, options);
  const auto x = result.params.at("x").to_vec<double>();

  REQUIRE(result.function_evaluations >= 1);
  REQUIRE(result.evaluation.max_constraint_violation <= 5e-2);
  REQUIRE(result.evaluation.value.item<double>() <= Approx(1.125).margin(2e-1));
  REQUIRE(x[0] >= -1e-12);
  REQUIRE(x[1] >= -1e-12);
  REQUIRE(x[0] + x[1] <= Approx(1.5).margin(5e-2));
  REQUIRE(x[0] == Approx(0.25).margin(2e-1));
  REQUIRE(x[1] == Approx(1.25).margin(2e-1));
}
