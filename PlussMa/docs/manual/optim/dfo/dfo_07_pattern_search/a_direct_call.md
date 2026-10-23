<!-- generated from a_direct_call.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# A direct call

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
  const auto o = dfo::pattern_search_minimizer(rosenbrock, vec({-1.2, 1.0}));
  std::cout << std::fixed << std::setprecision(5) << "minimizer     : " << o.minimizer.select(0, 0).item<double>() << ", "
            << o.minimizer.select(0, 1).item<double>() << "\n";
  std::cout << std::scientific << std::setprecision(2) << "minimum       : " << o.minimum << "\n";
  std::cout << "status        : " << dfo::to_string(o.status) << "\n";
  const auto& d = o.diagnostics;
  std::cout << "iterations    : " << d.iterations << "\nevaluations   : " << d.evaluations << "\ncache hits    : " << d.cache_hits
            << "\nsearch trials : " << d.search_trials << " (" << d.search_successes << " successful)\nfinal mesh    : " << d.mesh_size
            << "\nlast step     : " << d.last_step << "\n";
}
```

Output

```text
minimizer     : 0.99297, 0.98613
minimum       : 5.15e-05
status        : ReachedIterationBudget
iterations    : 200
evaluations   : 663
cache hits    : 0
search trials : 94 (30 successful)
final mesh    : 9.77e-04
last step     : successful_poll
```
The default budget is 100 iterations per variable, so this two-variable run stops at 200 iterations with `ReachedIterationBudget` while still improving. A status other than `Converged` is a message, not an error: raise the budget and continue from the returned `minimizer`.
