<!-- generated from what_stops_a_run.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# What stops a run

[Back to Tuning and Robustness](../septima_03_tuning_and_robustness.md)


Three gradient tolerances on the ten-variable chain with L-BFGS, and then a problem whose curvature differs by a factor of 10 000 between its two variables.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;
using optim::Trace;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  const int n = 10;
  std::vector<double> first, second, start;
  for (int i = 0; i + 1 < n; ++i) { first.push_back(i); second.push_back(i + 1); }
  for (int i = 0; i < n; ++i) start.push_back(i % 2 == 0 ? -1.2 : 1.0);
  const auto head = pluss::make_series(first, {n - 1}).to(pluss::ScalarType::Int64);
  const auto tail = pluss::make_series(second, {n - 1}).to(pluss::ScalarType::Int64);
  auto chain = [&](const Trace& x) -> Trace {
    const Trace a = pluss::subsp::math::diff::index_select(x, 0, head), b = pluss::subsp::math::diff::index_select(x, 0, tail);
    return optim::sum(100.0 * optim::square(b - optim::square(a)) + optim::square(1.0 - a));
  };

  std::cout << std::left << std::setw(20) << "gradient tolerance" << std::setw(14) << "exit" << std::setw(8) << "iters" << std::setw(12) << "gradient" << "objective\n";
  for (const double tolerance : {1e-3, 1e-6, 1e-9}) {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 2000;
    options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    options.gradient_tolerance = tolerance;
    options.step_tolerance = 1e-8;
    const auto r = optim::septima(chain, vec(start), options);
    std::cout << std::left << std::setw(20) << std::scientific << std::setprecision(0) << tolerance << std::setw(14)
              << (r.converged ? "Converged" : "NoProgress") << std::setw(8) << r.iteration << std::setprecision(1) << std::setw(12) << r.gradient_inf_norm
              << r.objective_value << "\n";
  }

  std::cout << "\nconditioning: 0.5 * (x0^2 + 10000 x1^2) from (1, 1)\n";
  auto stretched = [](const Trace& x) -> Trace { return optim::sum(optim::fixed(vec({1.0, 1e4})) * optim::square(x)) * 0.5; };
  struct Row { const char* label; bool lbfgs; double rate; };
  for (const Row row : {Row{"Adam, learning rate 0.01", false, 0.01}, Row{"Adam, learning rate 0.1", false, 0.1}, Row{"L-BFGS", true, 1.0}}) {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 5000;
    if (row.lbfgs) options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    else options.step.adam.learning_rate = row.rate;
    const auto r = optim::septima(stretched, vec({1.0, 1.0}), options);
    std::cout << "  " << std::left << std::setw(26) << row.label << (r.converged ? "Converged" : "not converged") << "  iterations " << std::setw(6) << r.iteration
              << std::scientific << std::setprecision(1) << r.objective_value << "\n";
  }
  return 0;
}
```

Output

```text
gradient tolerance  exit          iters   gradient    objective
1e-03               Converged     99      2.5e-04     3.3e-10
1e-06               NoProgress    124     2.5e-04     3.3e-10
1e-09               NoProgress    124     2.5e-04     3.3e-10

conditioning: 0.5 * (x0^2 + 10000 x1^2) from (1, 1)
  Adam, learning rate 0.01  Converged  iterations 531   2.6e-17
  Adam, learning rate 0.1   Converged  iterations 422   2.0e-17
  L-BFGS                    Converged  iterations 5     6.2e-33
```
With a gradient tolerance of 1e-3 the run converges in 99 iterations. With 1e-6 and 1e-9 it ends in 124 iterations with `NoProgress`, at the same gradient (2.5e-04) and the same objective (3.3e-10): this method, without a line search, cannot bring the gradient lower than that, so asking for more changes the flag and nothing else. **If a run ends with `NoProgress` and the gradient is within a small factor of what you need, the answer is usually the answer; loosen the tolerance to match what you can use, and the flag will say so.**

On the stretched quadratic, both Adam runs converge (in 531 and 422 iterations), and L-BFGS in 5. Adam divides each coordinate's step by the size of that coordinate's own gradients, so a stretch along the axes costs it little. L-BFGS learns the curvature itself and needs far fewer iterations.
