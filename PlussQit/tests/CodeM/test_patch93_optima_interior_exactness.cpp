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
using Pack = optim::ParameterPack<Series>;

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
  return [target](const Series& x) { return ((x - target).square()).sum(); };
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
  const auto x = r.params.at("x").host<double>();

  REQUIRE(r.exit_flag == optim::OptimaExitFlag::Converged);
  REQUIRE(x[0] + x[1] == Approx(2.0).margin(1e-9));
  REQUIRE(x[0] == Approx(1.5).margin(1e-6));
  REQUIRE(x[1] == Approx(0.5).margin(1e-6));
  REQUIRE(r.iteration <= 10);
}

TEST_CASE("interior method converges with an active linear inequality", "[optim][optima][interior][exact]") {
  const auto constraints = optim::constraints().ln_ineq(row({1, 1}), vec({2.0}));
  const auto r = optim::optima(distance_to(vec({2.0, 1.0})), vec({0.5, 0.5}), interior(), constraints);
  const auto x = r.params.at("x").host<double>();

  REQUIRE(r.exit_flag == optim::OptimaExitFlag::Converged);
  REQUIRE(x[0] == Approx(1.5).margin(1e-5));
  REQUIRE(x[1] == Approx(0.5).margin(1e-5));
  REQUIRE(r.iteration <= 20);
  REQUIRE(r.multipliers.linear_inequalities.host<double>()[0] == Approx(1.0).margin(1e-4));
}

TEST_CASE("interior method accepts a start exactly on a bound", "[optim][optima][interior][exact]") {
  const auto constraints = optim::constraints().lbs(vec({0.0, 0.0})).ubs(vec({1.5, 0.8}));
  const auto r = optim::optima(distance_to(vec({2.0, 1.0})), vec({0.0, 0.0}), interior(), constraints);
  const auto x = r.params.at("x").host<double>();

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
  const auto x = r.params.at("x").host<double>();

  REQUIRE(r.exit_flag == optim::OptimaExitFlag::Converged);
  REQUIRE(x[0] == Approx(1.0).margin(1e-4));
  REQUIRE(x[1] == Approx(1.0).margin(1e-4));
}

TEST_CASE("nonlinear equality is solved exactly by the interior method",
          "[optim][optima][exact]") {
  // min (1 - a)^2  s.t.  b = a^2, from a start on the wrong side. The solution is a = b = 1. The block b does
  // not appear in the objective, so its constraint derivative must survive every step.
  optim::ConstrainedProblem problem;
  problem.objective.autodiff = [](const Pack& p) { return (1.0 - p.at("a")).square(); };
  problem.nonlinear.equality = [](const Pack& p) {
    return 10.0 * (p.at("b") - p.at("a").square());
  };
  for (const auto algorithm : {optim::OptimaAlgorithm::InternalProtocol}) {
    optim::OptimaOptions options;
    options.algorithm = algorithm;
    const auto r = optim::optima(problem, scalars({{"a", -1.2}, {"b", 1.0}}), options);
    REQUIRE(r.exit_flag == optim::OptimaExitFlag::Converged);
    REQUIRE(as_f64(r.params.at("a")) == Approx(1.0).margin(1e-5));
    REQUIRE(as_f64(r.params.at("b")) == Approx(1.0).margin(1e-5));
    REQUIRE(r.evaluation.max_constraint_violation <= 1e-6);
  }
}

TEST_CASE("constraint derivatives do not depend on how the point was built", "[optim][optima][exact]") {
  // A point built as x + step carries history from that arithmetic; the derivative with respect to a
  // block the objective ignores must still come out right.
  optim::ConstrainedProblem problem;
  problem.objective.autodiff = [](const Pack& p) { return (1.0 - p.at("a")).square(); };
  problem.nonlinear.equality = [](const Pack& p) {
    return 10.0 * (p.at("b") - p.at("a").square());
  };
  const auto base = scalars({{"a", -0.5}, {"b", 0.2}});
  const auto first = optim::evaluate_problem(problem, base);
  const Series step = optim::flatten_values(first.gradient).to(ScalarType::Float64).mul(-0.1);
  const auto moved = optim::unflatten_values(base.layout(), optim::flatten_values(base).add(step));
  const auto second = optim::evaluate_problem(problem, moved);

  REQUIRE(second.equality_jacobian.at("a").host<double>()[0] == Approx(-20.0 * as_f64(moved.at("a"))));
  REQUIRE(second.equality_jacobian.at("b").host<double>()[0] == Approx(10.0));
}

