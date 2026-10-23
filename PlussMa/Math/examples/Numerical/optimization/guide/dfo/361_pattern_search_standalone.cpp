// Pattern search called directly. `pattern_search_minimizer(objective, initial, config)` returns a PSOutcome:
// minimiser, minimum, status and diagnostics (iterations, evaluations, cache hits, final mesh size, search trials and
// successes, constraint violation, the last kind of step taken). Here: the Rosenbrock function from (-1.2, 1) with no
// bounds and no constraints, all settings at their defaults.
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
  const auto o = dfo::pattern_search_minimizer(rosenbrock, vec({-1.2, 1.0}));
  std::cout << std::fixed << std::setprecision(5) << "minimizer     : " << o.minimizer.select(0, 0).item<double>() << ", "
            << o.minimizer.select(0, 1).item<double>() << "\n";
  std::cout << std::scientific << std::setprecision(2) << "minimum       : " << o.minimum << "\n";
  std::cout << "status        : " << dfo::to_string(o.status) << "\n";
  const auto& d = o.diagnostics;
  std::cout << "iterations    : " << d.iterations << "\nevaluations   : " << d.evaluations << "\ncache hits    : " << d.cache_hits
            << "\nsearch trials : " << d.search_trials << " (" << d.search_successes << " successful)\nfinal mesh    : " << d.mesh_size
            << "\nlast step     : " << d.last_step << "\n";
}
