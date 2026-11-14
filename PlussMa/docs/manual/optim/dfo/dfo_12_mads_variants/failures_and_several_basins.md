<!-- generated from failures_and_several_basins.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Variants for failing evaluations and several basins

[Back to Derivative-Free Optimization 12: The MADS Variants](../dfo_12_mads_variants.md)

The other two variants address different problems. **Disco MADS** is for a black box that *fails often*. A failed evaluation (`evaluation_ok` false, or a non-finite objective) remains a failure for the search, but Disco also tries a few points *near* the failed one and remembers the best valid one, which can replace the search's final answer if it is better. The rescue runs only once a given share of the evaluations has failed, so on a function that rarely fails it costs about what plain MADS costs.

**Variable-neighbourhood MADS** is for a function with several basins. Each later stage starts from the best point so far plus a random perturbation of growing size, runs MADS, and keeps the better outcome, so later stages reach further. The budget is divided among the stages, which is the trade-off: more stages explore more but each run is shorter.

## Program 3: Disco MADS

| `DiscoMadsConfig` | Meaning | Default |
|---|---|---|
| `base` | the engine settings | |
| `rescue_trials` | nearby points tried after a failed evaluation | 8 |
| `rescue_failure_rate` | the rescue only runs once this share of the evaluations has failed (after 20 calls); 0 rescues every failure | 0.5 |
| `rescue_radius` | distance of the first nearby point | 0.25 |
| `rescue_radius_decay` | factor applied to the radius for each later trial | 0.75 |
| `rng_seed` | | 5489 |

A failed evaluation (`evaluation_ok` false, or a non-finite objective) remains a failure for the search. Disco also evaluates the nearby points and remembers the best valid one, which can replace the search's final answer if it is better. The reported minimum is always the true value at the returned point.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>

namespace dfo = pluss::subsp::math::optimization;

