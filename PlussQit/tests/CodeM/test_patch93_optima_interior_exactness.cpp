// Exactness of the constrained solvers on problems whose answers are known in closed form: linear and
// nonlinear equalities are met to tolerance (not merely penalized), active inequalities and bounds
// converge in a handful of iterations, a start exactly on a bound is accepted, and a parameter block
// that the objective does not depend on keeps its constraint derivative after the first step.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <pluss/pluss.h>
#include <pluss/subsp/math/optim.h>

#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace {

using Catch::Approx;
using pluss::ScalarType;
using pluss::Series;
using pluss::make_series;
namespace optim = pluss::subsp::math::optim;
using Pack = optim::ParameterPack<optim::Trace>;

Series vec(std::vector<double> v) {
  return make_series(v, {static_cast<std::int64_t>(v.size())}).to(ScalarType::Float64);
}

Series row(std::vector<double> v) {
  return make_series(v, {1, static_cast<std::int64_t>(v.size())}).to(ScalarType::Float64);
}

optim::ParameterPack<Series> scalars(std::vector<std::pair<std::string, double>> blocks) {
  std::vector<optim::ParameterPack<Series>::Block> out;
  for (auto& [name, value] : blocks) {
    out.push_back({name, make_series(value).to(ScalarType::Float64).reshape({1})});
  }
  return optim::ParameterPack<Series>(std::move(out));
}

auto distance_to(const Series& target) {
  return [target](const optim::Trace& x) { return optim::sum(optim::square(x - optim::trace(target, false))); };
}

optim::OptimaOptions interior() {
  optim::OptimaOptions options;
  options.algorithm = optim::OptimaAlgorithm::InternalProtocol;
  return options;
}

} // namespace

TEST_CASE("interior method meets a linear equality to tolerance", "[optim][optima][interior][exact]") {
  const auto constraints = optim::constraints().ln_eq(row({1, 1}), vec({2.0}));
  const auto r = optim::optima(distance_to(vec({2.0, 1.0})), vec({0.5, 0.5}), interior(), constraints);
  const auto x = r.params.at("x").to_vec<double>();

  REQUIRE(r.exit_flag == optim::OptimaExitFlag::Converged);
  REQUIRE(x[0] + x[1] == Approx(2.0).margin(1e-9));
  REQUIRE(x[0] == Approx(1.5).margin(1e-6));
  REQUIRE(x[1] == Approx(0.5).margin(1e-6));
  REQUIRE(r.iteration <= 10);
}

TEST_CASE("interior method converges with an active linear inequality", "[optim][optima][interior][exact]") {
  const auto constraints = optim::constraints().ln_ineq(row({1, 1}), vec({2.0}));
  const auto r = optim::optima(distance_to(vec({2.0, 1.0})), vec({0.5, 0.5}), interior(), constraints);
  const auto x = r.params.at("x").to_vec<double>();

  REQUIRE(r.exit_flag == optim::OptimaExitFlag::Converged);
  REQUIRE(x[0] == Approx(1.5).margin(1e-5));
  REQUIRE(x[1] == Approx(0.5).margin(1e-5));
  REQUIRE(r.iteration <= 20);
  REQUIRE(r.multipliers.linear_inequalities.to_vec<double>()[0] == Approx(1.0).margin(1e-4));
}

TEST_CASE("interior method accepts a start exactly on a bound", "[optim][optima][interior][exact]") {
  const auto constraints = optim::constraints().lbs(vec({0.0, 0.0})).ubs(vec({1.5, 0.8}));
  const auto r = optim::optima(distance_to(vec({2.0, 1.0})), vec({0.0, 0.0}), interior(), constraints);
  const auto x = r.params.at("x").to_vec<double>();

  REQUIRE(r.exit_flag == optim::OptimaExitFlag::Converged);
  REQUIRE(x[0] == Approx(1.5).margin(1e-4));
  REQUIRE(x[1] == Approx(0.8).margin(1e-4));
}

