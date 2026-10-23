// Stopping. A MADS run ends when the frame falls below `mesh_tolerance` (Converged), at `max_iterations` or at
// `max_evaluations`, or, every `check_every` iterations, when the best value has improved by less than
// `relative_improvement_tolerance` (relatively) over the window. The window test can stop a run that is still creeping
// along a narrow valley; set `check_every` to 0 to rely on the frame alone.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}
const char* name(dfo::BlackboxSolverStatus s) {
  return s == dfo::BlackboxSolverStatus::Converged ? "Converged"
         : s == dfo::BlackboxSolverStatus::ReachedEvaluationBudget ? "ReachedEvaluationBudget" : "ReachedIterationBudget";
}
void run(const char* label, dfo::MadsBlackboxConfig c) {
  const auto o = dfo::orthomads_minimizer(rosenbrock, vec({-1.2, 1.0}), c);
  std::cout << std::left << std::setw(36) << label << std::setw(26) << name(o.status) << std::scientific << std::setprecision(2) << std::setw(11)
            << o.minimum << std::defaultfloat << std::setw(8) << o.diagnostics.evaluations << "iterations " << o.diagnostics.iterations << "\n";
}

int main() {
  dfo::MadsBlackboxConfig base;
  base.max_iterations = 20000;
  base.max_evaluations = 100000;
  run("defaults (check_every 10, 1e-8)", base);
  auto off = base; off.check_every = 0;
  run("check_every 0 (frame test only)", off);
  auto loose = base; loose.relative_improvement_tolerance = 1e-4;
  run("relative tolerance 1e-4", loose);
  auto it = base; it.max_iterations = 100;
  run("max_iterations 100", it);
  auto ev = base; ev.max_evaluations = 300;
  run("max_evaluations 300", ev);
}
