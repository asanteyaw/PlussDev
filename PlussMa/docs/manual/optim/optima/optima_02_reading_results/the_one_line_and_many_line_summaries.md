<!-- generated from the_one_line_and_many_line_summaries.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The one-line and many-line summaries

[Back to Reading Results and Watching a Run](../optima_02_reading_results.md)


Two ready-made reports describe a finished run. `compact_summary` (also reachable as `print_summary`) is one line for logs. `detailed_summary` is one field per line.

```cpp
#include <pluss/ma.h>

#include <iostream>
#include <vector>

int main() {
  namespace optim = pluss::optim;
  const auto f64 = pluss::ScalarType::Float64;
  auto vec = [&](std::vector<double> v) {
    return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64);
  };
  const auto r = optim::optima([](const optim::Trace& x) { return optim::sum(optim::square(x - 3.0)); },
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