TEST_CASE("interior method finds a strictly feasible region from an infeasible start",
          "[optim][optima][interior][exact]") {
  // The start violates three inequalities at once; the feasible region is the triangle below.
  const Series A = make_series(std::vector<double>{1, 0, 0, 1, -1, -1}, {3, 2}).to(ScalarType::Float64);
  const auto constraints = optim::constraints().ln_ineq(A, vec({1.0, 1.0, 0.0}));
  const auto r = optim::optima(distance_to(vec({3.0, 3.0})), vec({5.0, 5.0}), interior(), constraints);
  const auto x = r.params.at("x").to_vec<double>();

  REQUIRE(r.exit_flag == optim::OptimaExitFlag::Converged);
  REQUIRE(x[0] == Approx(1.0).margin(1e-4));
  REQUIRE(x[1] == Approx(1.0).margin(1e-4));
}

TEST_CASE("nonlinear equality is solved exactly by the interior method",
          "[optim][optima][exact]") {
  // min (1 - a)^2  s.t.  b = a^2, from a start on the wrong side. The solution is a = b = 1. The block b does
  // not appear in the objective, so its constraint derivative must survive every step.
  optim::ConstrainedProblem problem;
  problem.objective.autodiff = [](const Pack& p) { return optim::square(1.0 - p.at("a")); };
  problem.nonlinear.equality = [](const Pack& p) {
    return 10.0 * (p.at("b") - optim::square(p.at("a")));
  };
  for (const auto algorithm : {optim::OptimaAlgorithm::InternalProtocol}) {
    optim::OptimaOptions options;
    options.algorithm = algorithm;
    const auto r = optim::optima(problem, scalars({{"a", -1.2}, {"b", 1.0}}), options);
    REQUIRE(r.exit_flag == optim::OptimaExitFlag::Converged);
    REQUIRE(r.params.at("a").item<double>() == Approx(1.0).margin(1e-5));
    REQUIRE(r.params.at("b").item<double>() == Approx(1.0).margin(1e-5));
    REQUIRE(r.evaluation.max_constraint_violation <= 1e-6);
  }
}

TEST_CASE("constraint derivatives do not depend on how the point was built", "[optim][optima][exact]") {
  // A point built as x + step carries history from that arithmetic; the derivative with respect to a
  // block the objective ignores must still come out right.
  optim::ConstrainedProblem problem;
  problem.objective.autodiff = [](const Pack& p) { return optim::square(1.0 - p.at("a")); };
  problem.nonlinear.equality = [](const Pack& p) {
    return 10.0 * (p.at("b") - optim::square(p.at("a")));
  };
  const auto base = scalars({{"a", -0.5}, {"b", 0.2}});
  const auto first = optim::evaluate_problem(problem, base);
  const Series step = optim::flatten_values(first.gradient).to(ScalarType::Float64).mul(-0.1);
  const auto moved = optim::unflatten_values(base.layout(), optim::flatten_values(base).add(step));
  const auto second = optim::evaluate_problem(problem, moved);

  REQUIRE(second.equality_jacobian.at("a").to_vec<double>()[0] == Approx(-20.0 * moved.at("a").item<double>()));
  REQUIRE(second.equality_jacobian.at("b").to_vec<double>()[0] == Approx(10.0));
}

TEST_CASE("sequential QP meets linear constraints to machine precision", "[optim][optima][exact]") {
  // A fixed convex QP with an equality and bounds: min |x - (3, 1, 0)|^2, sum(x) = 1, 0 <= x <= 1.
  // The nearest feasible point is (1, 0, 0).
  const auto constraints = optim::constraints().ln_eq(row({1, 1, 1}), vec({1.0})).lbs(vec({0, 0, 0})).ubs(vec({1, 1, 1}));
  const auto r = optim::optima(distance_to(vec({3.0, 1.0, 0.0})), vec({0.4, 0.3, 0.3}), {}, constraints);
  const auto x = r.params.at("x").to_vec<double>();

  REQUIRE(r.exit_flag == optim::OptimaExitFlag::Converged);
  REQUIRE(x[0] + x[1] + x[2] == Approx(1.0).margin(1e-9));
  REQUIRE(x[0] == Approx(1.0).margin(1e-5));
  REQUIRE(x[1] == Approx(0.0).margin(1e-5));
  REQUIRE(x[2] == Approx(0.0).margin(1e-5));
}

