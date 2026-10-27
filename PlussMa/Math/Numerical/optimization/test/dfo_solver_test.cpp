#include <limits>
#include "../dfo_solver.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::pCPU;
using namespace pluss::subsp::math::optimization;

double as_f64_scalar(const Series& x) {
  if (!x.defined() || x.numel() != 1) {
    throw std::runtime_error("expected scalar Series");
  }
  return x.to(pCPU, ScalarType::Float64).item<double>();
}

void expect(bool cond, const std::string& msg, int* failures) {
  if (!cond) {
    ++(*failures);
    std::cerr << "[FAIL] " << msg << '\n';
  }
}

Series sphere(const Series& x) {
  return x.square().sum();
}

Series sum_le_one_constraint(const Series& x) {
  return x.sum().sub(1.0).reshape({1});
}

} // namespace

int main() {
  int failures = 0;

  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pCPU);
  const Series x0 = pluss::series::full({4}, 2.0, cfg);

  // Auto: low-dimensional unconstrained -> NelderMead.
  DfoProblem unconstrained;
  unconstrained.initial = x0;
  unconstrained.objective = sphere;

  DfoOptions auto_small;
  auto_small.max_iterations = 600;
  auto_small.max_evaluations = 2400;
  auto_small.relative_improvement_tolerance = 1e-8;
  const DfoResult r1 = DFO::solve(unconstrained, auto_small);
  std::cerr << "auto-small solver=" << to_string(r1.selected_solver)
            << " f=" << r1.minimum
            << " reason=" << r1.selection_reason << '\n';
  expect(r1.selected_solver == DfoSolverKind::NelderMead,
         "Auto selection did not choose NelderMead for small unconstrained problem",
         &failures);
  expect(std::isfinite(r1.minimum), "NelderMead auto run returned non-finite objective", &failures);
  expect(r1.minimum < 1e-6, "NelderMead auto run failed sphere target", &failures);

  // Auto: boxed problem -> DifferentialEvolution.
  DfoProblem boxed = unconstrained;
  boxed.lower_bounds = pluss::series::full({4}, -1.0, cfg);
  boxed.upper_bounds = pluss::series::full({4}, 1.0, cfg);

  DfoOptions auto_boxed;
  auto_boxed.max_iterations = 250;
  auto_boxed.max_evaluations = 10000;
  auto_boxed.check_every = 10;
  auto_boxed.rng_seed = 20260226ULL;
  const DfoResult r2 = DFO::solve(boxed, auto_boxed);
  std::cerr << "auto-boxed solver=" << to_string(r2.selected_solver)
            << " f=" << r2.minimum
            << " reason=" << r2.selection_reason << '\n';
  expect(r2.selected_solver == DfoSolverKind::DifferentialEvolution,
         "Auto selection did not choose DifferentialEvolution for boxed problem",
         &failures);
  expect(std::isfinite(r2.minimum), "Boxed auto run returned non-finite objective", &failures);
  const Series xb = r2.minimizer.to(pCPU, ScalarType::Float64);
  const double min_x = as_f64_scalar(xb.amin(0, false));
  const double max_x = as_f64_scalar(xb.amax(0, false));
  expect(min_x >= -1.000001 && max_x <= 1.000001, "Boxed auto run violated bounds", &failures);

  // Auto: constraints provided -> PatternSearch.
  DfoProblem constrained = unconstrained;
  constrained.constraints = sum_le_one_constraint;
  constrained.lower_bounds = pluss::series::full({4}, -2.0, cfg);
  constrained.upper_bounds = pluss::series::full({4}, 2.0, cfg);

  DfoOptions auto_constrained;
  auto_constrained.max_iterations = 500;
  auto_constrained.max_evaluations = 3000;
  auto_constrained.max_total_evaluations = 5000;
  auto_constrained.check_every = 10;
  auto_constrained.rng_seed = 20260227ULL;
  auto_constrained.mads.constraint_handling = ConstraintHandling::ProgressiveBarrier;
  const DfoResult r3 = DFO::solve(constrained, auto_constrained);
  std::cerr << "auto-constrained solver=" << to_string(r3.selected_solver)
            << " f=" << r3.minimum
            << " reason=" << r3.selection_reason << '\n';
  expect(r3.selected_solver == DfoSolverKind::PatternSearch,
         "Auto selection did not choose PatternSearch for constrained problem",
         &failures);
  expect(std::isfinite(r3.minimum), "Constrained auto run returned non-finite objective", &failures);

  // Explicit pattern-search route should be honored and handle constraints callback.
  DfoOptions forced_ps = auto_constrained;
  forced_ps.solver = DfoSolverKind::PatternSearch;
  forced_ps.max_iterations = 500;
  forced_ps.max_evaluations = 8000;
  forced_ps.pattern_search.constraint_policy = PSConstraintPolicy::MeritPenalty;
  forced_ps.pattern_search.polling_order = PSPollingOrder::SuccessFirst;
  forced_ps.pattern_search.direction_family = PSDirectionFamily::GSSAugmented;
  forced_ps.pattern_search.basis_family = PSBasisFamily::RandomOrtho;
  forced_ps.pattern_search.basis_refresh_interval = 2;
  const DfoResult rps = DFO::solve(constrained, forced_ps);
  std::cerr << "forced solver=" << to_string(rps.selected_solver)
            << " f=" << rps.minimum
            << " reason=" << rps.selection_reason << '\n';
  expect(rps.selected_solver == DfoSolverKind::PatternSearch,
         "Explicit PatternSearch override was not honored",
         &failures);
  expect(std::isfinite(rps.minimum), "PatternSearch forced solver returned non-finite objective", &failures);
  expect(rps.minimum < 1e-4, "PatternSearch forced solver failed sphere target", &failures);

  // Equality-aware adapter via constraints_values with AL policy.
  DfoProblem constrained_values = unconstrained;
  constrained_values.initial = pluss::series::full({4}, 1.5, cfg);
  constrained_values.lower_bounds = pluss::series::full({4}, -2.0, cfg);
  constrained_values.upper_bounds = pluss::series::full({4}, 2.0, cfg);
  constrained_values.constraints_values = [](const Series& x) {
    DfoConstraintValues c;
    c.inequality = x.square().sum().sub(1.0).reshape({1});
    c.equality = x.sum().sub(1.0).reshape({1});
    return c;
  };

  DfoOptions forced_ps_eq = auto_constrained;
  forced_ps_eq.solver = DfoSolverKind::PatternSearch;
  forced_ps_eq.max_iterations = 600;
  forced_ps_eq.max_evaluations = 12000;
  forced_ps_eq.pattern_search.constraint_policy = PSConstraintPolicy::AugmentedLagrangian;
  forced_ps_eq.pattern_search.al_max_outer_iterations = 6;
  forced_ps_eq.pattern_search.al_penalty_growth = 4.0;
  forced_ps_eq.pattern_search.al_violation_reduction_ratio = 0.6;
  const DfoResult rps_eq = DFO::solve(constrained_values, forced_ps_eq);
  expect(rps_eq.selected_solver == DfoSolverKind::PatternSearch,
         "constraints_values did not route through PatternSearch",
         &failures);
  const Series xeq = rps_eq.minimizer.to(pCPU, ScalarType::Float64);
  const double sum_x = as_f64_scalar(xeq.sum());
  const double norm2 = as_f64_scalar(xeq.square().sum());
  expect(std::abs(sum_x - 1.0) <= 5e-3,
         "constraints_values equality not satisfied by PatternSearch",
         &failures);
  expect(norm2 <= 1.0005,
         "constraints_values inequality not satisfied by PatternSearch",
         &failures);

  // Explicit override should be honored.
  DfoOptions forced_pso = auto_small;
  forced_pso.solver = DfoSolverKind::ParticleSwarm;
  forced_pso.max_iterations = 300;
  forced_pso.max_evaluations = 10000;
  forced_pso.rng_seed = 20260228ULL;
  const DfoResult r4 = DFO::solve(unconstrained, forced_pso);
  std::cerr << "forced solver=" << to_string(r4.selected_solver)
            << " f=" << r4.minimum
            << " reason=" << r4.selection_reason << '\n';
  expect(r4.selected_solver == DfoSolverKind::ParticleSwarm,
         "Explicit solver override was not honored",
         &failures);
  expect(std::isfinite(r4.minimum), "Forced solver returned non-finite objective", &failures);

  // Fluent builder: should produce same routing and comparable objective.
  const DfoResult rb = DFO::build()
                           .with_initial(x0)
                           .with_objective(sphere)
                           .with_bounds(pluss::series::full({4}, -1.0, cfg), pluss::series::full({4}, 1.0, cfg))
                           .with_max_iterations(250)
                           .with_max_evaluations(10000)
                           .with_check_every(10)
                           .with_seed(20260226ULL)
                           .solve();
  std::cerr << "builder solver=" << to_string(rb.selected_solver)
            << " f=" << rb.minimum
            << " reason=" << rb.selection_reason << '\n';
  expect(rb.selected_solver == DfoSolverKind::DifferentialEvolution,
         "Builder path did not choose DifferentialEvolution for boxed problem",
         &failures);
  expect(std::abs(rb.minimum - r2.minimum) < 1e-10,
         "Builder result deviates from equivalent direct solve",
         &failures);

  // PatternSearch status passthrough: infeasible stationary under extreme barrier.
  DfoProblem impossible = unconstrained;
  impossible.initial = pluss::make_series(std::vector<double>{0.0, 0.0}, {2}, cfg);
  impossible.constraints = [](const Series&) {
    return pluss::make_series(std::vector<double>{1.0}, {1});
  };

  DfoOptions ps_infeasible;
  ps_infeasible.solver = DfoSolverKind::PatternSearch;
  ps_infeasible.max_iterations = 120;
  ps_infeasible.max_evaluations = 500;
  ps_infeasible.pattern_search.initial_mesh_size = 0.5;
  ps_infeasible.pattern_search.mesh_tolerance = 1e-9;
  ps_infeasible.pattern_search.constraint_policy = PSConstraintPolicy::ExtremeBarrier;
  const DfoResult rinf = DFO::solve(impossible, ps_infeasible);
  expect(rinf.status == DfoStatus::InfeasibleStationary,
         "PatternSearch infeasible-stationary status was not propagated through DFO",
         &failures);
  expect(rinf.diagnostics.max_constraint_violation > 0.5,
         "PatternSearch infeasible diagnostics did not preserve max constraint violation",
         &failures);

  // PatternSearch status passthrough: time budget reached.
  DfoOptions ps_time;
  ps_time.solver = DfoSolverKind::PatternSearch;
  ps_time.max_iterations = 500;
  ps_time.max_evaluations = 10000;
  ps_time.pattern_search.time_limit_seconds = 1e-12;
  const DfoResult rtime = DFO::solve(unconstrained, ps_time);
  expect(rtime.status == DfoStatus::ReachedTimeBudget ||
             rtime.status == DfoStatus::ReachedEvaluationBudget,
         "PatternSearch time-budget status was not propagated through DFO",
         &failures);

  // PatternSearch diagnostics passthrough: random-search counters should surface in DFO diagnostics.
  DfoOptions ps_diag;
  ps_diag.solver = DfoSolverKind::PatternSearch;
  ps_diag.max_iterations = 350;
  ps_diag.max_evaluations = 9000;
  ps_diag.pattern_search.search_strategy = PSSearchStrategy::ExpandAndRandomDirections;
  ps_diag.pattern_search.search_trials = 12;
  ps_diag.pattern_search.search_complete = false;
  ps_diag.pattern_search.basis_family = PSBasisFamily::RandomOrtho;
  ps_diag.pattern_search.basis_refresh_interval = 2;
  const DfoResult rdiag = DFO::solve(unconstrained, ps_diag);
  expect(rdiag.diagnostics.search_trials > 0,
         "PatternSearch search trial diagnostics were not propagated through DFO",
         &failures);
  expect(rdiag.diagnostics.search_successes > 0,
         "PatternSearch search success diagnostics were not propagated through DFO",
         &failures);

  // Constraints through the facade must be respected and solved to the constrained optimum, not merely to a feasible point.
  // Three problems with known answers: a half-space (minimize |x|^2 with x0 + x1 >= 1: f = 0.5 at (0.5, 0.5)), an equality
  // (the same with x0 + x1 = 1) and a disc (the point of the unit disc nearest to (2, 1): f = (sqrt 5 - 1)^2 = 1.5279). A
  // fixed set of coordinate directions stops at (1, 0) or (0, 1) on the first, and a MADS route that ignores the constraint
  // returns the unconstrained minimum (0, 0), which violates it.
  {
    const Config c64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pCPU);
    const auto sphere2 = [](const Series& x) { return x.square().sum(); };
    const auto towards = [&c64](const Series& x) {
      return x.sub(pluss::make_series(std::vector<double>{2.0, 1.0}, {2}, c64)).square().sum();
    };
    const auto half_space = [](const Series& x) { return x.sum().mul(-1.0).add(1.0).reshape({1}); };
    const auto disc = [](const Series& x) { return x.square().sum().sub(1.0).reshape({1}); };
    const Series start = pluss::make_series(std::vector<double>{2.0, 2.0}, {2}, c64);

    DfoProblem half;
    half.initial = start; half.objective = sphere2; half.constraints = half_space;
    DfoProblem equal;
    equal.initial = start; equal.objective = sphere2;
    equal.constraints_values = [](const Series& x) { DfoConstraintValues v; v.equality = x.sum().sub(1.0).reshape({1}); return v; };
    DfoProblem round;
    round.initial = start; round.objective = towards; round.constraints = disc;

    DfoOptions options;
    options.max_iterations = 4000;
    options.max_evaluations = 40000;
    const auto near = [](double value, double target, double tolerance) { return std::abs(value - target) <= tolerance; };

    const auto a = DFO::solve(half, options);
    expect(a.selected_solver == DfoSolverKind::PatternSearch && near(a.minimum, 0.5, 1e-4) && a.status == DfoStatus::Converged,
           "constrained auto route reaches the half-space optimum", &failures);
    const auto b = DFO::solve(equal, options);
    expect(near(b.minimum, 0.5, 1e-4) && b.status == DfoStatus::Converged, "equality constraint is solved to its optimum", &failures);
    const auto d = DFO::solve(round, options);
    expect(near(d.minimum, 1.5279, 1e-3) && d.status == DfoStatus::Converged, "curved constraint is solved to its optimum", &failures);

    for (const auto solver : {DfoSolverKind::OrthoMads, DfoSolverKind::LtMads, DfoSolverKind::MadsAdvanced}) {
      DfoOptions mads = options;
      mads.solver = solver;
      const auto r = DFO::solve(half, mads);
      const auto x = r.minimizer.host<double>();
      expect(x[0] + x[1] >= 1.0 - 1e-9, "MADS route respects a constraint that was supplied", &failures);
      expect(r.minimum < 0.53, "MADS route is within a few percent of the constrained optimum", &failures);
    }
  }

  {
    // Nelder-Mead used to ignore bounds entirely. The unconstrained minimum (3, -1) lies outside the box [0,2]^2.
    DfoProblem boxed;
    boxed.initial = pluss::make_series(std::vector<double>{1.0, 1.0}, {2}, Config{}.with_dtype(ScalarType::Float64).with_platform(pCPU));
    std::int64_t outside = 0;
    boxed.objective = [&outside](const Series& x) {
      const auto v = x.host<double>();
      if (v[0] < 0.0 || v[0] > 2.0 || v[1] < 0.0 || v[1] > 2.0) ++outside;
      return x.sub(pluss::make_series(std::vector<double>{3.0, -1.0}, {2}, Config{}.with_dtype(ScalarType::Float64).with_platform(pCPU))).square().sum();
    };
    boxed.lower_bounds = pluss::make_series(std::vector<double>{0.0, 0.0}, {2}, Config{}.with_dtype(ScalarType::Float64).with_platform(pCPU));
    boxed.upper_bounds = pluss::make_series(std::vector<double>{2.0, 2.0}, {2}, Config{}.with_dtype(ScalarType::Float64).with_platform(pCPU));
    DfoOptions options;
    options.solver = DfoSolverKind::NelderMead;
    const auto r = DFO::solve(boxed, options);
    const auto x = r.minimizer.host<double>();
    expect(outside == 0, "Nelder-Mead never evaluates outside the bounds", &failures);
    expect(std::abs(x[0] - 2.0) < 1e-3 && std::abs(x[1]) < 1e-3 && std::abs(r.minimum - 2.0) < 1e-3,
           "Nelder-Mead returns the boxed optimum (2, 0)", &failures);
  }

  {
    // An infinite objective value means "this point is not allowed". Nelder-Mead must treat it as a very bad value
    // (the simplex moves away) and not throw. Minimise (a+1)^2 + (b-2)^2 over a >= 0: optimum (0, 2), value 1.
    DfoProblem walled;
    const Config c64 = Config{}.with_dtype(ScalarType::Float64).with_platform(pCPU);
    walled.initial = pluss::make_series(std::vector<double>{3.0, 3.0}, {2}, c64);
    walled.objective = [c64](const Series& x) {
      const auto v = x.host<double>();
      const double value = v[0] < 0.0 ? std::numeric_limits<double>::infinity() : (v[0] + 1.0) * (v[0] + 1.0) + (v[1] - 2.0) * (v[1] - 2.0);
      return pluss::make_series(value).to(ScalarType::Float64);
    };
    DfoOptions options;
    options.solver = DfoSolverKind::NelderMead;
    const auto r = DFO::solve(walled, options);
    const auto x = r.minimizer.host<double>();
    expect(std::abs(r.minimum - 1.0) < 1e-3 && x[0] >= 0.0 && x[0] < 1e-2 && std::abs(x[1] - 2.0) < 1e-2,
           "Nelder-Mead treats +Inf as a forbidden region and finds the wall optimum", &failures);
  }

  if (failures != 0) {
    std::cerr << "dfo_solver_test: " << failures << " failure(s)\n";
    return 1;
  }

  std::cout << "dfo_solver_test: all checks passed\n";
  return 0;
}
