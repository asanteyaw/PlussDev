// Budgets, the cache and the time limit. `max_iterations`, `max_evaluations` and `time_limit_seconds` end the run with
// the matching status. The cache (on by default) answers repeated points from memory; `cache_hits` counts them and
// `cache_tolerance` is how close two points must be to count as the same. Repeats occur when the poll axes do not
// change: the fixed coordinate basis below revisits points after each mesh refinement, the default rotating basis never does. A non-deterministic objective should run with
// the cache off. (The time limit is shown with an unreachable value; a real one makes the run machine-dependent.)
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}

int main() {
  std::cout << std::left << std::setw(30) << "setting" << std::setw(26) << "status" << std::setw(8) << "evals" << std::setw(8) << "iters" << "cache hits\n";
  auto show = [](const char* label, dfo::PSConfig c) {
    const auto o = dfo::pattern_search_minimizer(rosenbrock, vec({-1.2, 1.0}), c);
    std::cout << std::left << std::setw(30) << label << std::setw(26) << dfo::to_string(o.status) << std::setw(8) << o.diagnostics.evaluations
              << std::setw(8) << o.diagnostics.iterations << o.diagnostics.cache_hits << "\n";
  };
  dfo::PSConfig base;
  show("defaults (rotating basis)", base);
  dfo::PSConfig fixed_axes = base;
  fixed_axes.basis_family = dfo::PSBasisFamily::Canonical;
  show("coordinate basis, cache on", fixed_axes);
  fixed_axes.enable_cache = false;
  show("coordinate basis, cache off", fixed_axes);
  auto ev = base; ev.max_evaluations = 200;
  show("max_evaluations 200", ev);
  auto it = base; it.max_iterations = 50;
  show("max_iterations 50", it);
  auto tl = base; tl.time_limit_seconds = 1e9;
  show("time_limit_seconds 1e9", tl);
}
