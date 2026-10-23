<!-- generated from disco_mads.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Disco MADS

[Back to The MADS Variants](../dfo_12_mads_variants.md)


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
const char* name(dfo::BlackboxSolverStatus s) {
  return s == dfo::BlackboxSolverStatus::Converged ? "Converged"
         : s == dfo::BlackboxSolverStatus::ReachedEvaluationBudget ? "ReachedEvaluationBudget" : "ReachedIterationBudget";
}
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
