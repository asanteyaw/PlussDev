<!-- generated from fitting_a_curve.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Fitting a curve

[Back to Worked Problems and Checking Answers](../dfo_05_worked_problems.md)


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
    const auto p = r.minimizer.to_vec<double>();
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
