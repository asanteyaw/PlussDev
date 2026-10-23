<!-- generated from a_curved_valley.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A curved valley

[Back to First Problems](../septima_01_first_problems.md)


The Rosenbrock function `(1 - x)^2 + 100 (y - x^2)^2` has its minimum at `(1, 1)` at the end of a long, curved, narrow valley, and is the standard test of an optimizer. Here the parameters are two named scalar blocks, `x` and `y`, looked up by name inside the objective. The table gives iterations, function evaluations (a line search spends several per iteration), and the objective and point reached.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;
using Pack = optim::ParameterPack<optim::Trace>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }

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
  std::function<optim::Trace(const Pack&)> rosenbrock = [](const Pack& p) {
    return 100.0 * optim::square(p.at("y") - optim::square(p.at("x"))) + optim::square(1.0 - p.at("x"));
  };
  const auto start = optim::pack_scalars({{"x", scalar(-1.2)}, {"y", scalar(1.0)}});

  struct Row { const char* label; optim::SeptimaSolveOptions options; };
  std::vector<Row> rows(3);
  rows[0] = {"Adam, learning rate 0.02", {}};
  rows[0].options.step.adam.learning_rate = 0.02;
  rows[1] = {"L-BFGS, no line search", {}};
  rows[1].options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  rows[2] = {"L-BFGS, strong Wolfe", {}};
  rows[2].options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  rows[2].options.step.lbfgs.line_search = optim::SeptimaLbfgsLineSearch::StrongWolfe;
  for (auto& row : rows) row.options.max_iterations = 3000;

  std::cout << std::left << std::setw(28) << "setup" << std::setw(14) << "exit" << std::setw(8) << "iters" << std::setw(8) << "evals"
            << std::setw(12) << "objective" << "x, y\n";
  for (const auto& row : rows) {
    const auto r = optim::septima(rosenbrock, start, row.options);
    std::cout << std::left << std::setw(28) << row.label << std::setw(14) << name_of(r.exit_flag) << std::setw(8) << r.iteration
              << std::setw(8) << r.function_evaluations << std::scientific << std::setprecision(1) << std::setw(12) << r.objective_value
              << std::fixed << std::setprecision(5) << r.params[0].item<double>() << ", " << r.params[1].item<double>() << "\n";
  }
  return 0;
}
```

Output

```text
setup                       exit          iters   evals   objective   x, y
Adam, learning rate 0.02    MaxIterations 3000    3000    2.9e-07     0.99947, 0.99893
L-BFGS, no line search      NoProgress    102     102     5.6e-11     1.00000, 0.99999
L-BFGS, strong Wolfe        NoProgress    696     2062    3.1e-10     0.99999, 0.99998
```
Adam with a learning rate of 0.02 uses all 3000 iterations and ends at an objective of 2.9e-07, at `(0.99947, 0.99893)`: close, and still creeping. L-BFGS without a line search reaches 5.6e-11 at `(1.00000, 0.99999)` in 102 iterations. With the strong-Wolfe line search it takes 696 iterations and 2062 evaluations, about three evaluations per iteration, and ends at 3.1e-10: on this problem the line search buys nothing, and chapter 3 says when it does.

Two things to notice. Both L-BFGS runs end with `NoProgress`, not `Converged`, at an objective of about 1e-10. That flag means the objective and the step stopped changing before all three tolerances were met; here the gradient was a few times 1e-4 against a tolerance of 1e-6. It is not a failure. When a run ends this way, look at the objective and the gradient it reports and decide whether the answer is good enough, instead of reading the flag alone. And the iteration counts differ by a factor of thirty, which is the usual gap between a first-order step and one that uses curvature.
