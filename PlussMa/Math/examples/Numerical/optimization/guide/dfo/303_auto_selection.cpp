// What `Auto` picks. The choice depends on whether the problem is a black box, whether it has constraints, whether it
// has bounds, and how many variables it has. `selection_reason` says which rule fired. The rules are the same ones
// listed in the chapter; this program prints them for a spread of problems.
#include <pluss/ma.h>

#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
pluss::Series sphere(const pluss::Series& x) { return x.square().sum(); }

void run(const char* label, std::int64_t n, bool bounded, bool constrained, std::int64_t budget) {
  dfo::DfoProblem p;
  p.initial = filled(n, 1.5);
  p.objective = sphere;
  if (bounded) {
    p.lower_bounds = filled(n, -5.0);
    p.upper_bounds = filled(n, 5.0);
  }
  if (constrained) p.constraints = [](const pluss::Series& x) { return (x.sum() * -1.0 + 1.0).reshape({1}); };  // 1 - sum(x) <= 0
  dfo::DfoOptions o;
  o.max_evaluations = budget;
  const auto r = dfo::DFO::solve(p, o);
  std::cout << label << "\n    -> " << dfo::to_string(r.selected_solver) << ": " << r.selection_reason << "\n";
}

int main() {
  run("3 variables, no bounds", 3, false, false, 3000);
  run("12 variables, no bounds", 12, false, false, 3000);
  run("50 variables, no bounds", 50, false, false, 3000);
  run("3 variables, bounds", 3, true, false, 3000);
  run("50 variables, bounds", 50, true, false, 3000);
  run("3 variables, one constraint", 3, false, true, 3000);
}
