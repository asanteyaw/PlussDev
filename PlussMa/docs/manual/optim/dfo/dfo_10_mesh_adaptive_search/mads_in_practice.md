<!-- generated from mads_in_practice.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Mesh adaptive direct search in practice

[Back to Derivative-Free Optimization 10: Mesh Adaptive Direct Search](../dfo_10_mesh_adaptive_search.md)

Mesh adaptive direct search (MADS) is a pattern search whose poll directions keep changing, so that over a run they point in every direction rather than only along the axes. It handles rough, non-smooth and discontinuous functions, and its stopping rule is tied to a quantity you can reason about: the *frame size*, the size of the poll. The two scalar entry points, with different poll-direction schemes (`mesh_adaptive_direct_search` and an OrthoMADS variant), are covered here; Chapter 11 covers the black-box interface with failures, constraints and several objectives.

The programs move from a direct call to the settings. The two poll schemes are compared on a curved valley and a non-smooth function. The mesh and the frame are varied (starting scale, refinement speed, the tolerance that ends the run, opportunistic polling). The search step is shown to help on a function with several basins, and the stopping tests (the frame test and the improvement-window test) are shown separately.

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
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}
int main() {
  const auto o = dfo::mesh_adaptive_direct_search_minimizer(rosenbrock, vec({-1.2, 1.0}));
  std::cout << std::fixed << std::setprecision(5) << "minimizer   : " << o.minimizer.select(0, 0).item<double>() << ", " << o.minimizer.select(0, 1).item<double>() << "\n";
  std::cout << std::scientific << std::setprecision(2) << "minimum     : " << o.minimum << "\n";
  std::cout << "status      : " << dfo::to_string(o.status) << "\n";
  const auto& d = o.diagnostics;
  std::cout << "iterations  : " << d.iterations << "\nevaluations : " << d.evaluations << "\nfinal mesh  : " << d.mesh_size
            << "\nfinal frame : " << d.frame_size << "\nlast phase  : " << d.last_phase << "\n";
}
```

Output

```text
minimizer   : 0.90775, 0.82350
minimum     : 8.54e-03
status      : ReachedEvaluationBudget
iterations  : 311
evaluations : 1000
final mesh  : 7.32e-06
final frame : 1.88e-03
last phase  : refine_mesh
```
The default budget is 500 evaluations per variable, so this two-variable run stops after 1000 evaluations with `ReachedEvaluationBudget`, still some way from the minimum. Read the final frame size: when it is much larger than `mesh_tolerance`, the run was cut off, not finished. Raise `max_evaluations` (Program 2 does) and continue.

## Program 2: The two poll schemes

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <utility>
#include <vector>

namespace dfo = pluss::subsp::math::optimization;
using namespace pluss::tils;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}
int main() {
  auto kink = [](const pluss::Series& x) { return (x - vec({1.0, -2.0, 0.5, 3.0})).abs().sum(); };
  dfo::MadsBlackboxConfig c;
  c.max_iterations = 4000;
  c.max_evaluations = 40000;
  auto lt = c;
  lt.poll_scheme = dfo::MadsPollScheme::LtMadsRandom2N;

  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  out.writeln("Rosenbrock");
  const std::vector<std::pair<const char*, dfo::BlackboxSolverOutcome>> rosen = {
      {"orthomads_minimizer", dfo::orthomads_minimizer(rosenbrock, vec({-1.2, 1.0}), c)},
      {"ltmads_minimizer", dfo::ltmads_minimizer(rosenbrock, vec({-1.2, 1.0}), c)},
      {"mesh_adaptive..., LtMadsRandom2N", dfo::mesh_adaptive_direct_search_minimizer(rosenbrock, vec({-1.2, 1.0}), lt)}};
  for (const auto& [label, o] : rosen) out.writeln(" ", label, dfo::to_string(o.status), o.minimum, "evals", o.diagnostics.evaluations);

  out.writeln("sum |x - c|, 4 variables");
  const std::vector<std::pair<const char*, dfo::BlackboxSolverOutcome>> sums = {
      {"orthomads_minimizer", dfo::orthomads_minimizer(kink, vec({0, 0, 0, 0}), c)},
      {"ltmads_minimizer", dfo::ltmads_minimizer(kink, vec({0, 0, 0, 0}), c)}};
  for (const auto& [label, o] : sums) out.writeln(" ", label, dfo::to_string(o.status), o.minimum, "evals", o.diagnostics.evaluations);
}
```

Output

