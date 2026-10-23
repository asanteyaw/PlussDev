// The search step and bounds. `search_points` extra trial points are tried before each poll; they are drawn on the mesh over
// the whole region, which lets the method jump across the region instead of creeping along the frame, and on a function with
// several minima gives it a chance to find a better basin. Bounds keep every trial point inside the box. Problem: the 2-D
// Rastrigin function in [-5.12, 5.12]^2 from the start (3.2, 2.7), whose nearest local minimum is far from the global one.
// Three seeds per setting.
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
int main() {
  const double pi = 3.14159265358979323846;
  auto rastrigin = [pi](const pluss::Series& x) { return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum(); };
  std::cout << std::left << std::setw(20) << "search_points" << "minimum for seeds 1, 2, 3 (evaluations in brackets)\n";
  for (std::int64_t points : {0, 8, 32}) {
    std::cout << std::left << std::setw(20) << points;
    for (std::uint64_t seed : {1ULL, 2ULL, 3ULL}) {
      dfo::MadsBlackboxConfig c;
      c.lower_bounds = filled(2, -5.12);
      c.upper_bounds = filled(2, 5.12);
      c.search_points = points;
      c.rng_seed = seed;
      c.max_iterations = 4000;
      c.max_evaluations = 20000;
      const auto o = dfo::orthomads_minimizer(rastrigin, vec({3.2, 2.7}), c);
      std::cout << std::scientific << std::setprecision(2) << o.minimum << " (" << std::defaultfloat << o.diagnostics.evaluations << ")   ";
    }
    std::cout << "\n";
  }
}