TEST_CASE("converged reports success, not termination", "[optim][optima][contract]") {
  optim::OptimaOptions limited;
  limited.sequential_qp.max_iterations = 1;
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) - 1.0; });
  const auto stopped = optim::optima(distance_to(vec({3.0, 3.0})), vec({0.0, 0.0}), limited, disc);
  REQUIRE(stopped.exit_flag == optim::OptimaExitFlag::MaxIterations);
  REQUIRE_FALSE(stopped.converged);

  const auto solved = optim::optima(distance_to(vec({2.0, 1.0})), vec({0.0, 0.0}));
  REQUIRE(solved.exit_flag == optim::OptimaExitFlag::Converged);
  REQUIRE(solved.converged);
}

TEST_CASE("optima reports invalid input before it starts", "[optim][optima][contract]") {
  const auto wrong_blocks = optim::constraints().lbs(vec({0.0, 0.0}));
  auto objective = [](const Pack& p) { return optim::square(p.at("a")) + optim::square(p.at("b")); };
  REQUIRE_THROWS_WITH(optim::optima(objective, scalars({{"a", 1.0}, {"b", 1.0}}), {}, wrong_blocks),
                      Catch::Matchers::ContainsSubstring("same named blocks and shapes"));

  const auto r = optim::optima([](const optim::Trace& x) { return optim::square(x); }, vec({1.0, 1.0}));
  REQUIRE(r.exit_flag == optim::OptimaExitFlag::InvalidProblem);
  REQUIRE_FALSE(r.converged);
  REQUIRE_THAT(r.message, Catch::Matchers::ContainsSubstring("single number"));
}

TEST_CASE("reported first-order optimality matches the returned point", "[optim][optima][contract]") {
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) - 1.0; });
  const auto r = optim::optima(distance_to(vec({3.0, 3.0})), vec({0.0, 0.0}), {}, disc);
  REQUIRE(r.exit_flag == optim::OptimaExitFlag::Converged);
  REQUIRE(r.output.first_order_optimality <= 1e-5);
}

TEST_CASE("merit method (augmented Lagrangian) solves equality, inequality and bound constraints",
          "[optim][optima][merit][exact]") {
  optim::OptimaOptions options;
  options.algorithm = optim::OptimaAlgorithm::MeritMethod;

  const auto linear = optim::constraints().ln_eq(row({1, 1, 1}), vec({1.0})).lbs(vec({0, 0, 0})).ubs(vec({1, 1, 1}));
  const auto r = optim::optima(distance_to(vec({3.0, 1.0, 0.0})), vec({0.4, 0.3, 0.3}), options, linear);
  const auto x = r.params.at("x").to_vec<double>();
  REQUIRE(r.exit_flag == optim::OptimaExitFlag::Converged);
  REQUIRE(x[0] + x[1] + x[2] == Approx(1.0).margin(1e-6));
  REQUIRE(x[0] == Approx(1.0).margin(1e-4));
  REQUIRE(x[1] == Approx(0.0).margin(1e-4));
  REQUIRE(x[2] == Approx(0.0).margin(1e-4));

  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) - 1.0; });
  const auto d = optim::optima(distance_to(vec({1.0, 2.0})), vec({0.0, 0.0}), options, disc);
  const auto y = d.params.at("x").to_vec<double>();
  REQUIRE(d.exit_flag == optim::OptimaExitFlag::Converged);
  REQUIRE(y[0] == Approx(1.0 / std::sqrt(5.0)).margin(1e-4));
  REQUIRE(y[1] == Approx(2.0 / std::sqrt(5.0)).margin(1e-4));
  REQUIRE(d.multipliers.nonlinear_inequalities.to_vec<double>()[0] > 0.0);
}