```text
Rosenbrock
  orthomads_minimizer ReachedIterationBudget 3.20e-15 evals 12761
  ltmads_minimizer ReachedIterationBudget 3.43e-15 evals 12928
  mesh_adaptive..., LtMadsRandom2N ReachedIterationBudget 3.43e-15 evals 12928
sum |x - c|, 4 variables
  orthomads_minimizer Converged 0.00e+00 evals 2012
  ltmads_minimizer Converged 0.00e+00 evals 1784
```
Both schemes follow the curved Rosenbrock valley to about 1e-15 within the iteration limit, and both solve the non-smooth sum of absolute values exactly, where a simplex stalls (Chapter 2). The two are interchangeable in practice; the differences you see are the random path. `mesh_adaptive_direct_search_minimizer` with `LtMadsRandom2N` is exactly `ltmads_minimizer`.

## Program 3: The mesh and the frame

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <utility>
#include <vector>

namespace dfo = pluss::subsp::math::optimization;
using namespace pluss::tils;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}
int main() {
  std::vector<std::pair<const char*, dfo::MadsBlackboxConfig>> cases;
  dfo::MadsBlackboxConfig base;
  cases.emplace_back("defaults", base);
  auto big = base; big.initial_frame_size = 8.0; big.initial_mesh_size = 8.0;
  cases.emplace_back("initial frame and mesh 8", big);
  auto tiny = base; tiny.initial_frame_size = 0.01; tiny.initial_mesh_size = 0.01;
  cases.emplace_back("initial frame and mesh 0.01", tiny);
  auto gentle = base; gentle.frame_refine_factor = 0.8; gentle.mesh_refine_factor = 0.64;
  cases.emplace_back("gentle refinement (0.8, 0.64)", gentle);
  auto aggressive = base; aggressive.frame_refine_factor = 0.25; aggressive.mesh_refine_factor = 0.0625;
  cases.emplace_back("aggressive refinement", aggressive);
  auto tight = base; tight.check_every = 0; tight.mesh_tolerance = 1e-12;
  cases.emplace_back("mesh_tolerance 1e-12, no window", tight);
  auto mid = base; mid.check_every = 0; mid.mesh_tolerance = 1e-8;
  cases.emplace_back("mesh_tolerance 1e-8, no window", mid);
  auto loose = base; loose.check_every = 0; loose.mesh_tolerance = 1e-3;
  cases.emplace_back("mesh_tolerance 1e-3, no window", loose);
  auto full = base; full.opportunistic = false;
  cases.emplace_back("opportunistic off", full);

  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  for (auto [label, c] : cases) {
    auto f = [](const pluss::Series& x) {
      const auto d = x - vec({1.0, -1.0, 2.0, 0.5});
      const auto s = d.select(0, 0) + d.select(0, 1) + d.select(0, 2) + d.select(0, 3);
      return d.square().sum() + s.square() * 0.8 + 1.0;
    };
    c.max_iterations = 4000;
    c.max_evaluations = 40000;
    const auto o = dfo::orthomads_minimizer(f, vec({3.0, 2.0, -1.0, 1.0}), c);
    out.writeln(label, dfo::to_string(o.status), o.minimum, "evals", o.diagnostics.evaluations);
  }
}
```

Output

```text
defaults Converged 1.00e+00 evals 880
initial frame and mesh 8 Converged 1.00e+00 evals 839
initial frame and mesh 0.01 Converged 1.00e+00 evals 834
gentle refinement (0.8, 0.64) Converged 1.00e+00 evals 2905
aggressive refinement Converged 1.00e+00 evals 659
mesh_tolerance 1e-12, no window Converged 1.00e+00 evals 984
mesh_tolerance 1e-8, no window Converged 1.00e+00 evals 880
mesh_tolerance 1e-3, no window Converged 1.00e+00 evals 409
opportunistic off Converged 1.00e+00 evals 1281
```
- The **starting scale** hardly matters: the frame grows or shrinks by factors of 2 and 0.5 until it fits.
- **Refinement speed** matters. Shrinking gently (0.8) polls many times at nearly the same scale and costs several times the evaluations; shrinking quickly (0.25) is cheaper.
- **`mesh_tolerance`** is the accuracy you ask for. With the window test off, it alone ends the run, and tightening it from 1e-3 to 1e-12 costs about two and a half times the evaluations.
- **Opportunistic off** evaluates the whole poll every time and takes the best point: more evaluations per iteration.

## Program 4: The search step and bounds

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}
int main() {
  const double pi = 3.14159265358979323846;
  auto rastrigin = [pi](const pluss::Series& x) { return (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum(); };
  std::cout << std::left << std::setw(20) << "search_points" << "minimum for seeds 1, 2, 3 (evaluations in brackets)\n";
  for (std::int64_t points : {0, 8, 32}) {
    std::cout << std::left << std::setw(20) << points;
    for (std::uint64_t seed : {1ULL, 2ULL, 3ULL}) {
      dfo::MadsBlackboxConfig c;
      c.lower_bounds = filled(2, -5.12);
      c.upper_bounds = filled(2, 5.12);
      c.search_points = points;
      c.rng_seed = seed;
      c.max_iterations = 4000;
      c.max_evaluations = 20000;
      const auto o = dfo::orthomads_minimizer(rastrigin, vec({3.2, 2.7}), c);
      std::cout << std::scientific << std::setprecision(2) << o.minimum << " (" << std::defaultfloat << o.diagnostics.evaluations << ")   ";
    }
    std::cout << "\n";
  }
}
```

