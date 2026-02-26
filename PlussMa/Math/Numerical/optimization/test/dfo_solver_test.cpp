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
  const Series x0 = pluss::full({4}, 2.0, cfg);

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
  boxed.lower_bounds = pluss::full({4}, -1.0, cfg);
  boxed.upper_bounds = pluss::full({4}, 1.0, cfg);

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

  // Auto: constraints provided -> OrthoMads.
  DfoProblem constrained = unconstrained;
  constrained.constraints = sum_le_one_constraint;
  constrained.lower_bounds = pluss::full({4}, -2.0, cfg);
  constrained.upper_bounds = pluss::full({4}, 2.0, cfg);

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
  expect(r3.selected_solver == DfoSolverKind::OrthoMads,
         "Auto selection did not choose OrthoMads for constrained problem",
         &failures);
  expect(std::isfinite(r3.minimum), "Constrained auto run returned non-finite objective", &failures);

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
                           .with_bounds(pluss::full({4}, -1.0, cfg), pluss::full({4}, 1.0, cfg))
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

  if (failures != 0) {
    std::cerr << "dfo_solver_test: " << failures << " failure(s)\n";
    return 1;
  }

  std::cout << "dfo_solver_test: all checks passed\n";
  return 0;
}
