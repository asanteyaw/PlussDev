<!-- generated from the_report_summaries_and_multipliers.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The report, the summaries and the multipliers

[Back to Optima 2: Reading Results and Watching a Run](../optima_02_reading_results.md)

Beyond the verdict, a finished run reports how it got there and what it learned. `result.output` is the structured report: the work done, the constraint violation and optimality measure at the returned point, the method's own control parameter, whether a feasible checkpoint was recorded, and, for a failed run, the stall diagnostic. Two ready-made summaries (one line for a log, one field per line for a terminal) print the same facts, and the **Lagrange multipliers** say how hard each constraint pushes on the answer.

Multipliers are the most useful and least read part of the output. For an inequality, a multiplier is the first-order price: relax the limit by one unit and the objective improves by about that amount. A constraint with multiplier zero is inactive and costs nothing; a large multiplier marks the constraint that is shaping the answer. The last program here is about the **best feasible checkpoint**: a run that ends without converging may have visited a better point than its last, and the solver keeps the best feasible point it saw.

## Program 5: Every field of the report

`result.output` is the structured report. This program prints each field of one solve so you can match it against the table below.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const pluss::Series& x) { return (x - 3.0).square().sum(); };
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return p.at("x").square().sum() - 1.0; });

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

## Program 11: The one-line and many-line summaries

Two ready-made reports describe a finished run. `compact_summary` (also reachable as `print_summary`) is one line for logs. `detailed_summary` is one field per line.

```cpp
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  const auto r = optim::optima([](const pluss::Series& x) { return (x - 3.0).square().sum(); },
                               vec({0.0, 0.0}), {}, optim::constraints().ubs(vec({2.0, 2.5})));

  std::cout << "-- compact_summary --\n";
  optim::reports::compact_summary(std::cout, r) << "\n";
  std::cout << "-- print_summary (same line) --\n";
  optim::print_summary(std::cout, r) << "\n";
  std::cout << "-- detailed_summary --\n";
  optim::reports::detailed_summary(std::cout, r) << "\n";
  return 0;
}
```

Output

```text
-- compact_summary --
algorithm=sqp exit=1 iterations=1 evaluations=2 objective=1.25 violation=0 first_order=2e-08 backend_parameter=5 complementarity=0 stall=none message=optimality and feasibility tolerances satisfied
-- print_summary (same line) --
algorithm=sqp exit=1 iterations=1 evaluations=2 objective=1.25 violation=0 first_order=2e-08 backend_parameter=5 complementarity=0 stall=none message=optimality and feasibility tolerances satisfied
-- detailed_summary --
algorithm=sqp
exit=1
iterations=1
evaluations=2
constraint_violation=0
first_order_optimality=2e-08
step_size=1
backend_parameter=5
complementarity=0
krylov_iterations=0
best_feasible_available=true
returned_best_feasible=true
stall=none
stall_measure=0
stall_reference=0
message=optimality and feasibility tolerances satisfied
```
Both print the same facts. The compact form is the one to put in a log line; the detailed form is for a terminal, and its field names match the `output` table above.

## Program 10: Reading the multipliers

