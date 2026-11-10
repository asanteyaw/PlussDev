<!-- generated from PSPollingOrder.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `dfo::PSPollingOrder`

The order the poll points are tried in.

With the default opportunistic poll, a poll stops at the first improving point, so the order decides which improvement is taken. `Consecutive` tries the directions in basis order. `Random` shuffles them each poll. `SuccessFirst` starts with the direction that last worked, which helps along a valley.

## Fields

### `Consecutive`

`enumerator`, default `0`.

Basis order.

### `Random`

`enumerator`, default `1`.

A random order each poll.

### `SuccessFirst`

`enumerator`, default `2`.

The last successful direction first.

## Guidance

Set as `PSConfig::polling_order`.

## Practical Examples

### Example 1

The three orders on the Rosenbrock valley.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  for (const auto order : {dfo::PSPollingOrder::Consecutive, dfo::PSPollingOrder::Random, dfo::PSPollingOrder::SuccessFirst}) {
    dfo::PSConfig config;
    config.polling_order = order;
    config.search_strategy = dfo::PSSearchStrategy::Off;
    const dfo::PSOutcome o = dfo::pattern_search_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config);
    out.writeln(static_cast<int>(order), ": evaluations =", o.diagnostics.evaluations, ", minimum =", o.minimum);
  }
}
```

Output

```text
0 : evaluations = 664 , minimum = 0.01508061
1 : evaluations = 641 , minimum = 0.02073305
2 : evaluations = 629 , minimum = 0.01262310
```

### Example 2

A different seed changes the random order and nothing else.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  for (const std::uint64_t seed : {1, 2}) {
    dfo::PSConfig config;
    config.polling_order = dfo::PSPollingOrder::Random;
    config.rng_seed = seed;
    out.writeln("seed", seed, ": evaluations =", dfo::pattern_search_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config).diagnostics.evaluations);
  }
}
```

Output

```text
seed 1 : evaluations = 651
seed 2 : evaluations = 685
```

### Example 3

Success-first with a complete poll.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace dfo = pluss::subsp::math::optimization;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const auto rosenbrock = [](const Series &x) {
    const Series a = x.select(0, 0), b = x.select(0, 1);
    return (b - a * a).square() * 100.0 + (1.0 - a).square();
  };

  dfo::PSConfig config;
  config.polling_order = dfo::PSPollingOrder::SuccessFirst;
  config.complete_poll = true;
  const dfo::PSOutcome o = dfo::pattern_search_minimizer(rosenbrock, pluss::series::make({-1.2, 1.0}), config);
  out.writeln("evaluations =", o.diagnostics.evaluations, ", minimum =", o.minimum);
}
```

Output

```text
evaluations = 795 , minimum = 0.00001905
```

## See Also

- [`PSConfig`](PSConfig.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="PSPollPattern.html">
    <span class="label">Previous</span>
    <span class="title">PSPollPattern</span>
  </a>
  <a class="next-link" href="PSSearchStrategy.html">
    <span class="label">Next</span>
    <span class="title">PSSearchStrategy</span>
  </a>
</div>
<!-- apiref:nav:end -->
