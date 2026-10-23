// Seeds. The population methods (differential evolution, particle swarm) and the randomised poll bases draw random
// numbers. The draws come from `rng_seed` (default 5489), so a run is repeatable: the same seed gives the same answer
// and the same evaluation count, a different seed gives a different path to the same neighbourhood.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }

int main() {
  const double pi = 3.14159265358979323846;
  auto rastrigin = [pi](const pluss::Series& x) { return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum(); };
  std::cout << std::left << std::setw(8) << "seed" << std::setw(26) << "status" << std::setw(14) << "minimum" << "evaluations\n";
  for (std::uint64_t seed : {1ULL, 2ULL, 1ULL, 5489ULL, 5489ULL}) {
    dfo::DfoProblem p;
    p.initial = filled(3, 3.0);
    p.lower_bounds = filled(3, -5.12);
    p.upper_bounds = filled(3, 5.12);
    p.objective = rastrigin;
    dfo::DfoOptions o;
    o.solver = dfo::DfoSolverKind::DifferentialEvolution;
    o.max_evaluations = 4000;
    o.rng_seed = seed;
    const auto r = dfo::DFO::solve(p, o);
    std::cout << std::left << std::setw(8) << seed << std::setw(26) << dfo::to_string(r.status) << std::scientific
              << std::setprecision(3) << std::setw(14) << r.minimum << std::defaultfloat << r.diagnostics.evaluations << "\n";
  }
}
