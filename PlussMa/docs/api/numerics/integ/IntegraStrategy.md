<!-- generated from IntegraStrategy.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `integ::IntegraStrategy`

Which engine `integra` runs.

`Quadrature` runs [`quadra`](quadra.md) only. `MonteCarlo` runs [`aleatoria`](aleatoria.md) only. `Auto` (the default) runs quadrature first and falls back to Monte Carlo only if quadrature does not converge. `IntegraResult::strategy_used` reports which engine produced the value, `Quadrature` or `MonteCarlo`, never `Auto`.

## Fields

### `Quadrature`

`enumerator`, default `0`.

Quadrature only.

### `MonteCarlo`

`enumerator`, default `1`.

Monte Carlo only.

### `Auto`

`enumerator`, default `2`.

Quadrature, then Monte Carlo if it did not converge.

## Guidance

Set as `IntegraOptions::strategy`.

## Practical Examples

### Example 1

The three strategies on a smooth integrand.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  for (const integ::IntegraStrategy strategy : {integ::IntegraStrategy::Quadrature, integ::IntegraStrategy::MonteCarlo, integ::IntegraStrategy::Auto}) {
    integ::IntegraOptions options;
    options.strategy = strategy;
    options.monte_carlo.samples = 20000;
    options.monte_carlo.seed = 1;
    const integ::IntegraResult r = integ::integra([](const Series &t) { return t.square(); }, 0.0, 3.0, options);
    out.writeln(static_cast<int>(strategy), ": used", static_cast<int>(r.strategy_used), ", value =", r.value.item<double>());
  }
}
```

Output

```text
0 : used 0 , value = 9.00000
1 : used 1 , value = 8.86248
2 : used 0 , value = 9.00000
```

### Example 2

The default is `Auto`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  out.writeln("default is Auto =", integ::IntegraOptions{}.strategy == integ::IntegraStrategy::Auto);
}
```

Output

```text
default is Auto = true
```

### Example 3

The strategy reported is the one that ran.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  integ::IntegraOptions options;
  const integ::IntegraResult r = integ::integra([](const Series &t) { return t.exp(); }, 0.0, 1.0, options);
  out.writeln(r.strategy_used == integ::IntegraStrategy::Quadrature ? "quadrature" : "monte carlo");
}
```

Output

```text
quadrature
```

## See Also

- [`integra`](integra.md): uses it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="IntegraResult.html">
    <span class="label">Previous</span>
    <span class="title">IntegraResult</span>
  </a>
  <a class="next-link" href="LebesgueOptions.html">
    <span class="label">Next</span>
    <span class="title">LebesgueOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