TEST_CASE("sequential QP meets linear constraints to machine precision", "[optim][optima][exact]") {
  // A fixed convex QP with an equality and bounds: min |x - (3, 1, 0)|^2, sum(x) = 1, 0 <= x <= 1.
  // The nearest feasible point is (1, 0, 0).
  const auto constraints = optim::constraints().ln_eq(row({1, 1, 1}), vec({1.0})).lbs(vec({0, 0, 0})).ubs(vec({1, 1, 1}));
  const auto r = optim::optima(distance_to(vec({3.0, 1.0, 0.0})), vec({0.4, 0.3, 0.3}), {}, constraints);
  const auto x = r.params.at("x").host<double>();

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
      [](const Pack& p) { return (p.at("x").square()).sum() - 1.0; });
  const auto stopped = optim::optima(distance_to(vec({3.0, 3.0})), vec({0.0, 0.0}), limited, disc);
  REQUIRE(stopped.exit_flag == optim::OptimaExitFlag::MaxIterations);
  REQUIRE_FALSE(stopped.converged);

  const auto solved = optim::optima(distance_to(vec({2.0, 1.0})), vec({0.0, 0.0}));
  REQUIRE(solved.exit_flag == optim::OptimaExitFlag::Converged);
  REQUIRE(solved.converged);
}

TEST_CASE("optima reports invalid input before it starts", "[optim][optima][contract]") {
  const auto wrong_blocks = optim::constraints().lbs(vec({0.0, 0.0}));
  auto objective = [](const Pack& p) { return p.at("a").square() + p.at("b").square(); };
  REQUIRE_THROWS_WITH(optim::optima(objective, scalars({{"a", 1.0}, {"b", 1.0}}), {}, wrong_blocks),
                      Catch::Matchers::ContainsSubstring("same named blocks and shapes"));

  const auto r = optim::optima([](const Series& x) { return x.square(); }, vec({1.0, 1.0}));
  REQUIRE(r.exit_flag == optim::OptimaExitFlag::InvalidProblem);
  REQUIRE_FALSE(r.converged);
  REQUIRE_THAT(r.message, Catch::Matchers::ContainsSubstring("single number"));
}

TEST_CASE("reported first-order optimality matches the returned point", "[optim][optima][contract]") {
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return (p.at("x").square()).sum() - 1.0; });
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
  const auto x = r.params.at("x").host<double>();
  REQUIRE(r.exit_flag == optim::OptimaExitFlag::Converged);
  REQUIRE(x[0] + x[1] + x[2] == Approx(1.0).margin(1e-6));
  REQUIRE(x[0] == Approx(1.0).margin(1e-4));
  REQUIRE(x[1] == Approx(0.0).margin(1e-4));
  REQUIRE(x[2] == Approx(0.0).margin(1e-4));

  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return p.at("x").square().sum() - 1.0; });
  const auto d = optim::optima(distance_to(vec({1.0, 2.0})), vec({0.0, 0.0}), options, disc);
  const auto y = d.params.at("x").host<double>();
  REQUIRE(d.exit_flag == optim::OptimaExitFlag::Converged);
  REQUIRE(y[0] == Approx(1.0 / std::sqrt(5.0)).margin(1e-4));
  REQUIRE(y[1] == Approx(2.0 / std::sqrt(5.0)).margin(1e-4));
  REQUIRE(d.multipliers.nonlinear_inequalities.host<double>()[0] > 0.0);
}

TEST_CASE("every constrained algorithm reaches the same answer on a convex problem", "[optim][optima][exact]") {
  const auto constraints = optim::constraints().ln_ineq(row({1, 1}), vec({2.0})).lbs(vec({0.0, 0.0}));
  for (const auto algorithm : {optim::OptimaAlgorithm::MeritMethod, optim::OptimaAlgorithm::SequentialQP,
                               optim::OptimaAlgorithm::WorkingSet, optim::OptimaAlgorithm::InternalProtocol}) {
    optim::OptimaOptions options;
    options.algorithm = algorithm;
    const auto r = optim::optima(distance_to(vec({2.0, 1.0})), vec({0.5, 0.5}), options, constraints);
    const auto x = r.params.at("x").host<double>();
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
    const auto x = r.params.at("x").host<double>();
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
    const double v = as_f64(p.at("x"));
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
    REQUIRE(as_f64(r.params.at("x")) == Approx(3.0).margin(1e-5));
  }
}

