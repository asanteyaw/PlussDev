<!-- generated from a_function_that_fails.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A function that fails

[Back to Black Boxes](../dfo_04_black_boxes.md)


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
