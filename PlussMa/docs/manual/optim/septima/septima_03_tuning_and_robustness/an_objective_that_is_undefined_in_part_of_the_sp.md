<!-- generated from an_objective_that_is_undefined_in_part_of_the_sp.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# An objective that is undefined in part of the space

[Back to Tuning and Robustness](../septima_03_tuning_and_robustness.md)


`f(x) = -x - 0.2 log(1 - x)` is defined only for `x < 1` and has its minimum at `x = 0.8`. With a large learning rate the first Adam step from 0 jumps past 1, where the logarithm is NaN.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;
using optim::Trace;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  auto objective = [](const Trace& x) -> Trace { return optim::sum(x * -1.0) - 0.2 * optim::sum(optim::log(1.0 - x)); };

  std::cout << std::left << std::setw(42) << "setup" << std::setw(22) << "exit" << std::setw(8) << "iters" << "x\n";
  const auto run = [&](const char* label, std::int64_t recovery_attempts, bool bounded) {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 800;
    options.gradient_tolerance = 1e-5;
    options.step_tolerance = 1e-5;
    options.step.adam.learning_rate = 1.5;
    options.step.nan_recovery_attempts = recovery_attempts;
    optim::SeptimaConstraints constraints;
    if (bounded) constraints.ubs = {vec({0.99})};
    const auto r = optim::septima(objective, vec({0.0}), options, constraints);
    const char* exit = r.exit_flag == optim::SeptimaExitFlag::Converged ? "Converged"
                       : r.exit_flag == optim::SeptimaExitFlag::NonFiniteEvaluation ? "NonFiniteEvaluation" : "MaxIterations";
    std::cout << std::left << std::setw(42) << label << std::setw(22) << exit << std::setw(8) << r.iteration << std::fixed << std::setprecision(4)
              << r.params[0].item<double>() << "\n";
  };
  run("learning rate 1.5, no recovery", 0, false);
  run("learning rate 1.5, 6 recovery attempts", 6, false);
  run("learning rate 1.5, bound x <= 0.99", 2, true);
  return 0;
}
```

Output

```text
setup                                     exit                  iters   x
learning rate 1.5, no recovery            NonFiniteEvaluation   2       1.5000
learning rate 1.5, 6 recovery attempts    NonFiniteEvaluation   2       1.5000
learning rate 1.5, bound x <= 0.99        Converged             422     0.8000
```
The first run ends after two iterations with `NonFiniteEvaluation`, at `x = 1.5`. The second asks for six recovery attempts and ends the same way. The settings `nan_recovery_attempts` and `nan_recovery_shrink` retry an update that itself produces non-finite parameters or optimizer state with a smaller step. They do not look at the objective at the new point, so they cannot rescue a step into a region where the objective is undefined. What does rescue it is a bound that keeps every iterate inside the domain: with `x <= 0.99` the same run converges to 0.8 in 422 iterations. A smaller learning rate would also avoid the jump here, but a bound makes the guarantee. The general rule is to **bound the parameters wherever the objective is undefined outside the bounds**, and to set the bound a little inside the edge of the domain.