TEST_CASE("independent nonlinear slacks keep the interior method from crawling off a curved boundary",
          "[optim][optima][interior][exact]") {
  // minimize x0*x3*(x0+x1+x2) + x2  s.t.  x0*x1*x2*x3 >= 25,  sum x^2 = 40,  1 <= x <= 5, started on the
  // boundary of the product constraint and on two bounds; the optimum is 17.0140172891.
  const auto f = [](const optim::ParameterPack<Series>& p) {
    return p.at("a") * p.at("d") * (p.at("a") + p.at("b") + p.at("c")) + p.at("c");
  };
  const auto constraints =
      optim::constraints()
          .nl_ineq([](const optim::ParameterPack<Series>& p) { return 25.0 - p.at("a") * p.at("b") * p.at("c") * p.at("d"); })
          .nl_eq([](const optim::ParameterPack<Series>& p) {
            return p.at("a").square() + p.at("b").square() + p.at("c").square() + p.at("d").square() - 40.0;
          })
          .lbs(scalars({{"a", 1}, {"b", 1}, {"c", 1}, {"d", 1}}))
          .ubs(scalars({{"a", 5}, {"b", 5}, {"c", 5}, {"d", 5}}));
  const auto start = scalars({{"a", 1}, {"b", 5}, {"c", 5}, {"d", 1}});
  for (bool independent : {true, false}) {
    optim::OptimaOptions options = interior();
    options.internal_protocol.independent_nonlinear_slacks = independent;
    const auto r = optim::optima(f, start, options, constraints);
    INFO("independent slacks " << independent);
    REQUIRE(r.exit_flag == optim::OptimaExitFlag::Converged);
    REQUIRE(r.evaluation.value.item<double>() == Approx(17.0140172891).margin(1e-5));
    if (independent) REQUIRE(r.iteration <= 20);
  }
}

TEST_CASE("without independent slacks every iterate satisfies the nonlinear inequality", "[optim][optima][interior][exact]") {
  const auto f = [](const optim::ParameterPack<Series>& p) {
    return (p.at("a") - 3.0).square() + (p.at("b") - 3.0).square();
  };
  const auto disc = optim::constraints().nl_ineq([](const optim::ParameterPack<Series>& p) {
    return p.at("a").square() + p.at("b").square() - 2.0;
  });
  optim::OptimaOptions options = interior();
  options.internal_protocol.independent_nonlinear_slacks = false;
  double worst = 0.0;
  options.progress_hook = [&](const optim::OptimaProgressSnapshot& s) {
    if (s.stage == optim::OptimaProgressStage::Iteration) worst = std::max(worst, s.evaluation.max_constraint_violation);
    return false;
  };
  const auto r = optim::optima(f, scalars({{"a", 0.0}, {"b", 0.0}}), options, disc);
  REQUIRE(r.exit_flag == optim::OptimaExitFlag::Converged);
  REQUIRE(worst <= 1e-9);
}

TEST_CASE("sequential qp converges on a smoothing fit whose quadratic subproblem is ill-conditioned late in its interior iteration",
          "[optim][sqp][linear-policy]") {
  const int n = 32;
  std::vector<double> target(n), start(n), lo(n, 0.0), hi(n, 1.0);
  double total = 0.0;
  for (int i = 0; i < n; ++i) {
    target[i] = 0.2 + 0.6 * i / (n - 1) + 0.15 * std::sin(1.7 * i);
    start[i] = (i % 2 == 0) ? 0.9 : 0.1;
    total += target[i];
  }
  std::vector<double> diff(static_cast<std::size_t>(2 * (n - 1) * n), 0.0);
  std::vector<double> cap(static_cast<std::size_t>(2 * (n - 1)), 0.12);
  for (int i = 0; i + 1 < n; ++i) {
    diff[static_cast<std::size_t>((2 * i) * n + i)] = 1.0;
    diff[static_cast<std::size_t>((2 * i) * n + i + 1)] = -1.0;
    diff[static_cast<std::size_t>((2 * i + 1) * n + i)] = -1.0;
    diff[static_cast<std::size_t>((2 * i + 1) * n + i + 1)] = 1.0;
  }
  const auto constraints =
      optim::constraints()
          .ln_ineq(make_series(diff, {2 * (n - 1), n}).to(ScalarType::Float64), vec(cap))
          .ln_eq(make_series(std::vector<double>(n, 1.0), {1, n}).to(ScalarType::Float64), vec({total}))
          .lbs(vec(lo))
          .ubs(vec(hi));
  optim::OptimaOptions options;
  options.algorithm = optim::OptimaAlgorithm::SequentialQP;
  options.sequential_qp.max_iterations = 500;
  const auto r = optim::optima(distance_to(vec(target)), vec(start), options, constraints);
  REQUIRE(r.exit_flag == optim::OptimaExitFlag::Converged);
  REQUIRE(r.evaluation.max_constraint_violation <= 1e-6);
}

