<!-- generated from how_a_run_ends.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# How a run ends

[Back to First Problems](../septima_01_first_problems.md)


Each way a run can end, produced on purpose.

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
  auto quadratic = [](const Trace& x) -> Trace { return optim::sum(optim::square(x - 3.0)); };
  const auto row = [](const char* what, const optim::SeptimaResult& r) {
    std::cout << std::left << std::setw(40) << what << std::setw(21) << name_of(r.exit_flag) << "iterations " << std::setw(6) << r.iteration
              << r.message << "\n";
  };

  {
    optim::SeptimaSolveOptions o;
    o.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    row("L-BFGS on a quadratic", optim::septima(quadratic, vec({0.0, 0.0}), o));
  }
  {
    optim::SeptimaSolveOptions o;  // default Adam learning rate 0.001, default 200 iterations
    row("Adam with its default settings", optim::septima(quadratic, vec({0.0, 0.0}), o));
  }
  {
    optim::SeptimaSolveOptions o;
    o.step.adam.learning_rate = 0.1;
    o.progress_hook = [](const optim::SeptimaProgressSnapshot& s) { return s.iteration < 7; };  // true keeps going
    row("a hook that returns false at 7", optim::septima(quadratic, vec({0.0, 0.0}), o));
  }
  {
    optim::SeptimaSolveOptions o;  // a learning rate so small that nothing changes within the tolerances
    o.step.adam.learning_rate = 1e-18;
    o.max_no_progress_iterations = 5;
    row("a learning rate of 1e-18", optim::septima(quadratic, vec({0.0, 0.0}), o));
  }
  {
    // log of a negative number is NaN: a large first step leaves the domain.
    auto domain = [](const Trace& x) -> Trace { return optim::sum(optim::log(x)) * -1.0 + optim::sum(x * 0.1); };
    optim::SeptimaSolveOptions o;
    o.step.adam.learning_rate = 100.0;
    o.step.nan_recovery_attempts = 0;
    row("a step into log's undefined region", optim::septima(domain, vec({1.0}), o));
  }

  std::cout << "\nInvalid settings throw:\n";
  try {
    optim::SeptimaSolveOptions o;
    o.step.adam.learning_rate = -1.0;
    optim::septima(quadratic, vec({0.0}), o);
  } catch (const std::exception& e) {
    const std::string message = e.what();
    std::cout << "  " << message.substr(0, message.find('\n')) << "\n";
  }
  try {
    optim::SeptimaSolveOptions o;
    o.max_iterations = 0;
    optim::septima(quadratic, vec({0.0}), o);
  } catch (const std::exception& e) {
    const std::string message = e.what();
    std::cout << "  " << message.substr(0, message.find('\n')) << "\n";
  }
  return 0;
}
```

Output

```text
L-BFGS on a quadratic                   Converged            iterations 3     converged
Adam with its default settings          MaxIterations        iterations 200   max iterations reached
a hook that returns false at 7          Stopped              iterations 7     stopped by progress hook
a learning rate of 1e-18                NoProgress           iterations 6     no progress
a step into log's undefined region      NonFiniteEvaluation  iterations 20    objective became non-finite

Invalid settings throw:
  optim::septima_step_inplace: adam.learning_rate must be positive and finite
  optim::septima: max_iterations must be > 0
```
| Flag | Meaning | In the program |
|---|---|---|
| `Converged` | gradient, step and violation are all within tolerance | L-BFGS on a quadratic, in 3 iterations |
| `MaxIterations` | the budget ran out first | Adam with its defaults, 200 iterations |
| `Stopped` | the progress hook returned `false` | a hook that stops at iteration 7 |
| `NoProgress` | the objective and the step stopped changing before the tolerances were met | a learning rate of 1e-18, after 6 iterations |
| `NonFiniteEvaluation` | the objective or its gradient became NaN or infinite | a step of size 100 into the region where `log` is undefined |

`NoProgress` is declared after `max_no_progress_iterations` (25 by default; the program uses 5) consecutive iterations in which the objective changes by at most `objective_tolerance` and the step is at most `step_tolerance`. Invalid settings do not return a flag: the call throws an error that names the setting, as the last two lines show (a negative learning rate, and a budget of zero iterations).
