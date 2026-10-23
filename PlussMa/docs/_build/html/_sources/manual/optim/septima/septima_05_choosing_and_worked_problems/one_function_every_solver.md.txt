<!-- generated from one_function_every_solver.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# One function, every solver

[Back to Choosing a Solver, and Worked Problems](../septima_05_choosing_and_worked_problems.md)


The ten-variable chained Rosenbrock function, with no constraints, solved by Optima and by the three first-order methods.

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

  std::cout << std::left << std::setw(16) << "solver" << std::setw(16) << "exit" << std::setw(8) << "iters" << std::setw(8) << "evals" << std::setw(12) << "objective"
            << "gradient\n";
  const auto row = [](const char* label, const char* exit, std::int64_t iterations, std::int64_t evaluations, double value, double gradient) {
    std::cout << std::left << std::setw(16) << label << std::setw(16) << exit << std::setw(8) << iterations << std::setw(8) << evaluations << std::scientific
              << std::setprecision(1) << std::setw(12) << value << gradient << "\n";
  };

  {
    const auto r = optim::optima(chain, vec(start));
    row("Optima", r.exit_flag == optim::OptimaExitFlag::Converged ? "converged" : "other", r.iteration, r.function_evaluations, r.evaluation.value.item<double>(),
        r.output.first_order_optimality);
  }
  {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 5000;
    options.step.adam.learning_rate = 0.05;
    const auto r = optim::septima(chain, vec(start), options);
    row("Septima, Adam", r.converged ? "converged" : "MaxIterations", r.iteration, r.function_evaluations, r.objective_value, r.gradient_inf_norm);
  }
  {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 2000;
    options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    const auto r = optim::septima(chain, vec(start), options);
    row("Septima, L-BFGS", r.converged ? "converged" : "NoProgress", r.iteration, r.function_evaluations, r.objective_value, r.gradient_inf_norm);
  }
  {
    const auto r = optim::l_sr1_trust_region(chain, vec(start));
    row("L-SR1", r.converged ? "converged" : "not converged", r.iteration, r.function_evaluations, r.objective_value, r.gradient_norm);
  }
  return 0;
}
```

Output

```text
solver          exit            iters   evals   objective   gradient
Optima          converged       72      80      3.6e-09     1.3e-06
Septima, Adam   converged       4388    4388    2.1e-17     3.6e-09
Septima, L-BFGS NoProgress      124     124     3.3e-10     2.5e-04
L-SR1           converged       119     286     9.2e-20     1.1e-08
```
Optima converges in 72 iterations and 80 evaluations to an objective of 3.6e-09. L-SR1 converges in 119 iterations and 286 evaluations to 9.2e-20 with a gradient of 1.1e-08, a far more accurate answer, at more work. L-BFGS stops with `NoProgress` after 124 iterations at 3.3e-10 and a gradient of 2.5e-04. Adam converges after 4388 iterations to 2.1e-17.

Read the table by what you need. Fewest evaluations: Optima. Most accurate: L-SR1. Fewest evaluations per iteration and simplest: L-BFGS, if a gradient of 1e-4 is good enough. Adam: when nothing else works or when the gradient is noisy.
