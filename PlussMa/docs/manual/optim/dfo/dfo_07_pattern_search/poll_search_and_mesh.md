<!-- generated from poll_search_and_mesh.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# The poll, the search step and the mesh

[Back to Derivative-Free Optimization 7: The Pattern Search Minimiser](../dfo_07_pattern_search.md)

Pattern search keeps a **mesh size** and a best point. Each iteration optionally tries a few extra *search* points, then *polls*: it evaluates points at distance `mesh` along a set of directions around the best point, moves to an improving one and grows the mesh, or, if none improves, shrinks the mesh. Because the poll directions positively span the space, a failed poll at a small mesh says that no direction improves the point at that scale: the derivative-free analogue of a zero gradient.

This page opens up the three pieces. The **poll** can use a basis of n+1 or 2n directions, a fixed or a rotating basis, stop at the first improvement or evaluate everything. The **search step** adds points chosen to jump ahead and pays off on smooth functions. The **mesh** has a scale: with bounds, the step of each variable can be scaled by its range, which matters when the variables differ by orders of magnitude.

## Program 1: A direct call

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

## Program 2: The poll

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <utility>
#include <vector>

namespace dfo = pluss::subsp::math::optimization;
using namespace pluss::tils;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}

int main() {
  std::vector<std::pair<const char*, dfo::PSConfig>> cases;
  dfo::PSConfig base;
  cases.emplace_back("2N, consecutive (default)", base);
  auto np1 = base; np1.poll_pattern = dfo::PSPollPattern::PositiveBasisNp1;
  cases.emplace_back("N+1 basis", np1);
  auto rnd = base; rnd.polling_order = dfo::PSPollingOrder::Random;
  cases.emplace_back("random order", rnd);
  auto sf = base; sf.polling_order = dfo::PSPollingOrder::SuccessFirst;
  cases.emplace_back("success-first order", sf);
  auto full = base; full.complete_poll = true;
  cases.emplace_back("complete poll", full);
  auto canon = base; canon.basis_family = dfo::PSBasisFamily::Canonical;
  cases.emplace_back("coordinate-axis basis", canon);

  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  for (auto [label, c] : cases) {
    c.search_strategy = dfo::PSSearchStrategy::Off;  // isolate the poll
    const auto o = dfo::pattern_search_minimizer(rosenbrock, vec({-1.2, 1.0}), c);
    out.writeln(label, dfo::to_string(o.status), o.minimum, "evals", o.diagnostics.evaluations);
  }
}
```

Output

```text
2N, consecutive (default) ReachedIterationBudget 1.51e-02 evals 664
N+1 basis ReachedIterationBudget 4.17e-02 evals 513
random order ReachedIterationBudget 2.07e-02 evals 641
success-first order ReachedIterationBudget 1.26e-02 evals 629
complete poll ReachedIterationBudget 9.21e-03 evals 801
coordinate-axis basis ReachedIterationBudget 3.37e+00 evals 449
```
All rows used the same 200-iteration budget, so compare the minimum reached and the evaluations spent together.

- The **N+1 basis** polls n+1 points instead of 2n: fewer evaluations per iteration, a coarser look around.
- **Order** matters only when the poll stops at the first improvement. `SuccessFirst` tries the direction that last worked first and is usually best in a curved valley.
- **Complete poll** evaluates every direction and takes the best, costing more per iteration for a better step.
- The **coordinate-axis basis** cannot follow the diagonal Rosenbrock valley: it is the worst row by orders of magnitude. The default rotating basis exists for this reason. The coordinate basis is the right choice when the axes themselves are meaningful, for instance when each variable has its own units and coordinate steps make sense.

## Program 3: The search step

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

## Program 4: The mesh

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <utility>
#include <vector>

namespace dfo = pluss::subsp::math::optimization;
using namespace pluss::tils;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}

int main() {
  std::vector<std::pair<const char*, dfo::PSConfig>> cases;
  dfo::PSConfig base;
  cases.emplace_back("defaults (RangeLog scaling)", base);
  auto off = base; off.mesh_scaling = dfo::PSMeshScaling::Off;
  cases.emplace_back("mesh_scaling Off", off);
  auto nups = base; nups.mesh_scaling = dfo::PSMeshScaling::NupsDynamic;
  cases.emplace_back("mesh_scaling NupsDynamic", nups);
  auto small = base; small.initial_mesh_size = 0.01;
  cases.emplace_back("initial_mesh_size 0.01", small);
  auto large = base; large.initial_mesh_size = 100.0;
  cases.emplace_back("initial_mesh_size 100", large);
  auto slow = base; slow.mesh_contraction = 0.9; slow.mesh_expansion = 1.1;
  cases.emplace_back("expansion 1.1, contraction 0.9", slow);

  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  for (auto [label, c] : cases) {
    c.lower_bounds = vec({-10.0, -5000.0});
    c.upper_bounds = vec({10.0, 5000.0});
    auto f = [](const pluss::Series& x) { return (x.select(0, 0) - 3.0).square() + ((x.select(0, 1) - 2000.0) / 1000.0).square(); };
    const auto o = dfo::pattern_search_minimizer(f, vec({0.0, 0.0}), c);
    out.writeln(label, dfo::to_string(o.status), o.minimum, "evals", o.diagnostics.evaluations);
  }
}
```

Output

```text
defaults (RangeLog scaling) Converged 1.41e-10 evals 221
mesh_scaling Off ReachedIterationBudget 6.76e-03 evals 660
mesh_scaling NupsDynamic Converged 2.57e-03 evals 505
initial_mesh_size 0.01 Converged 9.34e-11 evals 201
initial_mesh_size 100 Converged 3.19e-11 evals 277
expansion 1.1, contraction 0.9 ReachedIterationBudget 9.98e-11 evals 786
```
The problem has one variable of order 1 and one of order 1000. With bounds supplied, `RangeLog` scales each variable's step by its range, so the poll behaves as if both were of order one, and the run converges accurately. Without scaling, a step suited to one variable is far too small or too large for the other. The initial mesh size matters much less than the scaling: the mesh grows and shrinks by factors of 2 and 0.5, so a poor start is corrected in a few iterations. Gentler factors (1.1 and 0.9) work, but need many more iterations.

Scaling is derived from the bounds. Without bounds there is no range to read, so give the variables comparable scales yourself (divide by a typical size inside the objective) or supply bounds.

## Decisions and pitfalls

- **A non-`Converged` status is a message, not an error.** The default budget is 100 iterations per variable; raise it and continue from the returned `minimizer`.
- **Use the rotating basis unless the axes are meaningful.** The coordinate basis cannot follow a diagonal valley and is worse by orders of magnitude on Rosenbrock; it is right when each variable has its own units.
- **Searching earns its evaluations on smooth functions.** `ExpandAlongSuccess` is nearly free and two to three orders of magnitude more accurate at the same budget; random search strategies spend many trials.
- **Scale the mesh when the variables differ.** `RangeLog` scaling reads the ranges from the bounds; without bounds, divide by a typical size inside the objective.
- **Compare the minimum and the evaluations together.** A cheaper poll may reach a worse point at the same iteration budget.

Where next: [Constraints, budgets and the cache](constraints_budgets_and_the_cache.md) covers pattern search's main strength.
