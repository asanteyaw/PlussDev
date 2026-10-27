// Trust, but check. A derivative-free answer carries no optimality certificate, so confirm it from outside:
// (1) the status is acceptable, (2) it is feasible, (3) a different method started somewhere else lands in the
// same place, (4) no small step in any coordinate improves it. This program applies all four to a constrained problem:
// minimise (x0-1)^2 + (x1-2.5)^2 subject to x0 - 2 x1 + 2 >= 0 and the box [0,4]^2 (the classic textbook example,
// optimum (1.4, 1.7) with objective 0.8).
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

double objective_value(const std::vector<double>& x) { return (x[0] - 1) * (x[0] - 1) + (x[1] - 2.5) * (x[1] - 2.5); }
double violation(const std::vector<double>& x) { return std::max(0.0, -(x[0] - 2 * x[1] + 2)); }

int main() {
  auto make = [](std::vector<double> start) {
    dfo::DfoProblem p;
    p.initial = vec(start);
    p.objective = [](const pluss::Series& x) { return (x - vec({1.0, 2.5})).square().sum(); };
    p.constraints = [](const pluss::Series& x) { return (x.select(0, 0) * -1.0 + x.select(0, 1) * 2.0 - 2.0).reshape({1}); };
    p.lower_bounds = vec({0.0, 0.0});
    p.upper_bounds = vec({4.0, 4.0});
    return p;
  };
  dfo::DfoOptions opt;
  opt.max_iterations = 4000;
  opt.max_evaluations = 40000;
  const auto a = dfo::DFO::solve(make({2.0, 0.0}), opt);
  dfo::DfoOptions other = opt;
  other.solver = dfo::DfoSolverKind::OrthoMads;
  const auto b = dfo::DFO::solve(make({0.5, 3.5}), other);

  const auto xa = a.minimizer.host<double>(), xb = b.minimizer.host<double>();
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "1. status      : " << dfo::to_string(a.status) << "\n";
  std::cout << "2. violation   : " << violation(xa) << std::scientific << std::setprecision(1) << "  (reported " << a.diagnostics.max_constraint_violation << ")\n";
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "3. second run  : (" << xb[0] << ", " << xb[1] << ") vs (" << xa[0] << ", " << xa[1] << "), objectives " << objective_value(xb)
            << " and " << objective_value(xa) << "\n";
  // 4. coordinate probes: step 1e-3 in each direction; a move that is feasible, inside the box and lower would refute the answer.
  int improving = 0;
  for (int i = 0; i < 2; ++i)
    for (double s : {-1e-3, 1e-3}) {
      auto z = xa;
      z[i] += s;
      const bool inside = z[0] >= 0 && z[0] <= 4 && z[1] >= 0 && z[1] <= 4;
      if (inside && violation(z) <= 1e-6 && objective_value(z) < objective_value(xa) - 1e-9) ++improving;
    }
  std::cout << "4. improving coordinate steps: " << improving << "\n";
}
