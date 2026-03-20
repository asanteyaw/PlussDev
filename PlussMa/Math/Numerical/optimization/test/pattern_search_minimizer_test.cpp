#include "../pattern_search_minimizer.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

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

} // namespace

int main() {
  int failures = 0;

  const Config cfg = Config{}.with_dtype(ScalarType::Float64).with_platform(pCPU);

  // Unconstrained sphere.
  {
    const Series x0 = pluss::make_series(std::vector<double>{3.0, -2.0, 1.5}, {3}, cfg);
    PSConfig ps;
    ps.max_iterations = 500;
    ps.max_evaluations = 8000;
    ps.initial_mesh_size = 0.5;
    ps.mesh_tolerance = 1e-8;
    ps.step_tolerance = 1e-8;
    ps.value_tolerance = 1e-8;
    ps.basis_family = PSBasisFamily::RandomOrtho;
    ps.basis_refresh_interval = 1;

    const auto out = pattern_search_minimizer(
        [](const Series& x) {
          return x.square().sum();
        },
        x0,
        ps);

    expect(std::isfinite(out.minimum), "unconstrained sphere returned non-finite objective", &failures);
    expect(out.minimum < 1e-6, "unconstrained sphere failed convergence target", &failures);
    expect(out.status == PSStatus::Converged ||
               out.status == PSStatus::ReachedEvaluationBudget,
           "unexpected unconstrained status", &failures);
  }

  // Linear inequality constrained quadratic: (x-1)^2 + (y-1)^2, x+y<=0.5.
  {
    const Series x0 = pluss::make_series(std::vector<double>{1.8, 1.2}, {2}, cfg);
    PSConfig ps;
    ps.max_iterations = 600;
    ps.max_evaluations = 12000;
    ps.initial_mesh_size = 0.5;
    ps.mesh_tolerance = 1e-8;
    ps.complete_poll = true;
    ps.constraint_policy = PSConstraintPolicy::MeritPenalty;
    ps.initial_penalty = 50.0;
    ps.penalty_growth = 8.0;
    ps.lower_bounds = pluss::full({2}, 0.0, cfg);
    ps.upper_bounds = pluss::full({2}, 2.0, cfg);
    ps.linear_ineq_matrix = pluss::make_series(std::vector<double>{1.0, 1.0}, {1, 2}, cfg);
    ps.linear_ineq_rhs = pluss::make_series(std::vector<double>{0.5}, {1}, cfg);

    const auto out = pattern_search_minimizer(
        [](const Series& x) {
          return x.sub(1.0).square().sum();
        },
        x0,
        ps);

    const Series x = out.minimizer.to(pCPU, ScalarType::Float64).reshape({2});
    const double xsum = x.select(0, 0).item<double>() + x.select(0, 1).item<double>();

    expect(xsum <= 0.5001, "linear inequality not satisfied", &failures);
    expect(out.diagnostics.max_constraint_violation <= 1e-3,
           "linear constrained solve returned high violation", &failures);
  }

  // Dynamic NUPS scaling on anisotropic bowl should update scales and converge.
  {
    const Series x0 = pluss::make_series(std::vector<double>{4.0, -2.0, 1.2}, {3}, cfg);
    PSConfig ps;
    ps.max_iterations = 700;
    ps.max_evaluations = 14000;
    ps.initial_mesh_size = 0.35;
    ps.mesh_tolerance = 1e-8;
    ps.step_tolerance = 1e-8;
    ps.value_tolerance = 1e-8;
    ps.mesh_scaling = PSMeshScaling::NupsDynamic;
    ps.nups_expansion = 1.8;
    ps.nups_contraction = 0.7;
    ps.nups_decay = 0.97;
    ps.nups_min_scale = 1e-4;
    ps.nups_max_scale = 1e5;
    ps.basis_family = PSBasisFamily::RandomOrtho;
    ps.basis_refresh_interval = 2;

    const auto out = pattern_search_minimizer(
        [](const Series& x) {
          const Series x0 = x.select(0, 0);
          const Series x1 = x.select(0, 1);
          const Series x2 = x.select(0, 2);
          return x0.square().mul(10000.0).add(x1.square().mul(10.0)).add(x2.square());
        },
        x0,
        ps);

    expect(std::isfinite(out.minimum), "NUPS anisotropic run returned non-finite objective", &failures);
    expect(out.minimum < 1e-5, "NUPS anisotropic run failed convergence target", &failures);
    expect(out.diagnostics.nups_updates > 0, "NUPS scaling mode did not record any scale updates", &failures);
  }

  // Random-direction search stage should contribute accepted improvements.
  {
    const Series x0 = pluss::make_series(std::vector<double>{2.8, -1.9, 1.4}, {3}, cfg);
    PSConfig ps;
    ps.max_iterations = 700;
    ps.max_evaluations = 18000;
    ps.initial_mesh_size = 0.3;
    ps.mesh_tolerance = 1e-8;
    ps.step_tolerance = 1e-8;
    ps.value_tolerance = 1e-8;
    ps.mesh_scaling = PSMeshScaling::RangeLog;
    ps.search_strategy = PSSearchStrategy::ExpandAndRandomDirections;
    ps.search_trials = 20;
    ps.search_complete = false;
    ps.search_expansion = 2.0;
    ps.rng_seed = 123456ULL;
    ps.basis_family = PSBasisFamily::RandomOrtho;
    ps.basis_refresh_interval = 2;

    const auto out = pattern_search_minimizer(
        [](const Series& x) {
          const Series x0 = x.select(0, 0).sub(0.5);
          const Series x1 = x.select(0, 1).add(0.2);
          const Series x2 = x.select(0, 2).sub(1.0);
          return x0.square().add(x1.square().mul(25.0)).add(x2.square().mul(5.0));
        },
        x0,
        ps);

    expect(std::isfinite(out.minimum), "random search run returned non-finite objective", &failures);
    expect(out.minimum < 1e-6, "random search run failed convergence target", &failures);
    expect(out.diagnostics.search_trials > 0, "random search did not evaluate any search trial", &failures);
    expect(out.diagnostics.search_successes > 0, "random search produced no accepted improvements", &failures);
  }

  // Nonlinear inequality constrained 1D: minimize (x-0.7)^2 s.t. x^2<=0.25.
  {
    const Series x0 = pluss::make_series(std::vector<double>{1.2}, {1}, cfg);
    PSConfig ps;
    ps.max_iterations = 300;
    ps.max_evaluations = 5000;
    ps.initial_mesh_size = 0.2;
    ps.mesh_tolerance = 1e-8;
    ps.constraint_policy = PSConstraintPolicy::AugmentedLagrangian;
    ps.initial_penalty = 20.0;
    ps.al_penalty_growth = 4.0;
    ps.al_max_outer_iterations = 5;
    ps.al_violation_reduction_ratio = 0.6;
    ps.lower_bounds = pluss::full({1}, -1.0, cfg);
    ps.upper_bounds = pluss::full({1}, 1.0, cfg);
    ps.nonlinear_constraints = [](const Series& x) {
      PSConstraintValues c;
      c.inequality = x.square().sub(0.25).reshape({1});
      return c;
    };

    const auto out = pattern_search_minimizer(
        [](const Series& x) {
          return x.sub(0.7).square().sum();
        },
        x0,
        ps);

    const double x = as_f64_scalar(out.minimizer.reshape({1}).select(0, 0));
    expect(std::abs(x) <= 0.5001, "nonlinear inequality not satisfied", &failures);
    expect(std::abs(x - 0.5) < 0.1, "nonlinear constrained optimum is off expected boundary", &failures);
    expect(out.diagnostics.outer_iterations >= 1, "AL outer loop did not run", &failures);
  }

  // Linear equality constrained 2D: minimize distance to (0.2,0.8) with x+y=1.
  {
    const Series x0 = pluss::make_series(std::vector<double>{1.7, -0.2}, {2}, cfg);
    PSConfig ps;
    ps.max_iterations = 500;
    ps.max_evaluations = 10000;
    ps.initial_mesh_size = 0.3;
    ps.mesh_tolerance = 1e-8;
    ps.step_tolerance = 1e-8;
    ps.value_tolerance = 1e-8;
    ps.constraint_policy = PSConstraintPolicy::MeritPenalty;
    ps.direction_family = PSDirectionFamily::ProjectedTangent;
    ps.basis_family = PSBasisFamily::RandomOrtho;
    ps.basis_refresh_interval = 2;
    ps.initial_penalty = 100.0;
    ps.penalty_growth = 4.0;
    ps.lower_bounds = pluss::full({2}, -1.0, cfg);
    ps.upper_bounds = pluss::full({2}, 2.0, cfg);
    ps.linear_eq_matrix = pluss::make_series(std::vector<double>{1.0, 1.0}, {1, 2}, cfg);
    ps.linear_eq_rhs = pluss::make_series(std::vector<double>{1.0}, {1}, cfg);

    const auto out = pattern_search_minimizer(
        [](const Series& x) {
          const pluss::Config tcfg =
              pluss::Config{}.with_dtype(x.scalar_type()).with_platform(x.platform());
          const Series target = pluss::make_series(std::vector<double>{0.2, 0.8}, {2}, tcfg);
          return x.sub(target).square().sum();
        },
        x0,
        ps);

    const Series x = out.minimizer.to(pCPU, ScalarType::Float64).reshape({2});
    const double eq_resid = std::abs(x.select(0, 0).item<double>() + x.select(0, 1).item<double>() - 1.0);
    expect(eq_resid <= 1e-4, "linear equality not satisfied", &failures);
    expect(out.minimum < 1e-4, "linear equality constrained target not reached", &failures);
  }

  // Mixed nonlinear eq+ineq with infeasible start under AL:
  // minimize distance to (0.3,0.7), s.t. x+y=1 and x^2+y^2<=1.
  {
    const Series x0 = pluss::make_series(std::vector<double>{1.8, 1.8}, {2}, cfg);
    PSConfig ps;
    ps.max_iterations = 450;
    ps.max_evaluations = 12000;
    ps.initial_mesh_size = 0.25;
    ps.mesh_tolerance = 1e-8;
    ps.step_tolerance = 1e-8;
    ps.value_tolerance = 1e-8;
    ps.constraint_policy = PSConstraintPolicy::AugmentedLagrangian;
    ps.initial_penalty = 20.0;
    ps.al_penalty_growth = 4.0;
    ps.al_max_outer_iterations = 6;
    ps.al_violation_reduction_ratio = 0.6;
    ps.direction_family = PSDirectionFamily::GSSAugmented;
    ps.basis_family = PSBasisFamily::RandomOrtho;
    ps.basis_refresh_interval = 2;
    ps.lower_bounds = pluss::full({2}, -2.0, cfg);
    ps.upper_bounds = pluss::full({2}, 2.0, cfg);
    ps.nonlinear_constraints = [](const Series& x) {
      PSConstraintValues c;
      c.inequality = x.square().sum().sub(1.0).reshape({1});
      c.equality = x.sum().sub(1.0).reshape({1});
      return c;
    };

    const auto out = pattern_search_minimizer(
        [](const Series& x) {
          const pluss::Config tcfg =
              pluss::Config{}.with_dtype(x.scalar_type()).with_platform(x.platform());
          const Series target = pluss::make_series(std::vector<double>{0.3, 0.7}, {2}, tcfg);
          return x.sub(target).square().sum();
        },
        x0,
        ps);

    const Series x = out.minimizer.to(pCPU, ScalarType::Float64).reshape({2});
    const double x0v = x.select(0, 0).item<double>();
    const double x1v = x.select(0, 1).item<double>();
    const double eq_resid = std::abs((x0v + x1v) - 1.0);
    const double ineq = x0v * x0v + x1v * x1v - 1.0;

    expect(eq_resid <= 5e-4, "AL mixed nonlinear equality not satisfied", &failures);
    expect(ineq <= 5e-4, "AL mixed nonlinear inequality not satisfied", &failures);
    expect(out.minimum < 1e-4, "AL mixed nonlinear target not reached", &failures);
    expect(out.diagnostics.outer_iterations >= 1, "AL mixed nonlinear outer loop did not run", &failures);
  }

  // AL should respect global evaluation budget across outer iterations.
  {
    const Series x0 = pluss::make_series(std::vector<double>{1.9, -1.6}, {2}, cfg);
    PSConfig ps;
    ps.max_iterations = 500;
    ps.max_evaluations = 40;
    ps.initial_mesh_size = 0.25;
    ps.mesh_tolerance = 1e-10;
    ps.step_tolerance = 1e-10;
    ps.value_tolerance = 1e-10;
    ps.constraint_policy = PSConstraintPolicy::AugmentedLagrangian;
    ps.initial_penalty = 30.0;
    ps.al_penalty_growth = 4.0;
    ps.al_max_outer_iterations = 6;
    ps.al_violation_reduction_ratio = 0.7;
    ps.lower_bounds = pluss::full({2}, -2.0, cfg);
    ps.upper_bounds = pluss::full({2}, 2.0, cfg);
    ps.nonlinear_constraints = [](const Series& x) {
      PSConstraintValues c;
      c.inequality = x.square().sum().sub(1.0).reshape({1});
      c.equality = x.sum().sub(1.0).reshape({1});
      return c;
    };

    const auto out = pattern_search_minimizer(
        [](const Series& x) {
          const Series x0 = x.select(0, 0);
          const Series x1 = x.select(0, 1);
          return x1.sub(x0.square()).square().mul(100.0).add(x0.sub(1.0).square());
        },
        x0,
        ps);

    expect(out.diagnostics.evaluations <= ps.max_evaluations,
           "AL consumed more evaluations than configured global budget", &failures);
    expect(out.status == PSStatus::ReachedEvaluationBudget ||
               out.status == PSStatus::Converged,
           "AL budget test returned unexpected status", &failures);
  }

  if (failures != 0) {
    std::cerr << "pattern_search_minimizer_test: " << failures << " failure(s)\n";
    return 1;
  }

  std::cout << "pattern_search_minimizer_test: all checks passed\n";
  return 0;
}
