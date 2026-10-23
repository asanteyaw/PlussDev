<!-- generated from an_equality_constraint.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# An equality constraint

[Back to Constraints](../septima_02_constraints.md)


Be as close as possible to `(2, 1)` while the coordinates sum to 2: minimize `0.5 |x - (2, 1)|^2` subject to `x0 + x1 - 2 = 0`. The answer is `(1.5, 0.5)`, and the multiplier is 0.5, because the objective's gradient there is `(-0.5, -0.5)`, which is 0.5 times the gradient of the constraint. The table runs both optimizers with the defaults at three starting penalties, and then with the multiplier step overridden. `updates` counts the multiplier updates and `penalty x` is the factor by which the run raised the penalty.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <sstream>

namespace optim = pluss::optim;
using optim::Trace;
using pluss::Series;

const auto f64 = pluss::ScalarType::Float64;
Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  const auto target = vec({2.0, 1.0});
  auto objective = [&](const Trace& x) -> Trace { return optim::sum(optim::square(x - optim::fixed(target))) * 0.5; };
  optim::SeptimaConstraints constraint;
  constraint.equalities = [](const std::vector<Series>& x) { return std::vector<Series>{x[0].sum() - 2.0}; };

  std::cout << std::left << std::setw(10) << "optimizer" << std::setw(9) << "penalty" << std::setw(11) << "dual step" << std::setw(15) << "exit" << std::setw(7) << "iters"
            << std::setw(9) << "updates" << std::setw(10) << "penalty x" << std::setw(12) << "violation" << std::setw(12) << "multiplier" << "x\n";
  struct Row { bool lbfgs; double penalty; double dual; };
  const Row rows[] = {{false, 1, 0}, {false, 10, 0}, {false, 100, 0}, {true, 1, 0}, {true, 10, 0}, {true, 100, 0}, {false, 10, 0.1}, {true, 10, 0.1}};
  for (const auto& row : rows) {
    optim::SeptimaSolveOptions options;
    options.max_iterations = 3000;
    options.gradient_tolerance = 1e-5;
    options.step_tolerance = 1e-5;
    options.step.adam.learning_rate = 0.1;
    if (row.lbfgs) options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
    options.step.equality_penalty = row.penalty;
    options.step.dual_learning_rate = row.dual;  // 0 = the library default, the penalty itself
    const auto r = optim::septima(objective, vec({0.0, 0.0}), options, constraint);
    std::ostringstream x, dual;
    x << std::fixed << std::setprecision(3) << r.params[0].select(0, 0).item<double>() << ", " << r.params[0].select(0, 1).item<double>();
    if (row.dual > 0.0) dual << row.dual; else dual << "default";
    std::cout << std::left << std::setw(10) << (row.lbfgs ? "L-BFGS" : "Adam") << std::setw(9) << std::defaultfloat << row.penalty << std::setw(11) << dual.str()
              << std::setw(15) << (r.converged ? "Converged" : "MaxIterations") << std::setw(7) << r.iteration << std::setw(9) << r.state.dual_updates << std::setw(10)
              << r.state.penalty_scale << std::scientific << std::setprecision(1) << std::setw(12) << r.constraint_violation << std::fixed << std::setprecision(3)
              << std::setw(12) << r.state.lambda_equalities[0].item<double>() << x.str() << "\n";
  }
  return 0;
}
```

Output

```text
optimizer penalty  dual step  exit           iters  updates  penalty x violation   multiplier  x
Adam      1        default    Converged      696    15       100       3.8e-07     0.500       1.500, 0.500
Adam      10       default    Converged      347    6        1         6.2e-07     0.500       1.500, 0.500
Adam      100      default    Converged      1392   27       1         5.8e-11     0.500       1.500, 0.500
L-BFGS    1        default    Converged      16     6        10        5.7e-07     0.500       1.500, 0.500
L-BFGS    10       default    Converged      15     5        1         2.4e-07     0.500       1.500, 0.500
L-BFGS    100      default    Converged      10     3        1         1.2e-07     0.500       1.500, 0.500
Adam      10       0.1        MaxIterations  3000   61       1e+03     4.8e-05     0.020       1.500, 0.500
L-BFGS    10       0.1        Converged      31     11       1e+05     4.9e-07     0.011       1.500, 0.500
```
With the defaults every row converges to `(1.500, 0.500)` with a multiplier of exactly 0.500, whatever the starting penalty. L-BFGS needs 16, 15 and 10 iterations for penalties of 1, 10 and 100; Adam needs 696, 347 and 1392, because each step is bounded by its learning rate. The penalty column shows the schedule at work: from a penalty of 1 the run raised it by a factor of 10 for L-BFGS and 100 for Adam, because the violation did not shrink fast enough; from 10 and 100 it left the penalty alone. A penalty that is too small is repaired, and one that is generous costs nothing in accuracy, only some iterations for Adam.

The last two rows override the multiplier step with 0.1, far below the penalty of 10. The schedule then compensates in the only way left: it inflates the penalty (by a factor of a thousand for Adam and a hundred thousand for L-BFGS) to force the violation down, and the multiplier, which now learns very slowly, ends at 0.020 and 0.011 instead of 0.5. The point is still right, but the multiplier is not. Leave `dual_learning_rate` at its default of 0, which means "use the penalty", unless you have a reason.