Output

```text
search_points       minimum for seeds 1, 2, 3 (evaluations in brackets)
0                   9.95e-01 (355)   9.95e-01 (355)   9.95e-01 (355)   
8                   9.95e-01 (1007)   9.95e-01 (858)   0.00e+00 (1209)   
32                  9.95e-01 (3559)   0.00e+00 (3691)   0.00e+00 (3140)   
```
Without a search step the method creeps from its start into the nearest local minimum (value 0.995) on every seed. The search step tries points across the frame, so it sometimes lands in a better basin: with 8 points one seed in three finds the global minimum, with 32 two in three. The price is evaluations, roughly three times as many for 8 points and ten times for 32. Use it when a local minimum is not good enough and the budget allows. With bounds, every trial point stays inside the box.

## Program 5: Stopping

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <utility>
#include <vector>

namespace dfo = pluss::subsp::math::optimization;
using namespace pluss::tils;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
pluss::Series rosenbrock(const pluss::Series& x) {
  const auto a = x.select(0, 0), b = x.select(0, 1);
  return (b - a * a).square() * 100.0 + (a * -1.0 + 1.0).square();
}
int main() {
  std::vector<std::pair<const char*, dfo::MadsBlackboxConfig>> cases;
  dfo::MadsBlackboxConfig base;
  base.max_iterations = 20000;
  base.max_evaluations = 100000;
  cases.emplace_back("defaults (check_every 10, 1e-8)", base);
  auto off = base; off.check_every = 0;
  cases.emplace_back("check_every 0 (frame test only)", off);
  auto loose = base; loose.relative_improvement_tolerance = 1e-4;
  cases.emplace_back("relative tolerance 1e-4", loose);
  auto it = base; it.max_iterations = 100;
  cases.emplace_back("max_iterations 100", it);
  auto ev = base; ev.max_evaluations = 300;
  cases.emplace_back("max_evaluations 300", ev);

  OutputEngine out;
  out.print_defaults(PrintOptions{}.sci(2));
  for (auto [label, c] : cases) {
    const auto o = dfo::orthomads_minimizer(rosenbrock, vec({-1.2, 1.0}), c);
    out.writeln(label, dfo::to_string(o.status), o.minimum, "evals", o.diagnostics.evaluations, "iterations", o.diagnostics.iterations);
  }
}
```

Output

```text
defaults (check_every 10, 1e-8) Converged 8.76e-23 evals 19280 iterations 6050
check_every 0 (frame test only) Converged 1.79e-11 evals 8822 iterations 2769
relative tolerance 1e-4 Converged 7.89e-19 evals 15636 iterations 4910
max_iterations 100 ReachedIterationBudget 1.36e-01 evals 327 iterations 100
max_evaluations 300 ReachedEvaluationBudget 1.86e-01 evals 300 iterations 92
```
By default both the improvement-window test and the frame test must hold, so the window test can only delay a stop, never cause one early. Switching it off (`check_every` 0) lets the frame alone end the run: here it stops after 8822 evaluations instead of 19280, at a value of 2e-11 instead of 9e-23. So the window test is what buys the last digits of accuracy on a function whose minimum value is 0, at roughly twice the cost. A looser relative tolerance sits between the two, and budgets end the run with their own status.

## Decisions and pitfalls

- **Read the final frame size.** When it is much larger than `mesh_tolerance`, the run was cut off, not finished; the default budget (500 evaluations per variable) ends a run well short of the minimum, so raise `max_evaluations` and continue.
- **The poll schemes are interchangeable in practice.** The differences you see are the random path.
- **Refine quickly.** Shrinking the frame gently (0.8) polls many times at nearly the same scale and costs several times the evaluations of shrinking quickly (0.25); the starting scale hardly matters.
- **`mesh_tolerance` is the accuracy you ask for.** Tightening it from 1e-3 to 1e-12 costs about two and a half times the evaluations.
- **Add a search step when there are several basins.** Without it MADS creeps into the nearest local minimum on every seed; with 32 search points two seeds in three found the global one, at a price in evaluations.
- **The window test only delays a stop.** By default both tests must hold; switching it off (`check_every` 0) lets the frame alone end the run, here after 8822 evaluations instead of 19280.

Where next: [Chapter 11](../dfo_11_mads_black_box.md) uses MADS as a black-box minimiser.