TEST_CASE("interior method with conjugate-gradient steps reaches the same optimum as with direct steps",
          "[optim][interior][linear-policy]") {
  const int n = 40;
  std::vector<double> target(n), start(n);
  double total = 0.0;
  for (int i = 0; i < n; ++i) {
    target[i] = 0.2 + 0.6 * i / (n - 1) + 0.15 * std::sin(1.7 * i);
    start[i] = (i % 2 == 0) ? 0.9 : 0.1;
    total += target[i];
  }
  std::vector<double> diff(static_cast<std::size_t>(2 * (n - 1) * n), 0.0);
  std::vector<double> cap(static_cast<std::size_t>(2 * (n - 1)), 0.12);
  for (int i = 0; i + 1 < n; ++i) {
    diff[static_cast<std::size_t>((2 * i) * n + i)] = 1.0;
    diff[static_cast<std::size_t>((2 * i) * n + i + 1)] = -1.0;
    diff[static_cast<std::size_t>((2 * i + 1) * n + i)] = -1.0;
    diff[static_cast<std::size_t>((2 * i + 1) * n + i + 1)] = 1.0;
  }
  const auto constraints =
      optim::constraints()
          .ln_ineq(make_series(diff, {2 * (n - 1), n}).to(ScalarType::Float64), vec(cap))
          .ln_eq(make_series(std::vector<double>(n, 1.0), {1, n}).to(ScalarType::Float64), vec({total}))
          .lbs(vec(std::vector<double>(n, 0.0)))
          .ubs(vec(std::vector<double>(n, 1.0)));
  auto solve = [&](optim::OptimaLinearSystemSolver solver) {
    optim::OptimaOptions options;
    options.algorithm = optim::OptimaAlgorithm::InternalProtocol;
    options.internal_protocol.linear_system_solver = solver;
    options.internal_protocol.system_structure = solver == optim::OptimaLinearSystemSolver::ConjugateGradient
                                                     ? optim::OptimaLinearSystemStructure::Sparse
                                                     : optim::OptimaLinearSystemStructure::Dense;
    return optim::optima(distance_to(vec(target)), vec(start), options, constraints);
  };
  const auto direct = solve(optim::OptimaLinearSystemSolver::Cholesky);
  const auto krylov = solve(optim::OptimaLinearSystemSolver::ConjugateGradient);
  REQUIRE(direct.exit_flag == optim::OptimaExitFlag::Converged);
  REQUIRE(krylov.exit_flag == optim::OptimaExitFlag::Converged);
  REQUIRE(krylov.evaluation.value.item<double>() == Approx(direct.evaluation.value.item<double>()).margin(1e-5));
}

