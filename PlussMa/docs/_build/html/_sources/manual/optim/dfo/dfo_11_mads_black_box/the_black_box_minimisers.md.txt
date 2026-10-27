<!-- generated from the_black_box_minimisers.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The black-box MADS minimisers

[Back to Derivative-Free Optimization 11: The Black-Box MADS Minimisers](../dfo_11_mads_black_box.md)

Chapter 4 used the black-box interface through the facade. Called directly, `mads_blackbox_minimizer`, `orthomads_blackbox_minimizer` and `ltmads_blackbox_minimizer` expose every setting: how several objectives are combined, how violations are priced, what counts against the budget, the cache, the surrogate and the Pareto archive. This page covers the settings that are specific to the black-box form.

The programs follow the life of a multi-objective black-box run. A direct call returns the objectives at the minimiser, the scalar the solver minimised and the archive. **Scalarisation** chooses how several objectives become one number: the weighted sum can reach only the ends of a non-convex trade-off, where Chebyshev finds the balanced compromise. The **surrogate** is a cheap model that reorders each poll without replacing an evaluation. **What counts as an evaluation** separates cheap pre-checks from expensive calls. And the **Pareto archive** is a bounded record of the non-dominated points the search met.

## Program 1: A direct call

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
int main() {
  dfo::MadsBlackboxAdvancedConfig c;
  c.constraint_handling = dfo::ConstraintHandling::ExtremeBarrier;
  c.max_evaluations = 2000;
  auto blackbox = [](const pluss::Series& x) {
    dfo::BlackboxEvaluation e;
    const auto a = x.select(0, 0), b = x.select(0, 1);
    e.objectives = ((a - 1.0).square() + b.square()).reshape({1}).unite({((a + 1.0).square() + b.square()).reshape({1})}, 0);
    e.constraints = (a - 0.5).reshape({1});
    return e;
  };
  const auto o = dfo::mads_blackbox_minimizer(blackbox, vec({-0.5, 0.5}), c);
  const auto f = o.objective_vector.host<double>();
  std::cout << std::fixed << std::setprecision(4) << "minimizer          : " << o.minimizer.select(0, 0).item<double>() << ", " << o.minimizer.select(0, 1).item<double>() << "\n";
  std::cout << "objectives         : " << f[0] << ", " << f[1] << "\n";
  std::cout << "scalarized minimum : " << o.scalarized_minimum << "  (equal weights)\n";
  std::cout << "status             : " << dfo::to_string(o.status) << "\n";
  const auto& d = o.diagnostics;
  std::cout << "evaluations        : " << d.evaluations << " (total " << d.total_evaluations << ", cache hits " << d.cache_hits << ")\n";
  std::cout << "objective / constraint dimension: " << d.objective_dimension << " / " << d.constraint_dimension << "\n";
  std::cout << "Pareto archive     : " << d.pareto_size << " points, " << o.pareto_points.size(0) << " x " << o.pareto_points.size(1) << " matrix\n";
}
```

Output

```text
minimizer          : 0.0000, 0.0000
objectives         : 1.0000, 1.0000
scalarized minimum : 1.0000  (equal weights)
status             : Converged
evaluations        : 281 (total 283, cache hits 2)
objective / constraint dimension: 2 / 1
Pareto archive     : 251 points, 251 x 2 matrix
```
Both objectives are read back at the minimiser, along with the scalar the solver minimised. The constraint is inactive here (the compromise sits at `x0 = 0`, inside `x0 <= 0.5`). The archive holds the non-dominated points the search met, as rows.

## Program 2: Weighted sum or Chebyshev

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
int main() {
  auto blackbox = [](const pluss::Series& x) {
    dfo::BlackboxEvaluation e;
    const auto a = x.select(0, 0);
    e.objectives = a.reshape({1}).unite({(a.square() * -1.0 + 1.0).reshape({1})}, 0);
    return e;
  };
  struct Row { const char* label; dfo::MultiObjectiveScalarization s; };
  const Row rows[] = {{"WeightedSum", dfo::MultiObjectiveScalarization::WeightedSum}, {"Chebyshev", dfo::MultiObjectiveScalarization::Chebyshev}};
  std::cout << std::left << std::setw(14) << "scalarization" << std::setw(12) << "x" << std::setw(12) << "f1" << std::setw(12) << "f2" << "evals\n";
  for (const auto& r : rows) {
    dfo::MadsBlackboxAdvancedConfig c;
    c.lower_bounds = vec({0.0});
    c.upper_bounds = vec({1.0});
    c.multiobjective_scalarization = r.s;
    c.objective_weights = (r.s == dfo::MultiObjectiveScalarization::WeightedSum) ? vec({0.6, 0.4}) : vec({1.0, 1.0});
    c.objective_reference = vec({0.0, 0.0});
    c.max_evaluations = 3000;
    const auto o = dfo::mads_blackbox_minimizer(blackbox, vec({0.8}), c);
    const auto f = o.objective_vector.host<double>();
    std::cout << std::left << std::setw(14) << r.label << std::fixed << std::setprecision(4) << std::setw(12) << o.minimizer.select(0, 0).item<double>()
              << std::setw(12) << f[0] << std::setw(12) << f[1] << o.diagnostics.evaluations << "\n";
  }
}
```

Output

