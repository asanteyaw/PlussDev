<!-- generated from the_smallest_problem_and_why_the_defaults_are_no.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The smallest problem, and why the defaults are not enough

[Back to First Problems](../septima_01_first_problems.md)


Minimize `sum((x - 3)^2)` over two parameters, started from the origin. The objective is a lambda on a `Trace`; `optim::sum` and `optim::square` are the traced versions of the familiar operations. The same problem is solved four ways.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;
using optim::Trace;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series start() { return pluss::make_series(std::vector<double>{0.0, 0.0}, {2}).to(f64); }

const char* name_of(optim::SeptimaExitFlag flag) {
  switch (flag) {
    case optim::SeptimaExitFlag::Converged: return "Converged";
    case optim::SeptimaExitFlag::MaxIterations: return "MaxIterations";
    case optim::SeptimaExitFlag::Stopped: return "Stopped";
    case optim::SeptimaExitFlag::NoProgress: return "NoProgress";
    case optim::SeptimaExitFlag::NonFiniteEvaluation: return "NonFiniteEvaluation";
    case optim::SeptimaExitFlag::InvalidProblem: return "InvalidProblem";
  }
  return "?";
}

int main() {
  auto objective = [](const Trace& x) -> Trace { return optim::sum(optim::square(x - 3.0)); };

  struct Row { const char* label; optim::SeptimaSolveOptions options; };
  std::vector<Row> rows(4);
  rows[0] = {"Adam, defaults", {}};
  rows[1] = {"Adam, defaults, 5000 iterations", {}};
  rows[1].options.max_iterations = 5000;
  rows[2] = {"Adam, learning rate 0.1", {}};
  rows[2].options.step.adam.learning_rate = 0.1;
  rows[3] = {"L-BFGS, defaults", {}};
  rows[3].options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;

  std::cout << std::left << std::setw(34) << "setup" << std::setw(16) << "exit" << std::setw(8) << "iters" << std::setw(14)
            << "objective" << "largest gradient\n";
  for (const auto& row : rows) {
    const auto r = optim::septima(objective, start(), row.options);
    std::cout << std::left << std::setw(34) << row.label << std::setw(16) << name_of(r.exit_flag) << std::setw(8) << r.iteration
              << std::scientific << std::setprecision(2) << std::setw(14) << r.objective_value << r.gradient_inf_norm << "\n";
  }
  return 0;
}
```

Output

```text
setup                             exit            iters   objective     largest gradient
Adam, defaults                    MaxIterations   200     1.57e+01      5.61e+00
Adam, defaults, 5000 iterations   MaxIterations   5000    7.78e-03      1.25e-01
Adam, learning rate 0.1           MaxIterations   200     8.93e-09      1.34e-04
L-BFGS, defaults                  Converged       3       0.00e+00      0.00e+00
```
Adam with its default settings uses all 200 iterations and stops with an objective of 15.7, far from the answer of 0: its default learning rate is 0.001, so each step moves a parameter by about a thousandth, and 200 steps cannot cross a distance of 3. Given 5000 iterations it reaches 7.8e-3. With a learning rate of 0.1 it gets to 8.9e-9 within the same 200 iterations, but the exit flag is still `MaxIterations`: a run counts as converged only when the largest gradient component is at most `gradient_tolerance` (1e-6), the last step is at most `step_tolerance` (1e-10), and the constraint violation is at most `constraint_tolerance` (1e-6). The gradient here is 1.3e-4, so the run is a good answer that has not yet met its own tests. L-BFGS needs three iterations and reports `Converged`, because on a quadratic it finds the exact step once it has seen the curvature.

The lesson is about the learning rate. It sets the size of every Adam step, so too small wastes the whole budget and too large overshoots. Chapter 3 shows how to choose it.
