// The cache. A mesh method often asks for a point it has already evaluated, because new meshes revisit old points.
// By default the advanced solver remembers every evaluated point (rounded to `cache_round_digits` digits) and answers
// repeats from memory. This program counts the real calls of the objective with the cache on and off. The solver's own
// `evaluations` counter equals the real calls, and `cache_hits` counts the repeats answered from memory.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  std::cout << std::left << std::setw(10) << "cache" << std::setw(22) << "objective calls" << std::setw(14) << "cache hits"
            << std::setw(24) << "diagnostics.evaluations" << "objective\n";
  for (bool enabled : {true, false}) {
    std::int64_t calls = 0;
    dfo::DfoProblem p;
    p.initial = vec({3.0, -2.0});
    p.blackbox = [&](const pluss::Series& x) {
      ++calls;
      dfo::BlackboxEvaluation e;
      e.objectives = (x - vec({1.0, 0.5})).abs().sum().reshape({1});  // a kink: many poll points repeat near the optimum
      return e;
    };
    dfo::DfoOptions o;
    o.mads.enable_cache = enabled;
    const auto r = dfo::DFO::solve(p, o);
    std::cout << std::left << std::setw(10) << (enabled ? "on" : "off") << std::setw(22) << calls << std::setw(14)
              << r.diagnostics.cache_hits << std::setw(24) << r.diagnostics.evaluations << std::scientific << std::setprecision(2)
              << r.minimum << "\n";
  }
}
