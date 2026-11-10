<!-- generated from DiscoMadsConfig.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::DiscoMadsConfig`

Settings of Disco MADS: the base configuration plus the rescue around failed evaluations.

`base` is the [`MadsBlackboxAdvancedConfig`](MadsBlackboxAdvancedConfig.md). Around each failure up to `rescue_trials` nearby points are tried, at random offsets of radius `rescue_radius` that shrinks by `rescue_radius_decay` each trial. The rescue is switched on only after a warm-up of 20 calls and once the share of failures reaches `rescue_failure_rate`; below it plain MADS copes and the extra calls would be wasted, and 0 rescues every failure.

## Fields

### `base`

`dfo::MadsBlackboxAdvancedConfig`, default `{}`.

The MADS settings.

### `rescue_trials`

`std::int64_t`, default `8`.

Nearby points tried around each failure.

### `rescue_failure_rate`

`double`, default `0.5`.

Share of failed evaluations at which the rescue starts; 0 rescues every failure.

### `rescue_radius`

`double`, default `0.25`.

Radius of the first rescue offsets.

### `rescue_radius_decay`

`double`, default `0.75`.

Factor by which the radius shrinks each trial.

### `rng_seed`

`std::uint64_t`, default `5489`.

Seed of the offsets.

## Guidance

Passed to `discomads_blackbox_minimizer`.

## Practical Examples

### Example 1

The failure-rate threshold, on a black box in which 60% of the points fail. With the threshold at 0 or 0.5 the rescue runs (60% is above it); at 0.9 it never starts and the run equals plain MADS.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto make_box = [](double rate, std::int64_t *calls) {
    return [rate, calls](const Series &x) {
      ++*calls;
      dfo::BlackboxEvaluation e;
      const double a = x.select(0, 0).item<double>(), b = x.select(0, 1).item<double>();
      double h = std::sin(12345.678 * a + 9876.543 * b + 0.5) * 43758.5453;   // a fixed pseudo-random fraction of points fails
      h -= std::floor(h);
      e.objectives = (h < rate) ? pluss::make_series(std::nan("")).reshape({1}) : (x - pluss::series::make({2.0, 1.0})).square().sum().reshape({1});
      return e;
    };
  };
  for (const double rate : {0.0, 0.5, 0.9}) {
    std::int64_t calls = 0;
    dfo::DiscoMadsConfig config;
    config.base.max_evaluations = 3000;
    config.rescue_failure_rate = rate;
    const dfo::BlackboxAdvancedOutcome o = dfo::discomads_blackbox_minimizer(make_box(0.6, &calls), pluss::series::make({0.0, 0.0}), config);
    out.writeln("rescue from failure rate", rate, ": minimum =", o.scalarized_minimum, ", calls =", calls);
  }
}
```

Output

```text
rescue from failure rate 0.0000 : minimum = 0.0000 , calls = 4521
rescue from failure rate 0.5000 : minimum = 0.0000 , calls = 4521
rescue from failure rate 0.9000 : minimum = 0.0000 , calls = 801
```

### Example 2

The rescue radius, on a black box in which 80% of the points fail.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto make_box = [](double rate, std::int64_t *calls) {
    return [rate, calls](const Series &x) {
      ++*calls;
      dfo::BlackboxEvaluation e;
      const double a = x.select(0, 0).item<double>(), b = x.select(0, 1).item<double>();
      double h = std::sin(12345.678 * a + 9876.543 * b + 0.5) * 43758.5453;   // a fixed pseudo-random fraction of points fails
      h -= std::floor(h);
      e.objectives = (h < rate) ? pluss::make_series(std::nan("")).reshape({1}) : (x - pluss::series::make({2.0, 1.0})).square().sum().reshape({1});
      return e;
    };
  };
  for (const double radius : {0.05, 0.25, 1.0}) {
    std::int64_t calls = 0;
    dfo::DiscoMadsConfig config;
    config.base.max_evaluations = 3000;
    config.rescue_radius = radius;
    const dfo::BlackboxAdvancedOutcome o = dfo::discomads_blackbox_minimizer(make_box(0.8, &calls), pluss::series::make({0.0, 0.0}), config);
    out.writeln("rescue radius", radius, ": minimum =", o.scalarized_minimum, ", calls =", calls);
  }
}
```

Output

```text
rescue radius 0.0500 : minimum = 4.7522 , calls = 1047
rescue radius 0.2500 : minimum = 3.3620 , calls = 1047
rescue radius 1.0000 : minimum = 0.8264 , calls = 1047
```

### Example 3

The number of trials and the radius decay, on a black box in which 80% of the points fail.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto make_box = [](double rate, std::int64_t *calls) {
    return [rate, calls](const Series &x) {
      ++*calls;
      dfo::BlackboxEvaluation e;
      const double a = x.select(0, 0).item<double>(), b = x.select(0, 1).item<double>();
      double h = std::sin(12345.678 * a + 9876.543 * b + 0.5) * 43758.5453;   // a fixed pseudo-random fraction of points fails
      h -= std::floor(h);
      e.objectives = (h < rate) ? pluss::make_series(std::nan("")).reshape({1}) : (x - pluss::series::make({2.0, 1.0})).square().sum().reshape({1});
      return e;
    };
  };
  for (const std::int64_t trials : {2, 16}) {
    std::int64_t calls = 0;
    dfo::DiscoMadsConfig config;
    config.base.max_evaluations = 3000;
    config.rescue_trials = trials;
    config.rescue_radius_decay = 0.9;
    const dfo::BlackboxAdvancedOutcome o = dfo::discomads_blackbox_minimizer(make_box(0.8, &calls), pluss::series::make({0.0, 0.0}), config);
    out.writeln("trials", trials, ": minimum =", o.scalarized_minimum, ", calls =", calls);
  }
}
```

Output

```text
trials 2 : minimum = 3.4911 , calls = 375
trials 16 : minimum = 2.8642 , calls = 1943
```

## See Also

- [`discomads_blackbox_minimizer`](discomads_blackbox_minimizer.md): uses it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="DifferentialEvolutionPrmmConfig.html">
    <span class="label">Previous</span>
    <span class="title">DifferentialEvolutionPrmmConfig</span>
  </a>
  <a class="next-link" href="MadsBlackboxAdvancedConfig.html">
    <span class="label">Next</span>
    <span class="title">MadsBlackboxAdvancedConfig</span>
  </a>
</div>
<!-- apiref:nav:end -->
