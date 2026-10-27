// Where the population starts. With bounds, the default (`initial_radius` infinite) draws the initial population from the
// whole box and always includes the start point itself as one member. A finite radius draws it from start +- radius
// instead (cut to the bounds), a small cluster that differential evolution widens through the differences between its
// members. Without bounds the radius is the only thing that sets the region (infinite means +-0.5). Same problem as before (3-D
// Rastrigin, box [-5.12, 5.12]^3, 20000 evaluations), three seeds per setting so that a pattern, not one lucky run, is read.
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <limits>
#include <string>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
pluss::Series rastrigin(const pluss::Series& x) {
  const double pi = 3.14159265358979323846;
  return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum();
}

int main() {
  std::cout << std::left << std::setw(16) << "initial_radius" << "minimum for seeds 1, 2, 3\n";
  for (double radius : {0.5, 2.0, std::numeric_limits<double>::infinity()}) {
    std::cout << std::left << std::setw(16) << (std::isfinite(radius) ? std::to_string(radius).substr(0, 3) : std::string("whole box"));
    for (std::uint64_t seed : {1ULL, 2ULL, 3ULL}) {
      dfo::DifferentialEvolutionConfig c;
      c.lower_bounds = filled(3, -5.12);
      c.upper_bounds = filled(3, 5.12);
      c.max_evaluations = 20000;
      c.initial_radius = radius;  // infinity = the default: the whole box
      c.rng_seed = seed;
      const auto o = dfo::differential_evolution_minimizer(rastrigin, filled(3, 3.0), c);
      std::cout << std::scientific << std::setprecision(2) << o.minimum << "   ";
    }
    std::cout << "\n";
  }
}
