<!-- generated from the_l_bfgs_settings.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The L-BFGS settings

[Back to Tuning and Robustness](../septima_03_tuning_and_robustness.md)


A ten-variable chained Rosenbrock function, where every neighbouring pair of variables forms a valley.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;
using optim::Trace;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

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

  std::cout << std::left << std::setw(32) << "setup" << std::setw(14) << "exit" << std::setw(8) << "iters" << std::setw(8) << "evals" << "objective\n";
  for (const bool wolfe : {false, true}) {
    for (const std::int64_t history : {1, 3, 10, 30}) {
      optim::SeptimaSolveOptions options;
      options.max_iterations = 2000;
      options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
      options.step.lbfgs.history_size = history;
      if (wolfe) options.step.lbfgs.line_search = optim::SeptimaLbfgsLineSearch::StrongWolfe;
      const auto r = optim::septima(chain, vec(start), options);
      std::cout << std::left << std::setw(32) << ((wolfe ? "strong Wolfe, history " : "no line search, history ") + std::to_string(history))
                << std::setw(14) << name_of(r.exit_flag) << std::setw(8) << r.iteration << std::setw(8) << r.function_evaluations << std::scientific
                << std::setprecision(1) << r.objective_value << "\n";
    }
  }
  std::cout << "\nlearning rate, no line search, default history (20)\n";
  for (const double rate : {0.1, 0.5, 1.0, 2.0}) {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 2000;
    options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    options.step.lbfgs.learning_rate = rate;
    const auto r = optim::septima(chain, vec(start), options);
    std::cout << "  " << std::left << std::setw(8) << std::defaultfloat << rate << std::setw(22) << name_of(r.exit_flag) << "iterations " << std::setw(6) << r.iteration
              << std::scientific << std::setprecision(1) << r.objective_value << "\n";
  }
  return 0;
}
```

Output

```text
setup                           exit          iters   evals   objective
no line search, history 1       NoProgress    214     214     1.4e-08
no line search, history 3       NoProgress    155     155     6.7e-09
no line search, history 10      NoProgress    137     137     9.5e-10
no line search, history 30      NoProgress    144     144     6.3e-11
strong Wolfe, history 1         NoProgress    176     508     2.3e-09
strong Wolfe, history 3         NoProgress    97      259     7.2e-09
strong Wolfe, history 10        NoProgress    94      248     7.8e-11
strong Wolfe, history 30        NoProgress    89      231     1.0e-10

learning rate, no line search, default history (20)
  0.1     NoProgress            iterations 413   4.6e-10
  0.5     NoProgress            iterations 150   1.5e-10
  1       NoProgress            iterations 124   3.3e-10
  2       NonFiniteEvaluation   iterations 32    inf
```
The history size matters at the short end. With a history of 1 the method uses only the last step pair and needs 214 iterations; with 3 it needs 155, with 10 it needs 137, and beyond that there is no gain (144 at a history of 30). The default of 20 is comfortably in the flat part.

(With equality or inequality constraints, chapter 2, L-BFGS always protects each step with a backtracking test on the augmented function, whatever this setting says.) The line search buys fewer iterations and costs more evaluations. With a history of 10, no line search needs 137 iterations and 137 evaluations; the strong-Wolfe search needs 94 iterations but 248 evaluations. When an evaluation is expensive, the run that uses fewer evaluations is the one without the line search. The line search earns its cost when a full step is not safe: with constraints (chapter 2), or on an objective with sharp changes in curvature.

The learning rate is the length of the first trial step. Values from 0.1 to 1 all work, and 1 needs the fewest iterations (124), 0.1 the most (413); at 2 the steps become so long that the run reaches non-finite values and ends with `NonFiniteEvaluation` after 32 iterations. Without a line search, keep it at or below 1.

All the L-BFGS runs here end with `NoProgress` at an objective near 1e-10. As in chapter 1, that is the method reaching the limit of what it can do with these tolerances, at a very good point.
