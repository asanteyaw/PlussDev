<!-- generated from the_cache.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The cache

[Back to Black Boxes](../dfo_04_black_boxes.md)


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
