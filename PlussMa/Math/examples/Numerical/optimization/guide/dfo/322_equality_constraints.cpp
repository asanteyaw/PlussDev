// Equality constraints go through `constraints_values`, which returns both kinds at once: `inequality` (<= 0) and
// `equality` (= 0). Minimise x0^2 + x1^2 + x2^2 on the plane x0 + x1 + x2 = 3, whose nearest point to the origin is
// (1, 1, 1) with objective 3. Only the pattern-search solver supports equalities, and it is what Auto chooses here.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  dfo::DfoProblem p;
  p.initial = vec({3.0, 0.0, 0.0});
  p.objective = [](const pluss::Series& x) { return x.square().sum(); };
  p.constraints_values = [](const pluss::Series& x) {
    dfo::DfoConstraintValues c;
    c.equality = (x.sum() - 3.0).reshape({1});
    return c;
  };
  const auto r = dfo::DFO::solve(p);
  std::cout << "solver    : " << dfo::to_string(r.selected_solver) << "\n";
  std::cout << "status    : " << dfo::to_string(r.status) << "\n";
  std::cout << std::fixed << std::setprecision(4) << "minimizer : " << r.minimizer.select(0, 0).item<double>() << ", "
            << r.minimizer.select(0, 1).item<double>() << ", " << r.minimizer.select(0, 2).item<double>() << "\n";
  std::cout << "objective : " << r.minimum << "  (exact 3)\n";
  std::cout << std::scientific << std::setprecision(1) << "violation : " << r.diagnostics.max_constraint_violation << "\n";
  std::cout << "evals     : " << r.diagnostics.evaluations << "\n";
}
