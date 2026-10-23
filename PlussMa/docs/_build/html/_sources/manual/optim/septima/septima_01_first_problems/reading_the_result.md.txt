<!-- generated from reading_the_result.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Reading the result

[Back to First Problems](../septima_01_first_problems.md)


The problem is a bounded fit, `0.5 ((a - 2)^2 + (b - 1)^2)` with `a <= 1.5`, so the bound is active at the answer. The program prints each field of the result.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace optim = pluss::optim;
using optim::Trace;
using Pack = optim::ParameterPack<Trace>;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series scalar(double v) { return pluss::make_series(v).to(f64); }

int main() {
  std::function<Trace(const Pack&)> objective = [](const Pack& p) {
    return 0.5 * (optim::square(p.at("a") - 2.0) + optim::square(p.at("b") - 1.0));
  };
  optim::SeptimaConstraints constraints;
  constraints.ubs = {scalar(1.5), scalar(100.0)};  // one bound per block, in block order

  optim::SeptimaSolveOptions options;
  options.step.optimizer = optim::SeptimaOptimizer::Lbfgs;
  const auto r = optim::septima(objective, optim::pack_scalars({{"a", scalar(0.0)}, {"b", scalar(0.0)}}), options, constraints);

  std::cout << std::scientific << std::setprecision(2) << std::boolalpha;
  std::cout << "converged               " << r.converged << "\n";
  std::cout << "exit_flag               " << static_cast<int>(r.exit_flag) << "  (1 = Converged)\n";
  std::cout << "message                 " << r.message << "\n";
  std::cout << std::fixed << std::setprecision(4);
  std::cout << "params                  a = " << r.params[0].item<double>() << ", b = " << r.params[1].item<double>() << "\n";
  std::cout << "objective_value         " << r.objective_value << "\n";
  std::cout << std::scientific << std::setprecision(2);
  std::cout << "gradient_inf_norm       " << r.gradient_inf_norm << "  (the objective's gradient alone)\n";
  std::cout << "stationarity            " << r.stationarity << "  (with the active bound taken into account)\n";
  std::cout << "step_inf_norm           " << r.step_inf_norm << "  (size of the last step)\n";
  std::cout << "constraint_violation    " << r.constraint_violation << "\n";
  std::cout << "iteration               " << r.iteration << "\n";
  std::cout << "function_evaluations    " << r.function_evaluations << "\n";
  return 0;
}
```

Output

```text
converged               true
exit_flag               1  (1 = Converged)
message                 converged
params                  a = 1.5000, b = 1.0000
objective_value         0.1250
gradient_inf_norm       5.00e-01  (the objective's gradient alone)
stationarity            0.00e+00  (with the active bound taken into account)
step_inf_norm           0.00e+00  (size of the last step)
constraint_violation    0.00e+00
iteration               2
function_evaluations    2
```
In the order you will use them:

- `converged` and `exit_flag` say whether the run met its tests. `message` says why it stopped.
- `params` is the answer, one `Series` per block, in the order you gave them.
- `objective_value` is the objective at the answer.
- `gradient_inf_norm` is the largest component of the gradient of the **objective alone**. Here it is 0.5, not zero: at a constrained optimum the objective still pulls toward the bound.
- `stationarity` is the largest component of the gradient of the constrained problem, with the components that an active bound blocks removed. It is the number the solver compares with `gradient_tolerance`. Here it is 0, which is why the run converged although `gradient_inf_norm` is 0.5. Without constraints it equals `gradient_inf_norm`.
- `step_inf_norm` is the size of the last step; `constraint_violation` is the worst violation of a bound, an equality or an inequality.
- `iteration` and `function_evaluations` are the cost. Without a line search they are equal.
- `state` (not printed) holds the optimizer's memory and the constraint multipliers (chapter 2).