A Lagrange multiplier measures how hard a constraint pushes on the answer. For an inequality it is the first-order price: relaxing the limit by one unit lowers the objective by about that amount. The result carries six multiplier groups, one per constraint family. This program uses three problems whose multipliers can be worked out by hand and checks each.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const pluss::Series& x) { return (x - 3.0).square().sum(); };
  std::cout << std::fixed << std::setprecision(4);

  // Bounds x <= (2, 2.5): the gradient at the answer is (-2, -1), so the upper multipliers are (2, 1).
  const auto bounded = optim::optima(objective, vec({0.0, 0.0}), {}, optim::constraints().ubs(vec({2.0, 2.5})));
  const auto upper = bounded.multipliers.upper_bounds.host<double>();
  const auto lower = bounded.multipliers.lower_bounds.host<double>();
  std::cout << "bounds:            upper = [" << upper[0] << ", " << upper[1] << "] (expected 2, 1); lower multipliers: "
            << lower.size() << " (no lower bound set)\n";

  // x0 + x1 <= 4: at (2, 2) the gradient is (-2, -2) and the constraint row is (1, 1), so the multiplier is 2.
  const auto linear = optim::optima(
      objective, vec({0.0, 0.0}), {},
      optim::constraints().ln_ineq(pluss::make_series(std::vector<double>{1, 1}, {1, 2}).to(f64), vec({4.0})));
  std::cout << "linear inequality: " << linear.multipliers.linear_inequalities.host<double>()[0]
            << " (expected 2)\n";

  // x0^2 + x1^2 = 1: at (1,1)/sqrt(2) the gradient is -(3 - 0.7071) * (2, 2); the row is 2*(0.7071, 0.7071).
  const auto nonlinear = optim::optima(
      objective, vec({1.0, 0.0}), {},
      optim::constraints().nl_eq([](const Pack& p) { return p.at("x").square().sum() - 1.0; }));
  std::cout << "nonlinear equality: " << nonlinear.multipliers.nonlinear_equalities.host<double>()[0]
            << " (expected 3.2426)\n";
  return 0;
}
```

Output

```text
bounds:            upper = [2.0000, 1.0000] (expected 2, 1); lower multipliers: 0 (no lower bound set)
linear inequality: 2.0000 (expected 2)
nonlinear equality: 3.2426 (expected 3.2426)
```
The bound problem has gradient `(-2, -1)` at its answer, so the upper multipliers are `(2, 1)` and the lower group is empty because no lower bound was set. The linear inequality has multiplier `2`, the price of tightening `x0 + x1 <= 4`. The nonlinear equality has multiplier `3.2426`, which equals `(3 - 1/sqrt 2) * sqrt 2`, the gradient magnitude divided by the constraint-gradient magnitude. A family the problem does not use has an empty multiplier series; an inactive constraint has multiplier zero.

## Program 9: The best feasible checkpoint

A run that ends without converging may have visited a better point than its last. The solver keeps the best feasible point it saw. `result.best_feasible` holds it and `output.best_feasible_*` summarizes it. `returned_best_feasible` tells you whether the answer in `params` is the checkpoint or the final iterate.

```cpp
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::subsp::math::optim;
  using Pack = optim::ParameterPack<pluss::Series>;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  auto objective = [](const pluss::Series& x) { return (x - 3.0).square().sum(); };
  const auto disc = optim::constraints().nl_ineq(
      [](const Pack& p) { return p.at("x").square().sum() - 1.0; });

  optim::OptimaOptions options;
  options.sequential_qp.max_iterations = 3;
  const auto r = optim::optima(objective, vec({0.0, 0.0}), options, disc);

  std::cout << std::boolalpha;
  std::cout << "exit flag " << static_cast<int>(r.exit_flag) << ", converged " << r.converged << "\n";
  std::cout << "returned answer: objective " << r.evaluation.value.item<double>() << ", violation "
            << r.evaluation.max_constraint_violation << "\n";
  std::cout << "best feasible available: " << r.output.best_feasible_available << "\n";
  std::cout << "answer is the checkpoint: " << r.output.returned_best_feasible << "\n";
  if (r.best_feasible) {
    const auto x = r.best_feasible->params.at("x").host<double>();
    std::cout << "checkpoint point [" << x[0] << ", " << x[1] << "]\n";
    std::cout << "checkpoint objective " << r.best_feasible->value.item<double>() << ", violation "
              << r.best_feasible->max_constraint_violation << "\n";
    std::cout << "report matches checkpoint: "
              << (r.output.best_feasible_value == r.best_feasible->value.item<double>()) << "\n";
  }
  return 0;
}
```

Output

```text
exit flag 0, converged false
returned answer: objective 10.5147, violation 3.00243e-06
best feasible available: true
answer is the checkpoint: false
checkpoint point [0, 0]
checkpoint objective 18, violation 0
report matches checkpoint: true
```
Sequential QP was stopped after three iterations, so the flag is `MaxIterations` and `converged` is false. The returned answer is excellent in value (`10.5147`) but infeasible by `3e-06`, just over the default tolerance, because this method approaches feasibility from outside. The only feasible point it ever visited is the starting point, `(0, 0)` with objective `18`, so that is the checkpoint, and the answer in `params` is not that checkpoint (`false`). Here the checkpoint is feasible but far worse, which shows the trade-off: neither is automatically the one to use. A program that wants a usable point from a run that did not converge should read `best_feasible`, and check its violation, rather than `params`.

## Decisions and pitfalls

- **Read the optimality measure on its own scale.** `first_order_optimality` is scaled by the size of the gradient at the start for some methods, so a huge starting gradient makes a large absolute residual look small; compare it with the tolerance, not with zero.
- **Multipliers exist only for the families the problem uses.** An unused family has an empty multiplier series, and an inactive constraint has multiplier zero; do not read an empty series as "zero cost".
- **Use the multiplier as a price, with care.** It is a first-order estimate: it is accurate for small changes of the limit and says nothing about large ones.
- **The checkpoint is not automatically the better answer.** `returned_best_feasible` tells you whether `params` is the checkpoint; a final iterate that is slightly infeasible but excellent in value may be worth more than a feasible but poor checkpoint. Decide with the tolerance you can accept.
- **Log the compact summary, show the detailed one.** The field names of the detailed summary match the `output` table above.

Where next: [Watching a run](watching_a_run.md) shows the progress hooks that let you see these quantities as the run proceeds.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="exit_flags_and_stall_categories.html">
    <span class="label">Previous</span>
    <span class="title">Exit flags and stall categories</span>
  </a>
  <a class="next-link" href="watching_a_run.html">
    <span class="label">Next</span>
    <span class="title">Watching a run</span>
  </a>
</div>
<!-- apiref:nav:end -->
