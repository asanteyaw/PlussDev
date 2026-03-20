#include <catch2/catch_test_macros.hpp>
#include <pluss/ma.h>

#include <cmath>
#include <vector>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::pCPU;
using namespace pluss::subsp::math::optimization;

double as_f64_scalar(const Series& x) {
  return x.to(pCPU, ScalarType::Float64).item<double>();
}

} // namespace

TEST_CASE("DFO PatternSearch exposes status and diagnostics for infeasible regimes", "[pattern_search][dfo][status]") {
  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pCPU);

  DfoProblem problem;
  problem.initial = pluss::make_series(std::vector<double>{0.0, 0.0}, {2}, cfg);
  problem.objective = [](const Series& x) { return x.square().sum(); };
  problem.constraints = [](const Series&) { return pluss::make_series(std::vector<double>{1.0}, {1}); };

  DfoOptions options;
  options.solver = DfoSolverKind::PatternSearch;
  options.max_iterations = 120;
  options.max_evaluations = 500;
  options.pattern_search.initial_mesh_size = 0.5;
  options.pattern_search.mesh_tolerance = 1e-9;
  options.pattern_search.constraint_policy = PSConstraintPolicy::ExtremeBarrier;

  const DfoResult out = DFO::solve(problem, options);

  REQUIRE(out.selected_solver == DfoSolverKind::PatternSearch);
  REQUIRE(out.status == DfoStatus::InfeasibleStationary);
  REQUIRE(out.diagnostics.max_constraint_violation > 0.5);
}

TEST_CASE("DFO PatternSearch propagates time-budget termination", "[pattern_search][dfo][time]") {
  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pCPU);

  DfoProblem problem;
  problem.initial = pluss::full({4}, 2.0, cfg);
  problem.objective = [](const Series& x) { return x.square().sum(); };

  DfoOptions options;
  options.solver = DfoSolverKind::PatternSearch;
  options.max_iterations = 500;
  options.max_evaluations = 10000;
  options.pattern_search.time_limit_seconds = 1e-12;

  const DfoResult out = DFO::solve(problem, options);
  REQUIRE(out.selected_solver == DfoSolverKind::PatternSearch);
  REQUIRE((out.status == DfoStatus::ReachedTimeBudget ||
           out.status == DfoStatus::ReachedEvaluationBudget));
}

TEST_CASE("PatternSearch random-search diagnostics and AL feasibility are preserved through DFO", "[pattern_search][dfo][closure]") {
  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pCPU);

  DfoProblem constrained;
  constrained.initial = pluss::make_series(std::vector<double>{1.8, 1.7}, {2}, cfg);
  constrained.objective = [](const Series& x) {
    const Series t = pluss::make_series(std::vector<double>{0.3, 0.7}, {2},
                                        Config{}.with_dtype(x.scalar_type()).with_platform(x.platform()));
    return x.sub(t).square().sum();
  };
  constrained.constraints_values = [](const Series& x) {
    DfoConstraintValues c;
    c.inequality = x.square().sum().sub(1.0).reshape({1});
    c.equality = x.sum().sub(1.0).reshape({1});
    return c;
  };
  constrained.lower_bounds = pluss::full({2}, -2.0, cfg);
  constrained.upper_bounds = pluss::full({2}, 2.0, cfg);

  DfoOptions options;
  options.solver = DfoSolverKind::PatternSearch;
  options.max_iterations = 500;
  options.max_evaluations = 14000;
  options.pattern_search.constraint_policy = PSConstraintPolicy::AugmentedLagrangian;
  options.pattern_search.al_max_outer_iterations = 6;
  options.pattern_search.al_penalty_growth = 4.0;
  options.pattern_search.al_violation_reduction_ratio = 0.6;
  options.pattern_search.search_strategy = PSSearchStrategy::ExpandAndRandomDirections;
  options.pattern_search.search_trials = 16;
  options.pattern_search.search_complete = false;
  options.pattern_search.basis_family = PSBasisFamily::RandomOrtho;
  options.pattern_search.basis_refresh_interval = 2;

  const DfoResult out = DFO::solve(constrained, options);
  const Series x = out.minimizer.to(pCPU, ScalarType::Float64).reshape({2});
  const double x0 = as_f64_scalar(x.select(0, 0));
  const double x1 = as_f64_scalar(x.select(0, 1));
  const double eq_resid = std::abs((x0 + x1) - 1.0);
  const double ineq = x0 * x0 + x1 * x1 - 1.0;

  REQUIRE(out.selected_solver == DfoSolverKind::PatternSearch);
  REQUIRE(out.diagnostics.outer_iterations >= 1);
  REQUIRE(out.diagnostics.search_trials > 0);
  REQUIRE(out.diagnostics.search_successes > 0);
  REQUIRE(eq_resid <= 1e-3);
  REQUIRE(ineq <= 1e-3);
  REQUIRE(out.diagnostics.max_constraint_violation <= 1e-3);
}
