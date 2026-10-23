<!-- generated from three_ways_to_call.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Three ways to call

[Back to First Problems](../dfo_01_first_problems.md)


`DFO::solve` takes a `DfoProblem` and `DfoOptions` you fill in field by field, which is the form to use once a problem has bounds or constraints. `DFO::build(start)` chains setters (`with_objective`, `with_bounds`, `with_solver`, `with_seed`, `with_max_evaluations`, and so on) and finishes with `.solve()`.

```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series objective(const pluss::Series& x) { return (x - vec({1.0, -2.0})).square().sum() + 3.0; }

void report(const char* label, const dfo::DfoResult& r) {
  std::cout << std::left << std::setw(14) << label << std::setw(26) << dfo::to_string(r.status) << std::setw(12)
            << dfo::to_string(r.selected_solver) << std::fixed << std::setprecision(6) << std::setw(12) << r.minimum
            << "evals " << r.diagnostics.evaluations << "\n";
}

int main() {
  const auto start = vec({4.0, 4.0});

  report("solve_dfo", dfo::solve_dfo(objective, start));

  dfo::DfoProblem problem;
  problem.initial = start;
  problem.objective = objective;
  report("DFO::solve", dfo::DFO::solve(problem, dfo::DfoOptions{}));

  report("DfoBuilder", dfo::DFO::build(start).with_objective(objective).solve());
}
```

Output

```text
solve_dfo     Converged                 NelderMead  3.000000    evals 92
DFO::solve    Converged                 NelderMead  3.000000    evals 92
DfoBuilder    Converged                 NelderMead  3.000000    evals 92
```
The three entry points do the same thing; choose by taste.
