// Variable-neighbourhood MADS (`vnsmads_blackbox_minimizer`) for functions with several basins. It runs MADS from the start, then
// repeatedly "kicks" the best point by a random perturbation (a normal draw of scale `initial_kick_scale`, multiplied by
// `kick_growth` each stage) and runs MADS again from there, keeping the best outcome. `neighborhoods` is the number of
// stages; the evaluation budget of `base` is divided among them. The kicks let it leave a basin that plain MADS would
// settle in. Problem: the 2-D Rastrigin function in [-5.12, 5.12]^2 from (3.2, 2.7); three seeds, 9000 evaluations in every row.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
int main() {
  const double pi = 3.14159265358979323846;
  auto blackbox = [pi](const pluss::Series& x) {
    dfo::BlackboxEvaluation e;
    e.objectives = (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum().reshape({1});
    return e;
  };
  std::cout << std::left << std::setw(26) << "neighborhoods" << "minimum for seeds 1, 2, 3\n";
  for (std::int64_t stages : {1, 3, 6, 12}) {
    std::cout << std::left << std::setw(26) << stages;
    for (std::uint64_t seed : {1ULL, 2ULL, 3ULL}) {
      dfo::VnsMadsConfig c;
      c.base.lower_bounds = filled(2, -5.12);
      c.base.upper_bounds = filled(2, 5.12);
      c.base.max_evaluations = 9000;
      c.base.max_iterations = 5000;
      c.base.rng_seed = seed;
      c.rng_seed = seed;
      c.neighborhoods = stages;
      c.initial_kick_scale = 1.0;
      const auto o = dfo::vnsmads_blackbox_minimizer(blackbox, vec({3.2, 2.7}), c);
      std::cout << std::scientific << std::setprecision(2) << o.scalarized_minimum << "   ";
    }
    std::cout << "\n";
  }
}
