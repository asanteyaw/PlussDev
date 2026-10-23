// When no point satisfies the constraints. The two constraints x0 >= 2 and x0 <= 1 contradict each other. The solver
// cannot reach feasibility; it returns the least-violating point it found and the violation tells you so. With the
// default budget the run stops on the iteration limit; given room to finish, it recognises the situation and reports
// InfeasibleStationary. Either way the check that matters is `max_constraint_violation`.
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  dfo::DfoProblem p;
  p.initial = vec({0.0, 0.0});
  p.objective = [](const pluss::Series& x) { return x.square().sum(); };
  p.constraints = [](const pluss::Series& x) {
    const auto x0 = x.select(0, 0);
    return (x0 * -1.0 + 2.0).reshape({1}).unite({(x0 - 1.0).reshape({1})}, 0);  // 2 - x0 <= 0 and x0 - 1 <= 0
  };
  dfo::DfoOptions roomy;
  roomy.max_iterations = 4000;
  roomy.max_evaluations = 40000;
  const std::pair<const char*, dfo::DfoOptions> runs[] = {{"default budget", dfo::DfoOptions{}}, {"roomy budget", roomy}};
  for (const auto& [label, options] : runs) {
    const auto r = dfo::DFO::solve(p, options);
    std::cout << std::left << std::setw(16) << label << std::setw(24) << dfo::to_string(r.status) << std::scientific
              << std::setprecision(1) << "violation " << r.diagnostics.max_constraint_violation << std::fixed << std::setprecision(3)
              << "   x0 " << r.minimizer.select(0, 0).item<double>() << "\n";
  }
}
