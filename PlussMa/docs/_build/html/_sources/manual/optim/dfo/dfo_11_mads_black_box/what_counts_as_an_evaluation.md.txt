<!-- generated from what_counts_as_an_evaluation.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# What counts as an evaluation

[Back to The Black-Box MADS Minimisers](../dfo_11_mads_black_box.md)


```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
const char* name(dfo::BlackboxSolverStatus s) {
  return s == dfo::BlackboxSolverStatus::Converged ? "Converged"
         : s == dfo::BlackboxSolverStatus::ReachedEvaluationBudget ? "ReachedEvaluationBudget" : "ReachedIterationBudget";
}
int main() {
  std::cout << std::left << std::setw(40) << "setting" << std::setw(26) << "status" << std::setw(8) << "evals" << std::setw(8) << "total" << "cache hits\n";
  auto run = [](const char* label, dfo::MadsBlackboxAdvancedConfig c, bool free_half) {
    std::int64_t calls = 0;
    auto blackbox = [&](const pluss::Series& x) {
      dfo::BlackboxEvaluation e;
      e.objectives = (x - vec({1.0, 2.0})).abs().sum().reshape({1});
      e.count_evaluation = !(free_half && (calls++ % 2 == 1));
      return e;
    };
    const auto o = dfo::mads_blackbox_minimizer(blackbox, vec({4.0, -3.0}), c);
    std::cout << std::left << std::setw(40) << label << std::setw(26) << name(o.status) << std::setw(8) << o.diagnostics.evaluations
              << std::setw(8) << o.diagnostics.total_evaluations << o.diagnostics.cache_hits << "\n";
  };
  dfo::MadsBlackboxAdvancedConfig base;
  base.max_evaluations = 400;
  run("defaults", base, false);
  run("every other call free", base, true);
  auto capped = base; capped.max_total_evaluations = 300;
  run("every other call free, total cap 300", capped, true);
  auto nocount = base; nocount.count_cache_hits_in_total_evaluations = false;
  run("cache hits not counted in total", nocount, false);
  auto nocache = base; nocache.enable_cache = false;
  run("cache off", nocache, false);
}
```

Output

```text
setting                                 status                    evals   total   cache hits
defaults                                ReachedEvaluationBudget   400     401     1
every other call free                   Converged                 246     553     62
every other call free, total cap 300    ReachedEvaluationBudget   150     300     1
cache hits not counted in total         ReachedEvaluationBudget   400     400     1
cache off                               ReachedEvaluationBudget   400     400     0
```
`evaluations` is what `max_evaluations` limits. `total_evaluations` counts every call, and is limited by `max_total_evaluations`. Marking cheap pre-checks with `count_evaluation = false` lets the run do more work per expensive evaluation; the total cap keeps the overall work bounded either way.
