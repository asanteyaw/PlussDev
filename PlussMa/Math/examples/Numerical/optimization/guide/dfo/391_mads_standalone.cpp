// Mesh adaptive direct search called directly. `mesh_adaptive_direct_search_minimizer(objective, initial, config)` returns
// a BlackboxSolverOutcome: minimiser, minimum, status, and diagnostics (iterations, evaluations, last phase, final mesh and
// frame size). Each iteration polls 2n points around the best point on a frame of size `frame`, laid on a finer mesh;
// a success enlarges the frame and mesh, a failure refines them, and the run converges when the frame is smaller than
// `mesh_tolerance`. Here: the Rosenbrock function from (-1.2, 1) with every setting at its default.
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
int main() {
  const auto o = dfo::mesh_adaptive_direct_search_minimizer(rosenbrock, vec({-1.2, 1.0}));
  std::cout << std::fixed << std::setprecision(5) << "minimizer   : " << o.minimizer.select(0, 0).item<double>() << ", " << o.minimizer.select(0, 1).item<double>() << "\n";
  std::cout << std::scientific << std::setprecision(2) << "minimum     : " << o.minimum << "\n";
  std::cout << "status      : " << dfo::to_string(o.status) << "\n";
  const auto& d = o.diagnostics;
  std::cout << "iterations  : " << d.iterations << "\nevaluations : " << d.evaluations << "\nfinal mesh  : " << d.mesh_size
            << "\nfinal frame : " << d.frame_size << "\nlast phase  : " << d.last_phase << "\n";
}
