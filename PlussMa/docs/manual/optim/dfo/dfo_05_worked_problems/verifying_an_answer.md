<!-- generated from verifying_an_answer.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Verifying an answer

[Back to Worked Problems and Checking Answers](../dfo_05_worked_problems.md)


A gradient method can check a first-order condition. A derivative-free one cannot, so check from the outside. Four checks, in order of cost:

1. **Status.** `Converged` is the target. A budget status is acceptable only if you then raise the budget and see the answer settle.
2. **Feasibility.** Recompute the constraints yourself at the returned point. This also guards against a mistake in how you wrote them.
3. **Independence.** Run a different method from a different start. If both land on the same point, a spurious stop is unlikely. If they differ, one of them has stopped early or the problem has several local minima, and the better objective wins.
4. **Local probes.** Move each coordinate a little in both directions. If any feasible, in-bounds step lowers the objective, the answer is not a local minimum.

```cpp
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

  const auto xa = a.minimizer.to_vec<double>(), xb = b.minimizer.to_vec<double>();
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
```

Output

```text
1. status      : Converged
2. violation   : 0.0000  (reported 0.0e+00)
3. second run  : (1.4622, 1.7311) vs (1.4000, 1.7000), objectives 0.8048 and 0.8000
4. improving coordinate steps: 0
```
Reading the output: the status is `Converged`. The point is feasible: the recomputed violation and the solver's own report agree on zero. The independent mesh run, from the opposite corner, ends near the same point but a little above the optimum (0.8048 against 0.8000), the boundary effect of Chapter 3: its answer is feasible and close, not exact. No coordinate step improves the first answer. Two answers within 0.01 of each other from unrelated starts and methods are strong evidence; keep the lower one, and tighten the stopping criteria if you need more digits.
