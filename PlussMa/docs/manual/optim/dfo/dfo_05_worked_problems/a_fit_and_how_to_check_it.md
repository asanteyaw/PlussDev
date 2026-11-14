<!-- generated from a_fit_and_how_to_check_it.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A fitted curve, and how to check an answer

[Back to Derivative-Free Optimization 5: Worked Problems and Checking Answers](../dfo_05_worked_problems.md)

Two worked problems finish the first part of the derivative-free guide. The first is a curve fit, `y = a exp(-b t) + c`, with bounds that keep the parameters physical and the sum of squared residuals as the objective. It uses a **two-stage strategy** that is worth copying: a cheap local method (Nelder–Mead) gets close, then a different method (OrthoMADS) restarts from that answer and polishes it. Starting the second stage from the first stage's `minimizer` costs only the polish, and a second method with a different stopping rule goes further.

The second is a set of **four checks for an answer** from a method that cannot check a first-order condition: the status, feasibility recomputed by hand, independence (a different method from a different start should land on the same point), and local probes (step each coordinate a little in both directions; any feasible, in-bounds step that lowers the objective shows the answer is not a local minimum).

## Program 1: Fitting a curve

Fit `y = a exp(-b t) + c` to eight measurements. The objective is the sum of squared residuals, written as ordinary Series arithmetic, with bounds that keep the parameters physical.

The strategy is two stages. A cheap local method (`NelderMead`) gets close. A second, different method (`OrthoMads`) restarts from that answer and polishes it. Starting the second stage from the first stage's `minimizer` costs only the polish, and the second method's different stopping rule goes further than the first.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  const auto t = vec({0.0, 0.5, 1.0, 1.5, 2.0, 3.0, 4.0, 6.0});
  std::vector<double> yv;
  for (double ti : {0.0, 0.5, 1.0, 1.5, 2.0, 3.0, 4.0, 6.0}) yv.push_back(2.0 * std::exp(-0.7 * ti) + 0.5);
  const auto y = vec(yv);
  auto sse = [&](const pluss::Series& p) {
    const auto model = (t * (p.select(0, 1) * -1.0)).exp() * p.select(0, 0) + p.select(0, 2);
    return (model - y).square().sum();
  };

  dfo::DfoProblem prob;
  prob.initial = vec({1.0, 1.0, 0.0});
  prob.objective = sse;
  prob.lower_bounds = vec({0.0, 0.0, -1.0});
  prob.upper_bounds = vec({10.0, 5.0, 5.0});

  auto show = [](const char* label, const dfo::DfoResult& r) {
    const auto p = r.minimizer.host<double>();
    std::cout << std::left << std::setw(26) << label << std::setw(26) << dfo::to_string(r.status) << std::fixed << std::setprecision(4)
              << "a " << p[0] << "  b " << p[1] << "  c " << p[2] << std::scientific << std::setprecision(1) << "  sse " << r.minimum
              << std::defaultfloat << "  evals " << r.diagnostics.evaluations << "\n";
  };

  dfo::DfoOptions first;
  first.solver = dfo::DfoSolverKind::NelderMead;
  const auto r1 = dfo::DFO::solve(prob, first);
  show("1. Nelder-Mead", r1);

  prob.initial = r1.minimizer;
  dfo::DfoOptions polish;
  polish.solver = dfo::DfoSolverKind::OrthoMads;
  const auto r2 = dfo::DFO::solve(prob, polish);
  show("2. OrthoMads from (1)", r2);
  std::cout << "true values               a 2.0000  b 0.7000  c 0.5000\n";
}
```

Output

```text
1. Nelder-Mead            Converged                 a 1.9999  b 0.7000  c 0.5000  sse 2.5e-09  evals 253
2. OrthoMads from (1)     Converged                 a 2.0000  b 0.7000  c 0.5000  sse 1.1e-24  evals 614
true values               a 2.0000  b 0.7000  c 0.5000
```
Both stages end `Converged`. The first is accurate to about four digits, the second to machine precision. The fit is exact because the data contain no noise; with noisy data the final sum of squares would be the noise level, and the parameters would be wrong by about as much as the data allow, whatever the solver. If the starting point were far from this basin (many local minima), replace stage one by `DifferentialEvolutionPrmm` as in Chapter 2.

## Program 2: Verifying an answer

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
```

Output

```text
1. status      : Converged
2. violation   : 0.0000  (reported 0.0e+00)
3. second run  : (1.4000, 1.7000) vs (1.4000, 1.7000), objectives 0.8000 and 0.8000
4. improving coordinate steps: 0
```
Reading the output: the status is `Converged`. The point is feasible: the recomputed violation and the solver's own report agree on zero. The independent mesh run, from the opposite corner, ends at the same point with the same objective, 0.8000. No coordinate step improves the first answer. Two answers that agree from unrelated starts and methods are strong evidence; if they differed slightly, keep the lower one and tighten the stopping criteria for more digits.

## Decisions and pitfalls

- **Combine a fast method with a precise one.** Nelder–Mead to get close, a mesh method to polish: each does what it is good at.
- **Noise-free data give an exact fit; noisy data do not.** With noise the final sum of squares is the noise level and the parameters are wrong by about as much as the data allow, whatever the solver.
- **A derivative-free answer must be verified from outside.** Status, feasibility, an independent run and local probes, in order of cost.
- **Two different answers mean one has stopped early or there are several minima.** The better objective wins; investigate the other.
- **Local probes are a necessary test, not a sufficient one.** They show the point is a local minimum; only an independent global search addresses the global question.

Where next: The remaining chapters document each method on its own: [Nelder–Mead](../dfo_06_nelder_mead.md) first.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
