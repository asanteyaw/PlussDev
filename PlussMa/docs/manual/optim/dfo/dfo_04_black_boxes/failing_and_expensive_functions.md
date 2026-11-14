<!-- generated from failing_and_expensive_functions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Functions that fail and functions that are expensive

[Back to Derivative-Free Optimization 4: Black Boxes](../dfo_04_black_boxes.md)

A black box is a function you cannot see inside: a simulation, a solver, a measurement. Two properties of black boxes shape how you call a derivative-free method. First, they fail. The simulation does not converge when a parameter exceeds some value; the instrument returns nothing. The right response is to report the failure (`evaluation_ok = false`), not to invent a huge objective value as a penalty: a made-up value has to be chosen larger than any real one and distorts the comparison between points. The solver then finds the best *usable* point, and the failed calls are the price of finding the edge of the usable region; they count against the budget.

Second, they are expensive, and optimisers revisit points. The **cache** answers repeated points from memory, so the objective runs fewer times. When a call costs minutes it is the first thing to leave on, and the only reason to turn it off is a function that is not deterministic.

## Program 1: A function that fails

Say the simulation does not converge when its first parameter exceeds 1.5. Return `evaluation_ok = false` there; do not invent a huge objective value. A made-up penalty has to be chosen larger than any real value and distorts the comparison between points.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  std::int64_t calls = 0, failures = 0;
  dfo::DfoProblem p;
  p.initial = vec({0.0, 0.0});
  p.blackbox = [&](const pluss::Series& x) {
    ++calls;
    dfo::BlackboxEvaluation e;
    if (x.select(0, 0).item<double>() > 1.5) {
      ++failures;
      e.objectives = pluss::make_series(0.0).to(f64);  // ignored: the point is flagged as failed
      e.evaluation_ok = false;
      return e;
    }
    e.objectives = (x - vec({2.0, 1.0})).square().sum().reshape({1});
    return e;
  };
  const auto r = dfo::DFO::solve(p);
  std::cout << "solver      : " << dfo::to_string(r.selected_solver) << " (" << r.selection_reason << ")\n";
  std::cout << "status      : " << dfo::to_string(r.status) << "\n";
  std::cout << std::fixed << std::setprecision(3) << "minimizer   : " << r.minimizer.select(0, 0).item<double>() << ", "
            << r.minimizer.select(0, 1).item<double>() << "\n";
  std::cout << "objective   : " << r.minimum << "  (usable optimum 0.250)\n";
  std::cout << "calls made  : " << calls << " (" << failures << " of them failed)\n";
}
```

Output

```text
solver      : MadsAdvanced (blackbox callback provided)
status      : Converged
minimizer   : 1.500, 0.991
objective   : 0.250  (usable optimum 0.250)
calls made  : 268 (106 of them failed)
```
The true minimum lies inside the failing region. The solver finds the best usable point, at the edge of it, and `Converged` means the mesh shrank around that edge. The failed calls are the price of discovering the edge; they count against the budget.

## Program 2: The cache

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }

int main() {
  std::cout << std::left << std::setw(10) << "cache" << std::setw(22) << "objective calls" << std::setw(14) << "cache hits"
            << std::setw(24) << "diagnostics.evaluations" << "objective\n";
  for (bool enabled : {true, false}) {
    std::int64_t calls = 0;
    dfo::DfoProblem p;
    p.initial = vec({3.0, -2.0});
    p.blackbox = [&](const pluss::Series& x) {
      ++calls;
      dfo::BlackboxEvaluation e;
      e.objectives = (x - vec({1.0, 0.5})).abs().sum().reshape({1});  // a kink: many poll points repeat near the optimum
      return e;
    };
    dfo::DfoOptions o;
    o.mads.enable_cache = enabled;
    const auto r = dfo::DFO::solve(p, o);
    std::cout << std::left << std::setw(10) << (enabled ? "on" : "off") << std::setw(22) << calls << std::setw(14)
              << r.diagnostics.cache_hits << std::setw(24) << r.diagnostics.evaluations << std::scientific << std::setprecision(2)
              << r.minimum << "\n";
  }
}
```

Output

```text
cache     objective calls       cache hits    diagnostics.evaluations objective
on        457                   66            457                     4.79e-13
off       664                   0             664                     0.00e+00
```
With the cache on, repeated points are answered from memory (`cache_hits`) and the objective runs fewer times than with it off. When a call is expensive this is the first thing to leave on. Turn it off only when the function is not deterministic (the same input can give different outputs), because a cached value is returned for any later request of that point.

## Decisions and pitfalls

- **Report failure, do not fake a value.** `evaluation_ok = false` lets the method discard the point without distorting its comparisons.
- **Failed calls cost budget.** Discovering the edge of the feasible region takes evaluations; allow for it.
- **`Converged` at the edge means the mesh shrank around it.** The minimum may lie inside the failing region; the answer is the best usable point.
- **Leave the cache on for deterministic functions.** `cache_hits` counts the saved calls; turn it off only if the same input can give different outputs.

Where next: [Several objectives and constraint policies](several_objectives_and_constraint_policies.md) covers the other two black-box features.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="several_objectives_and_constraint_policies.html">
    <span class="label">Next</span>
    <span class="title">Several objectives and constraint policies</span>
  </a>
</div>
<!-- apiref:nav:end -->
