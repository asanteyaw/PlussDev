<!-- generated from budgets_and_the_cache.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Budgets and the cache

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
  std::cout << std::left << std::setw(30) << "setting" << std::setw(26) << "status" << std::setw(8) << "evals" << std::setw(8) << "iters" << "cache hits\n";
  auto show = [](const char* label, dfo::PSConfig c) {
    const auto o = dfo::pattern_search_minimizer(rosenbrock, vec({-1.2, 1.0}), c);
    std::cout << std::left << std::setw(30) << label << std::setw(26) << dfo::to_string(o.status) << std::setw(8) << o.diagnostics.evaluations
              << std::setw(8) << o.diagnostics.iterations << o.diagnostics.cache_hits << "\n";
  };
  dfo::PSConfig base;
  show("defaults (rotating basis)", base);
  dfo::PSConfig fixed_axes = base;
  fixed_axes.basis_family = dfo::PSBasisFamily::Canonical;
  show("coordinate basis, cache on", fixed_axes);
  fixed_axes.enable_cache = false;
  show("coordinate basis, cache off", fixed_axes);
  auto ev = base; ev.max_evaluations = 200;
  show("max_evaluations 200", ev);
  auto it = base; it.max_iterations = 50;
  show("max_iterations 50", it);
  auto tl = base; tl.time_limit_seconds = 1e9;
  show("time_limit_seconds 1e9", tl);
}
```

Output

```text
setting                       status                    evals   iters   cache hits
defaults (rotating basis)     ReachedIterationBudget    663     200     0
coordinate basis, cache on    ReachedIterationBudget    528     200     228
coordinate basis, cache off   ReachedIterationBudget    756     200     0
max_evaluations 200           ReachedEvaluationBudget   200     60      0
max_iterations 50             ReachedIterationBudget    169     50      0
time_limit_seconds 1e9        ReachedIterationBudget    663     200     0
```
Budgets end the run with their own status. The cache answers repeated points from memory. A rotating basis never revisits a point, so the cache has nothing to save; with fixed coordinate axes the poll repeats points after each mesh refinement, and the cache saved 228 evaluations here. Turn the cache off for objectives that are not deterministic.
