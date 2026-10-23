// Inequality constraints. The constraint function returns a vector g(x); the point is feasible when every entry is
// <= 0. Here: minimise (x0-2)^2 + (x1-2)^2 inside the unit disc, x0^2 + x1^2 - 1 <= 0, and also x0 + x1 <= 3. The
// unconstrained minimum (2, 2) is outside the disc; the answer is the nearest point of the disc, (1/sqrt 2, 1/sqrt 2),
// where the objective equals 2 (2 - 1/sqrt 2)^2 = 3.3431. Supplying `constraints` makes Auto choose pattern search.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  dfo::DfoProblem p;
  p.initial = vec({0.0, 0.0});
  p.objective = [](const pluss::Series& x) { return (x - vec({2.0, 2.0})).square().sum(); };
  p.constraints = [](const pluss::Series& x) {
    const auto disc = (x.square().sum() - 1.0).reshape({1});
    const auto half = (x.sum() - 3.0).reshape({1});
    return disc.unite({half}, 0);
  };
  const auto r = dfo::DFO::solve(p);
  std::cout << "solver    : " << dfo::to_string(r.selected_solver) << " (" << r.selection_reason << ")\n";
  std::cout << "status    : " << dfo::to_string(r.status) << "\n";
  std::cout << std::fixed << std::setprecision(4) << "minimizer : " << r.minimizer.select(0, 0).item<double>() << ", "
            << r.minimizer.select(0, 1).item<double>() << "\n";
  std::cout << "objective : " << r.minimum << "  (exact 3.3431)\n";
  std::cout << std::scientific << std::setprecision(1) << "violation : " << r.diagnostics.max_constraint_violation << "\n";
  std::cout << "evals     : " << r.diagnostics.evaluations << "\n";
}