TEST_CASE("every constrained algorithm reaches the same answer on a convex problem", "[optim][optima][exact]") {
  const auto constraints = optim::constraints().ln_ineq(row({1, 1}), vec({2.0})).lbs(vec({0.0, 0.0}));
  for (const auto algorithm : {optim::OptimaAlgorithm::MeritMethod, optim::OptimaAlgorithm::SequentialQP,
                               optim::OptimaAlgorithm::WorkingSet, optim::OptimaAlgorithm::InternalProtocol}) {
    optim::OptimaOptions options;
    options.algorithm = algorithm;
    const auto r = optim::optima(distance_to(vec({2.0, 1.0})), vec({0.5, 0.5}), options, constraints);
    const auto x = r.params.at("x").to_vec<double>();
    INFO("algorithm " << static_cast<int>(algorithm));
    REQUIRE(r.exit_flag == optim::OptimaExitFlag::Converged);
    REQUIRE(x[0] == Approx(1.5).margin(1e-4));
    REQUIRE(x[1] == Approx(0.5).margin(1e-4));
  }
}

TEST_CASE("infinite bounds mean unbounded for every constrained algorithm", "[optim][optima][exact][bounds]") {
  const double inf = std::numeric_limits<double>::infinity();
  // min (x0-2)^2 + (x1-1)^2 with x0 >= -inf, x1 in [0, +inf), x0 + x1 >= 4  -> (2.5, 1.5)
  const auto constraints = optim::constraints()
                               .lbs(vec({-inf, 0.0}))
                               .ubs(vec({inf, inf}))
                               .ln_ineq(row({-1, -1}), vec({-4.0}));
  for (auto algorithm : {optim::OptimaAlgorithm::InternalProtocol, optim::OptimaAlgorithm::SequentialQP,
                         optim::OptimaAlgorithm::WorkingSet, optim::OptimaAlgorithm::MeritMethod}) {
    optim::OptimaOptions options;
    options.algorithm = algorithm;
    INFO("algorithm " << static_cast<int>(algorithm));
    const auto r = optim::optima(distance_to(vec({2.0, 1.0})), vec({3.0, 3.0}), options, constraints);
    const auto x = r.params.at("x").to_vec<double>();
    REQUIRE(r.exit_flag == optim::OptimaExitFlag::Converged);
    REQUIRE(x[0] == Approx(2.5).margin(1e-5));
    REQUIRE(x[1] == Approx(1.5).margin(1e-5));
  }
}

TEST_CASE("every algorithm backs off an infinite objective wall and converges", "[optim][optima][exact][nonfinite]") {
  // f = 100 (x-3)^2 for x > 2.5 and +inf otherwise; the first Newton-type step from x = 4 lands in the wall.
  optim::ConstrainedProblem problem;
  problem.objective.analytic = [](const optim::ParameterPack<Series>& p)
      -> std::pair<Series, optim::ParameterPack<Series>> {
    const double v = p.at("x").item<double>();
    const double inf = std::numeric_limits<double>::infinity();
    if (!(v > 2.5)) {
      return {make_series(inf).to(ScalarType::Float64),
              optim::ParameterPack<Series>({{"x", make_series(inf).to(ScalarType::Float64).reshape({1})}})};
    }
    return {make_series(100.0 * (v - 3.0) * (v - 3.0)).to(ScalarType::Float64),
            optim::ParameterPack<Series>({{"x", make_series(200.0 * (v - 3.0)).to(ScalarType::Float64).reshape({1})}})};
  };
  for (auto algorithm : {optim::OptimaAlgorithm::InternalProtocol, optim::OptimaAlgorithm::SequentialQP,
                         optim::OptimaAlgorithm::WorkingSet, optim::OptimaAlgorithm::MeritMethod}) {
    optim::OptimaOptions options;
    options.algorithm = algorithm;
    INFO("algorithm " << static_cast<int>(algorithm));
    const auto r = optim::optima(problem, scalars({{"x", 4.0}}), options);
    REQUIRE(r.exit_flag == optim::OptimaExitFlag::Converged);
    REQUIRE(r.params.at("x").item<double>() == Approx(3.0).margin(1e-5));
  }
}
