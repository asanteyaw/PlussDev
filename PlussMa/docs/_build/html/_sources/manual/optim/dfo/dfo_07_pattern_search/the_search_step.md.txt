<!-- generated from the_search_step.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The search step

[Back to The Pattern Search Minimiser](../dfo_07_pattern_search.md)


```cpp
#include <pluss/ma.h>

#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}

int main() {
  struct S { const char* name; dfo::PSSearchStrategy s; };
  const S rows[] = {{"Off", dfo::PSSearchStrategy::Off},
                    {"ExpandAlongSuccess (default)", dfo::PSSearchStrategy::ExpandAlongSuccess},
                    {"RandomDirections", dfo::PSSearchStrategy::RandomDirections},
                    {"ExpandAndRandomDirections", dfo::PSSearchStrategy::ExpandAndRandomDirections}};
  std::cout << std::left << std::setw(32) << "search strategy" << std::setw(26) << "status" << std::setw(11) << "minimum" << std::setw(8) << "evals"
            << "trials / successes\n";
  for (const auto& r : rows) {
    dfo::PSConfig c;
    c.search_strategy = r.s;
    const auto o = dfo::pattern_search_minimizer(rosenbrock, vec({-1.2, 1.0}), c);
    std::cout << std::left << std::setw(32) << r.name << std::setw(26) << dfo::to_string(o.status) << std::scientific << std::setprecision(2)
              << std::setw(11) << o.minimum << std::defaultfloat << std::setw(8) << o.diagnostics.evaluations << o.diagnostics.search_trials
              << " / " << o.diagnostics.search_successes << "\n";
  }
}
```

Output

```text
search strategy                 status                    minimum    evals   trials / successes
Off                             ReachedIterationBudget    1.51e-02   664     0 / 0
ExpandAlongSuccess (default)    ReachedIterationBudget    5.15e-05   663     94 / 30
RandomDirections                Converged                 8.51e-09   2186    1732 / 52
ExpandAndRandomDirections       Converged                 1.84e-11   1565    1238 / 39
```
Searching earns its evaluations on smooth functions: along a successful direction `ExpandAlongSuccess` costs almost nothing extra and is two to three orders of magnitude more accurate here at the same budget. The random strategies spend many trials (1732 trials, 52 successes) but converge fully within the budget. Use them when the function has structure the poll cannot see.
