<!-- generated from every_field_of_the_report.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Every field of the report

[Back to Reading Results and Watching a Run](../optima_02_reading_results.md)


`result.output` is the structured report. This program prints each field of one solve so you can match it against the table below.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  using Pack = optim::ParameterPack<optim::Trace>;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const optim::Trace& x) { return optim::sum(optim::square(x - 3.0)); };
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return optim::sum(optim::square(p.at("x"))) - 1.0; });

  const auto r = optim::optima(objective, vec({0.0, 0.0}), {}, disc);
  const auto& o = r.output;

  std::cout << std::boolalpha << std::setprecision(4) << std::scientific;
  std::cout << "algorithm                             " << o.algorithm << "\n";
  std::cout << "iterations                            " << o.iterations << "\n";
  std::cout << "function_evaluations                  " << o.function_evaluations << "\n";
  std::cout << "constraint_violation                  " << o.constraint_violation << "\n";
  std::cout << "first_order_optimality                " << o.first_order_optimality << "\n";
  std::cout << "step_size                             " << o.step_size << "\n";
  std::cout << "line_search_fraction                  " << o.line_search_fraction << "\n";
  std::cout << "backend_parameter                     " << o.backend_parameter << "\n";
  std::cout << "complementarity                       " << o.complementarity << "\n";
  std::cout << "krylov_iterations                     " << o.krylov_iterations << "\n";
  std::cout << "best_feasible_available               " << o.best_feasible_available << "\n";
  std::cout << "returned_best_feasible                " << o.returned_best_feasible << "\n";
  std::cout << "best_feasible_value                   " << o.best_feasible_value << "\n";
  std::cout << "best_feasible_constraint_violation    " << o.best_feasible_constraint_violation << "\n";
  std::cout << "best_feasible_first_order_optimality  " << o.best_feasible_first_order_optimality << "\n";
  std::cout << "multiplier_max_norm                   " << o.multiplier_max_norm << "\n";
  std::cout << "active_multiplier_count               " << o.active_multiplier_count << "\n";
  std::cout << "detail_message                        " << o.detail_message << "\n";
  std::cout << "stall_category                        " << optim::optima_stall_category_name(o.stall_category) << "\n";
  std::cout << "stall_measure                         " << o.stall_measure << "\n";
  std::cout << "stall_reference                       " << o.stall_reference << "\n";
  return 0;
}
```

Output

```text
algorithm                             sqp
iterations                            4
function_evaluations                  9
constraint_violation                  0.0000e+00
first_order_optimality                1.1464e-08
step_size                             1.0000e+00
line_search_fraction                  1.0000e+00
backend_parameter                     3.5769e+00
complementarity                       0.0000e+00
krylov_iterations                     0
best_feasible_available               true
returned_best_feasible                true
best_feasible_value                   1.0515e+01
best_feasible_constraint_violation    0.0000e+00
best_feasible_first_order_optimality  4.5858e+00
multiplier_max_norm                   3.2426e+00
active_multiplier_count               1
detail_message                        optimality and feasibility tolerances satisfied
stall_category                        none
stall_measure                         0.0000e+00
stall_reference                       0.0000e+00
```
| Field | Meaning |
|---|---|
| `algorithm` | The method's label: `merit-method`, `sqp`, `working-set` or `internal-protocol`. |
| `iterations`, `function_evaluations` | Major iterations and the number of objective evaluations. The second is far larger when line searches or recovery steps run. |
| `constraint_violation` | The largest violation of any constraint at the returned point. |
| `first_order_optimality` | How far the returned point is from satisfying the optimality conditions. For sequential QP and the working-set method it is the largest component of the Lagrangian gradient at the returned point, using the multiplier estimate in `result.multipliers`, measured on the scale the optimality tolerance uses: for sequential QP and the interior method it is divided by the larger of 1 and the size of the gradient at the start. A start where the gradient is enormous therefore makes a large absolute residual look small (chapter 8, Program 4). |
| `step_size`, `line_search_fraction` | The fraction of the full step the last iteration accepted. |
| `backend_parameter` | The method's own control parameter: the penalty weight for the QP methods and the merit method, the barrier parameter for the interior method. Sequential QP sets its penalty weight from the size of the multipliers at each iteration, so it moves between iterations; here it ended at `3.5769`. |
| `complementarity` | The complementarity residual; meaningful for the interior method, zero otherwise. |
| `krylov_iterations` | Iterations spent in the iterative linear solver, when it is used. |
| `best_feasible_available`, `returned_best_feasible` | Whether a feasible checkpoint was recorded, and whether the answer in `params` is that checkpoint. |
| `best_feasible_value`, `best_feasible_constraint_violation`, `best_feasible_first_order_optimality` | The checkpoint's objective value, violation and stationarity. Infinity when no checkpoint exists. |
| `multiplier_max_norm`, `active_multiplier_count` | The largest multiplier magnitude, and how many constraints are active. |
| `detail_message` | The expanded message, including the stall diagnostic for a failed run. |
| `stall_category`, `stall_measure`, `stall_reference` | The stall category, the quantity that triggered it, and the threshold it was compared with. |

The solve is a success: `first_order_optimality` is `1e-8`, so the returned point satisfies the optimality conditions to high accuracy in only nine evaluations. `multiplier_max_norm` is `3.2426`, the multiplier of the one active constraint.