```text
scalarization x           f1          f2          evals
WeightedSum   1.0000      1.0000      0.0000      29
Chebyshev     0.6180      0.6180      0.6180      87
```
On a non-convex front the weighted sum can only reach an end of the trade-off, here the end at `x = 1`, and Chebyshev finds the balanced compromise at `x = 0.618`. When the front is convex both rules reach interior points and the weighted sum is cheaper. Choose Chebyshev when you want a particular balance or suspect the trade-off is not convex.

## Program 3: The surrogate

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
int main() {
  std::cout << std::left << std::setw(6) << "dim" << std::setw(20) << "surrogate_mode" << std::setw(14) << "status" << std::setw(8) << "evals" << "orderings\n";
  for (std::int64_t dim : {4, 8, 12}) {
    std::vector<double> w(dim);
    for (std::int64_t i = 0; i < dim; ++i) w[i] = static_cast<double>(i + 1);
    const auto weights = vec(w);
    for (auto mode : {dfo::SurrogateMode::Off, dfo::SurrogateMode::CandidateOrdering}) {
      dfo::MadsBlackboxAdvancedConfig c;
      c.surrogate_mode = mode;
      c.max_iterations = 5000;
      c.max_evaluations = 20000;
      auto blackbox = [&](const pluss::Series& x) {
        dfo::BlackboxEvaluation e;
        e.objectives = ((x - 1.0).square() * weights).sum().reshape({1});
        return e;
      };
      const auto o = dfo::mads_blackbox_minimizer(blackbox, filled(dim, -1.0), c);
      std::cout << std::left << std::setw(6) << dim << std::setw(20) << (mode == dfo::SurrogateMode::Off ? "Off" : "CandidateOrdering")
                << std::setw(14) << dfo::to_string(o.status) << std::setw(8) << o.diagnostics.evaluations << o.diagnostics.surrogate_orderings << "\n";
    }
  }
}
```

Output

```text
dim   surrogate_mode      status        evals   orderings
4     Off                 Converged     1454    0
4     CandidateOrdering   Converged     1250    212
8     Off                 Converged     5205    0
8     CandidateOrdering   Converged     4678    424
12    Off                 Converged     13228   0
12    CandidateOrdering   Converged     10398   644
```
The model never replaces an evaluation, so the answer is as reliable as without it; it only reorders each poll. On this smooth function it saves a tenth to a fifth of the evaluations, more in higher dimension, where polls are longer. The saving is bounded by construction: ordering can only shorten the part of a poll that comes after the first improving point, and an unsuccessful poll must still evaluate every candidate, so no model can remove those evaluations. On a rough function the ordering helps less. It is worth trying when each evaluation is expensive and the function is reasonably smooth.

## Program 4: What counts as an evaluation

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
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
    std::cout << std::left << std::setw(40) << label << std::setw(26) << dfo::to_string(o.status) << std::setw(8) << o.diagnostics.evaluations
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

## Program 5: The Pareto archive

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
int main() {
  std::cout << std::left << std::setw(22) << "pareto_archive_limit" << std::setw(16) << "archive size" << std::setw(4) << "" << "x range\n";
  for (std::int64_t limit : {256, 20, 5}) {
    dfo::MadsBlackboxAdvancedConfig c;
    c.pareto_archive_limit = limit;
    c.multiobjective_scalarization = dfo::MultiObjectiveScalarization::WeightedSum;
    c.objective_weights = vec({0.5, 0.5});
    c.max_evaluations = 800;
    auto blackbox = [](const pluss::Series& x) {
      dfo::BlackboxEvaluation e;
      const auto a = x.select(0, 0);
      e.objectives = a.square().reshape({1}).unite({(a - 2.0).square().reshape({1})}, 0);
      return e;
    };
    const auto o = dfo::mads_blackbox_minimizer(blackbox, vec({5.0}), c);
    const auto p = o.pareto_points.reshape({o.pareto_points.numel()}).pull<double>();
    
    double lo = 1e300, hi = -1e300;
    for (double v : p) { lo = std::min(lo, v); hi = std::max(hi, v); }
    std::cout << std::left << std::setw(22) << limit << std::setw(16) << p.size() << std::setw(4) << "" << std::fixed << std::setprecision(3) << lo << " .. " << hi << "\n";
  }
}
```

Output

```text
pareto_archive_limit  archive size        x range
256                   73                  -0.100 .. 1.940
20                    20                  -0.100 .. 1.940
5                     5                   -0.100 .. 1.940
```
The archive is a record of the search, not a computed front: it holds non-dominated points among those evaluated. With a tight limit it drops the most crowded entries, so a small archive still spans the front. To trace a front, solve with several weightings and merge the archives.

## Decisions and pitfalls

- **Use Chebyshev for a non-convex front.** The weighted sum reaches only an end of the trade-off; on a convex front both rules reach interior points and the weighted sum is cheaper.
- **The surrogate never replaces an evaluation.** The answer is as reliable as without it; it only reorders each poll, saving a tenth to a fifth of the evaluations on a smooth function (more in higher dimension).
- **Mark cheap pre-checks with `count_evaluation = false`.** `evaluations` is what `max_evaluations` limits; `total_evaluations` counts every call and is limited by `max_total_evaluations`, which keeps the overall work bounded.
- **The archive is a record, not a front.** To trace a front, solve with several weightings and merge the archives; with a tight limit the archive drops the most crowded entries so it still spans the front.
- **Read both objectives at the minimiser, not only the scalar.** The scalar depends on your weights; the vector is the trade-off.

Where next: [Chapter 12](../dfo_12_mads_variants.md) covers the specialised variants.
