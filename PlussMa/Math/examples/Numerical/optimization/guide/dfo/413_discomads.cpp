// Disco MADS (`discomads_blackbox_minimizer`) for a black box that fails or returns non-finite values at many points (a
// simulation that does not always converge). A failed evaluation stays a failure for the search, as in plain MADS, but
// Disco also looks around each failure: up to `rescue_trials` nearby points (random offsets of radius `rescue_radius`,
// shrinking by `rescue_radius_decay` each trial) are evaluated, and any valid result is kept as a candidate answer, so valid
// information near failures is not thrown away. The rescue calls are extra calls of the black box. Problem: minimise
// (x0-2)^2 + (x1-1)^2 where a fixed fraction of points, chosen by a hash of the coordinates, fails (returns NaN). The minimum is 0.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
const char* name(dfo::BlackboxSolverStatus s) {
  return s == dfo::BlackboxSolverStatus::Converged ? "Converged"
         : s == dfo::BlackboxSolverStatus::ReachedEvaluationBudget ? "ReachedEvaluationBudget" : "ReachedIterationBudget";
}
int main() {
  std::cout << std::left << std::setw(10) << "failing" << std::setw(24) << "method" << std::setw(12) << "minimum" << std::setw(14) << "calls made" << "failed calls\n";
  for (double rate : {0.3, 0.8}) {
    std::int64_t calls = 0, failures = 0;
    auto blackbox = [&](const pluss::Series& x) {
      ++calls;
      dfo::BlackboxEvaluation e;
      const double a = x.select(0, 0).item<double>(), b = x.select(0, 1).item<double>();
      double h = std::sin(12345.678 * a + 9876.543 * b + 0.5) * 43758.5453;
      h -= std::floor(h);
      if (h < rate) {
        ++failures;
        e.objectives = pluss::make_series(std::nan("")).to(f64).reshape({1});
        return e;
      }
      e.objectives = (x - vec({2.0, 1.0})).square().sum().reshape({1});
      return e;
    };
    auto report = [&](const std::string& label, const dfo::BlackboxAdvancedOutcome& o) {
      std::cout << std::left << std::setw(10) << std::to_string(static_cast<int>(rate * 100)) + "%" << std::setw(24) << label << std::scientific << std::setprecision(2)
                << std::setw(12) << o.scalarized_minimum << std::defaultfloat << std::setw(14) << calls << failures << "\n";
    };
    {
      calls = failures = 0;
      dfo::MadsBlackboxAdvancedConfig c;
      c.max_evaluations = 3000;
      report("plain MADS", dfo::mads_blackbox_minimizer(blackbox, vec({0.0, 0.0}), c));
    }
    for (std::int64_t trials : {3, 12}) {
      calls = failures = 0;
      dfo::DiscoMadsConfig c;
      c.base.max_evaluations = 3000;
      c.rescue_trials = trials;
      report("Disco, rescue_trials " + std::to_string(trials), dfo::discomads_blackbox_minimizer(blackbox, vec({0.0, 0.0}), c));
    }
  }
}