TEST_CASE("every algorithm solves a problem written in extreme units, and multipliers come back in the caller's units",
          "[optim][optima][scaling]") {
  for (double K : {1e-6, 1e6, 1e9}) {
    auto objective = [K](const Pack& p) {
      return K * (p.at("a") - 1.0).square() + (1.0 / K) * (p.at("b") - 1500.0).square() + K * 0.01 * p.at("a");
    };
    const auto constraints =
        optim::constraints()
            .nl_eq([K](const Pack& p) { return K * (p.at("a").square() + (p.at("b") * 0.001).square() - 3.0); })
            .nl_ineq([K](const Pack& p) { return (p.at("b") - 1400.0) / K; });
    for (auto algorithm : {optim::OptimaAlgorithm::SequentialQP, optim::OptimaAlgorithm::WorkingSet,
                           optim::OptimaAlgorithm::InternalProtocol}) {
      optim::OptimaOptions options;
      options.algorithm = algorithm;
      options.sequential_qp.max_iterations = 300;
      options.working_set.max_iterations = 300;
      options.internal_protocol.max_iterations = 300;
      const auto r = optim::optima(objective, scalars({{"a", 0.3}, {"b", 900.0}}), options, constraints);
      INFO("K = " << K << ", algorithm " << static_cast<int>(algorithm));
      REQUIRE(r.exit_flag == optim::OptimaExitFlag::Converged);
      // The solution a ~ 1.0198, b = 1400 (the inequality binds) in natural units.
      REQUIRE(as_f64(r.params.at("a")) == Approx(1.0198).margin(2e-3));
      REQUIRE(as_f64(r.params.at("b")) == Approx(1400.0).margin(1.0));
    }
  }

  // d f / d x = 2 K (x - 3) = -2 K at x = 2, so the multiplier of x <= 2 is 2 K in the caller's units.
  const double K = 1e6;
  auto objective = [K](const Series& x) { return K * (x - 3.0).square().sum(); };
  const auto r = optim::optima(objective, vec({0.0}), {}, optim::constraints().ln_ineq(row({1.0}), vec({2.0})));
  REQUIRE(r.exit_flag == optim::OptimaExitFlag::Converged);
  REQUIRE(r.multipliers.linear_inequalities.host<double>()[0] == Approx(2.0 * K).epsilon(1e-3));
  REQUIRE(r.evaluation.value.item<double>() == Approx(K).epsilon(1e-6));
}

TEST_CASE("the quadratic-model algorithms solve a smoothing fit with a hundred and twenty variables and several hundred rows",
          "[optim][optima][linear-policy]") {
  // 238 smoothness rows, one equality and bounds: the subproblem has ~700 inequality rows, whose late interior
  // iterations are ill-conditioned. Each algorithm must reach the same optimum (to the tolerance) quickly.
  const int n = 120;
  std::vector<double> target(n), start(n);
  double total = 0.0;
  for (int i = 0; i < n; ++i) {
    target[i] = 0.2 + 0.6 * i / (n - 1) + 0.15 * std::sin(1.7 * i);
    start[i] = (i % 2 == 0) ? 0.9 : 0.1;
    total += target[i];
  }
  std::vector<double> diff(static_cast<std::size_t>(2 * (n - 1) * n), 0.0);
  std::vector<double> cap(static_cast<std::size_t>(2 * (n - 1)), 0.12);
  for (int i = 0; i + 1 < n; ++i) {
    diff[static_cast<std::size_t>((2 * i) * n + i)] = 1.0;
    diff[static_cast<std::size_t>((2 * i) * n + i + 1)] = -1.0;
    diff[static_cast<std::size_t>((2 * i + 1) * n + i)] = -1.0;
    diff[static_cast<std::size_t>((2 * i + 1) * n + i + 1)] = 1.0;
  }
  const auto constraints =
      optim::constraints()
          .ln_ineq(make_series(diff, {2 * (n - 1), n}).to(ScalarType::Float64), vec(cap))
          .ln_eq(make_series(std::vector<double>(n, 1.0), {1, n}).to(ScalarType::Float64), vec({total}))
          .lbs(vec(std::vector<double>(n, 0.0)))
          .ubs(vec(std::vector<double>(n, 1.0)));
  std::vector<double> values;
  for (auto algorithm : {optim::OptimaAlgorithm::SequentialQP, optim::OptimaAlgorithm::WorkingSet,
                         optim::OptimaAlgorithm::InternalProtocol}) {
    optim::OptimaOptions options;
    options.algorithm = algorithm;
    const auto r = optim::optima(distance_to(vec(target)), vec(start), options, constraints);
    INFO("algorithm " << static_cast<int>(algorithm));
    REQUIRE(r.exit_flag == optim::OptimaExitFlag::Converged);
    REQUIRE(r.evaluation.max_constraint_violation <= 1e-5);
    if (algorithm == optim::OptimaAlgorithm::SequentialQP) REQUIRE(r.function_evaluations <= 20);
    values.push_back(r.evaluation.value.item<double>());
  }
  REQUIRE(values[0] == Approx(values[2]).margin(1e-4));
  REQUIRE(values[1] == Approx(values[2]).margin(1e-4));
}