const auto f64 = pluss::ScalarType::Float64;
pluss::Series vec(std::vector<double> v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(f64); }
pluss::Series filled(std::int64_t n, double v) { return pluss::make_series(std::vector<double>(n, v), {n}).to(f64); }
int main() {
  std::cout << std::left << std::setw(10) << "failing" << std::setw(24) << "method" << std::setw(12) << "minimum" << std::setw(14) << "calls made" << "failed calls\n";
  for (double rate : {0.3, 0.8}) {
    std::int64_t calls = 0, failures = 0;
    auto blackbox = [&](const pluss::Series& x) {
      ++calls;
      dfo::BlackboxEvaluation e;
      const double a = x.select(0, 0).item<double>(), b = x.select(0, 1).item<double>();
      double h = std::sin(12345.678 * a + 9876.543 * b + 0.5) * 43758.5453;
      h -= std::floor(h);
      if (h < rate) {
        ++failures;
        e.objectives = pluss::make_series(std::nan("")).to(f64).reshape({1});
        return e;
      }
      e.objectives = (x - vec({2.0, 1.0})).square().sum().reshape({1});
      return e;
    };
    auto report = [&](const std::string& label, const dfo::BlackboxAdvancedOutcome& o) {
      std::cout << std::left << std::setw(10) << std::to_string(static_cast<int>(rate * 100)) + "%" << std::setw(24) << label << std::scientific << std::setprecision(2)
                << std::setw(12) << o.scalarized_minimum << std::defaultfloat << std::setw(14) << calls << failures << "\n";
    };
    {
      calls = failures = 0;
      dfo::MadsBlackboxAdvancedConfig c;
      c.max_evaluations = 3000;
      report("plain MADS", dfo::mads_blackbox_minimizer(blackbox, vec({0.0, 0.0}), c));
    }
    for (std::int64_t trials : {3, 12}) {
      calls = failures = 0;
      dfo::DiscoMadsConfig c;
      c.base.max_evaluations = 3000;
      c.rescue_trials = trials;
      report("Disco, rescue_trials " + std::to_string(trials), dfo::discomads_blackbox_minimizer(blackbox, vec({0.0, 0.0}), c));
    }
  }
}
```

Output

```text
failing   method                  minimum     calls made    failed calls
30%       plain MADS              3.20e-25    444           114
30%       Disco, rescue_trials 3  3.20e-25    459           119
30%       Disco, rescue_trials 12 3.20e-25    504           128
80%       plain MADS              4.99e+00    151           128
80%       Disco, rescue_trials 3  3.39e+00    487           397
80%       Disco, rescue_trials 12 2.86e+00    1495          1215
```
When 30% of the evaluations fail, the failure rate is below `rescue_failure_rate`, the rescue stays off, and Disco costs about what plain MADS costs (a few more calls while the failure rate is still being estimated). When 80% fail, plain MADS stops far from the minimum (about 5), and the rescue finds better points (about 3.4 with 3 trials, 2.9 with 12) at three to ten times the calls. Neither reaches the minimum, because most of the space is simply not observable. The rescue is for high failure rates; at low ones it switches itself off.

## Program 4: Variable-neighbourhood MADS

| `VnsMadsConfig` | Meaning | Default |
|---|---|---|
| `base` | the engine settings | |
| `neighborhoods` | number of stages (the first is plain MADS) | 5 |
| `initial_kick_scale` | scale of the first random perturbation | 0.2 |
| `kick_growth` | factor applied to the scale at each later stage | 1.7 |
| `rng_seed` | | 5489 |

Each later stage starts from the best point so far plus a random perturbation, runs MADS, and keeps the better outcome. The perturbations grow, so later stages reach further.

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
  const double pi = 3.14159265358979323846;
  auto blackbox = [pi](const pluss::Series& x) {
    dfo::BlackboxEvaluation e;
    e.objectives = (x.square() - (x * (2.0 * pi)).cos() * 10.0 + 10.0).sum().reshape({1});
    return e;
  };
  std::cout << std::left << std::setw(26) << "neighborhoods" << "minimum for seeds 1, 2, 3\n";
  for (std::int64_t stages : {1, 3, 6, 12}) {
    std::cout << std::left << std::setw(26) << stages;
    for (std::uint64_t seed : {1ULL, 2ULL, 3ULL}) {
      dfo::VnsMadsConfig c;
      c.base.lower_bounds = filled(2, -5.12);
      c.base.upper_bounds = filled(2, 5.12);
      c.base.max_evaluations = 9000;
      c.base.max_iterations = 5000;
      c.base.rng_seed = seed;
      c.rng_seed = seed;
      c.neighborhoods = stages;
      c.initial_kick_scale = 1.0;
      const auto o = dfo::vnsmads_blackbox_minimizer(blackbox, vec({3.2, 2.7}), c);
      std::cout << std::scientific << std::setprecision(2) << o.scalarized_minimum << "   ";
    }
    std::cout << "\n";
  }
}
```

Output

```text
neighborhoods             minimum for seeds 1, 2, 3
1                         9.95e-01   9.95e-01   9.95e-01   
3                         9.95e-01   9.95e-01   0.00e+00   
6                         9.95e-01   9.95e-01   0.00e+00   
12                        0.00e+00   9.95e-01   0.00e+00   
```
A single stage (plain MADS) settles in the nearest local minimum, value 0.995, on all three seeds. More stages find the global minimum on more seeds: with 12 stages, two of three. The budget is divided among stages, so more stages mean shorter runs each; on a harder problem the balance matters. Compare with the search step of Chapter 10 and the population methods of Chapters 8 and 9, which also address several basins.

## Decisions and pitfalls

- **Disco's rescue stays off until failures are common.** At a 30% failure rate (below `rescue_failure_rate`) it behaves like plain MADS; at 80% plain MADS stops far from the minimum (about 5) and the rescue finds it.
- **The reported minimum is always the true value at the returned point.** A rescued point replaces the answer only if it is better and valid.
- **More stages find the global basin on more seeds.** One stage (plain MADS) settled in the nearest local minimum on every seed; 12 stages found the global one on two of three.
- **Stages divide the budget.** On a harder problem the balance between many short runs and a few long ones matters; compare on your own problem.
- **Run several seeds either way.** Both variants are random; report the spread.

Where next: That completes the derivative-free chapters; [linear programming](../../linprog.md) is next.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="decomposition_variants.html">
    <span class="label">Previous</span>
    <span class="title">Variants for many variables: cooperative and partial-space MADS</span>
  </a>
</div>
<!-- apiref:nav:end -->
