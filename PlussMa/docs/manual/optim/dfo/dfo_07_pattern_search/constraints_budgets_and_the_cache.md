<!-- generated from constraints_budgets_and_the_cache.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Constraints, budgets and the cache

[Back to Derivative-Free Optimization 7: The Pattern Search Minimiser](../dfo_07_pattern_search.md)

Pattern search is the method behind constrained derivative-free problems. **Bounds and the two linear forms** are enforced on every trial point exactly, with no penalty, so the returned point satisfies them to rounding; the poll adapts its directions to the constraints active at the current point and adds the edges of the feasible cone, so the method can slide along a face and leave a vertex. **Nonlinear constraints** are handled by one of three policies, from the cheapest (a barrier that rejects violating points) to the most accurate (an augmented Lagrangian that adds multiplier estimates to a penalty and refines them over several outer rounds, the only one that can satisfy an equality).

The page ends with budgets and the cache. Each budget (evaluations, iterations) ends the run with its own status, and the cache answers repeated points from memory; a rotating basis never revisits a point, so the cache has nothing to save, while fixed axes repeat points after each mesh refinement.

## Program 5: Linear constraints

Bounds and the two linear forms are enforced on every trial point exactly. No penalty is involved, so the returned point satisfies them to rounding.

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
  // By hand: on the plane x0 - x1 = 0.5 with x0 + x1 + x2 = 3 we have x1 = x0 - 0.5 and x2 = 3.5 - 2 x0, so
  // f = (x0-2)^2 + (x0-2.5)^2 + (1.5-2 x0)^2, f' = 12 x0 - 15 = 0, giving x0 = 1.25, x1 = 0.75, x2 = 1, f = 3.125.
  dfo::PSConfig c;
  c.lower_bounds = vec({0.0, 0.0, 0.0});
  c.upper_bounds = vec({5.0, 5.0, 5.0});
  c.linear_ineq_matrix = pluss::make_series(std::vector<double>{1.0, 1.0, 1.0}, {1, 3}).to(f64);
  c.linear_ineq_rhs = vec({3.0});
  c.linear_eq_matrix = pluss::make_series(std::vector<double>{1.0, -1.0, 0.0}, {1, 3}).to(f64);
  c.linear_eq_rhs = vec({0.5});
  auto f = [](const pluss::Series& x) { return (x - 2.0).square().sum(); };
  const auto o = dfo::pattern_search_minimizer(f, vec({0.5, 0.0, 0.5}), c);
  const auto x = o.minimizer.host<double>();
  std::cout << std::fixed << std::setprecision(4) << "minimizer : " << x[0] << ", " << x[1] << ", " << x[2] << "  (expected 1.2500, 0.7500, 1.0000)\n";
  std::cout << "minimum   : " << o.minimum << "  (expected 3.1250)\n";
  std::cout << "x0 - x1   : " << x[0] - x[1] << "  (must be 0.5)\n";
  std::cout << "sum       : " << x[0] + x[1] + x[2] << "  (must be <= 3)\n";
  std::cout << "status    : " << dfo::to_string(o.status) << ", evaluations " << o.diagnostics.evaluations << "\n";
}
```

Output

```text
minimizer : 1.2500, 0.7500, 1.0000  (expected 1.2500, 0.7500, 1.0000)
minimum   : 3.1250  (expected 3.1250)
x0 - x1   : 0.5000  (must be 0.5)
sum       : 3.0000  (must be <= 3)
status    : Converged, evaluations 177
```
The start is a vertex of the feasible region, where two inequalities and the equality meet. Moving off it means going along an edge of the feasible cone, which the method builds from the active constraints.

## Program 6: Nonlinear constraints and their policies

`nonlinear_constraints` returns a `PSConstraintValues` with an `inequality` vector (feasible when every entry is `<= 0`) and an `equality` vector (`= 0`).

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
  struct P { const char* name; dfo::PSConstraintPolicy p; };
  const P rows[] = {{"ExtremeBarrier", dfo::PSConstraintPolicy::ExtremeBarrier},
                    {"MeritPenalty", dfo::PSConstraintPolicy::MeritPenalty},
                    {"AugmentedLagrangian", dfo::PSConstraintPolicy::AugmentedLagrangian}};
  std::cout << std::left << std::setw(22) << "policy" << std::setw(26) << "status" << std::setw(11) << "objective" << std::setw(12) << "violation"
            << std::setw(8) << "evals" << "outer rounds\n";
  for (const auto& r : rows) {
    dfo::PSConfig c;
    c.constraint_policy = r.p;
    c.nonlinear_constraints = [](const pluss::Series& x) {
      dfo::PSConstraintValues v;
      v.inequality = (x.square().sum() - 1.0).reshape({1});
      return v;
    };
    auto f = [](const pluss::Series& x) { return (x - 2.0).square().sum(); };
    c.max_iterations = 4000;
    c.max_evaluations = 40000;
    const auto o = dfo::pattern_search_minimizer(f, vec({0.0, 0.0}), c);
    std::cout << std::left << std::setw(22) << r.name << std::setw(26) << dfo::to_string(o.status) << std::fixed << std::setprecision(4)
              << std::setw(11) << o.minimum << std::scientific << std::setprecision(1) << std::setw(12) << o.diagnostics.max_constraint_violation
              << std::defaultfloat << std::setw(8) << o.diagnostics.evaluations << o.diagnostics.outer_iterations << "\n";
  }
}
```

Output

```text
policy                status                    objective  violation   evals   outer rounds
ExtremeBarrier        Converged                 3.3437     0.0e+00     121     0
MeritPenalty          Converged                 3.3438     0.0e+00     342     0
AugmentedLagrangian   Converged                 3.3431     7.6e-07     1398    8
```
- **ExtremeBarrier** rejects any violating point. Cheapest, and always feasible, but it can only approach a boundary from inside, so it stops a little short.
- **MeritPenalty** adds a penalty on the violation to the objective and increases it as needed. It reaches closer to the boundary.
- **AugmentedLagrangian** (the default) adds multiplier estimates to the penalty and refines them over several outer rounds. It costs the most evaluations and gives the most accurate answer, and it is the policy built for equality constraints, which a barrier cannot satisfy (a random point never lies exactly on one). Its violation is small but not zero.

## Program 7: Budgets and the cache

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

## Decisions and pitfalls

- **Linear constraints are exact; nonlinear ones are approximate.** A linear constraint holds to rounding at the returned point; a nonlinear one holds to a small violation that depends on the policy.
- **Choose the policy for the constraint.** `ExtremeBarrier` is cheapest and always feasible but stops short of the boundary; `MeritPenalty` reaches closer; `AugmentedLagrangian` (the default) is the most accurate and the only one that can satisfy equalities.
- **An equality's violation is small, not zero.** A random point never lies exactly on a curve; read the reported violation.
- **Turn the cache off for objectives that are not deterministic.** The same input must give the same output for the cache to be valid.
- **Start at a vertex only knowingly.** The method moves along an edge of the feasible cone from a vertex; if it does not move, check that the cone has directions.

Where next: [Chapter 8](../dfo_08_differential_evolution.md) turns to the population methods.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="poll_search_and_mesh.html">
    <span class="label">Previous</span>
    <span class="title">The poll, the search step and the mesh</span>
  </a>
</div>
<!-- apiref:nav:end -->
