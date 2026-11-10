<!-- generated from QuadraRule.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `integ::QuadraRule`

The composite rule `quadra` applies on an interval.

`Trapezoidal` joins neighbouring nodes with straight lines: robust, order 2. `Simpson` (the default) fits parabolas through triples of nodes: order 4, exact for cubics. `GaussLegendre` places 5 or 10 nodes per panel at the roots of a Legendre polynomial: spectral accuracy for smooth integrands, the best choice when the integrand is analytic. `Auto` tries `GaussLegendre` first and falls back to `Simpson`, then `Trapezoidal`, only if the previous rule fails to converge within its refinement budget; the rule that produced the answer is reported in `QuadraResult::rule_used`.

## Fields

### `Trapezoidal`

`enumerator`, default `0`.

Composite trapezoid, order 2.

### `Simpson`

`enumerator`, default `1`.

Composite Simpson, order 4 (the default).

### `GaussLegendre`

`enumerator`, default `2`.

Panelled Gauss-Legendre, 5 or 10 points a panel.

### `Auto`

`enumerator`, default `3`.

GaussLegendre, then Simpson, then Trapezoidal, on non-convergence.

## Guidance

Set as `QuadraOptions::rule` or `QuadraNdOptions::rule`.

## Practical Examples

### Example 1

The three rules on `∫₀¹ eˣ dx = e − 1 = 1.71828183`, at the same node budget with refinement off. Gauss-Legendre is the most accurate, the trapezoid the least.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(12));
  const auto f = [](const Series &t) { return t.exp(); };
  for (const integ::QuadraRule rule : {integ::QuadraRule::Trapezoidal, integ::QuadraRule::Simpson, integ::QuadraRule::GaussLegendre}) {
    integ::QuadraOptions options;
    options.rule = rule;
    options.adaptive = false;
    options.nodes = 9;
    options.panels = 2;
    const integ::QuadraResult r = integ::quadra(f, 0.0, 1.0, options);
    out.writeln(static_cast<int>(rule), ": error =", std::abs(r.value.item<double>() - (std::exp(1.0) - 1.0)), ", evaluations =", r.evaluations);
  }
}
```

Output

```text
0 : error = 0.001767416025 , evaluations = 10
1 : error = 0.000000953466 , evaluations = 11
2 : error = 0.000000000000 , evaluations = 10
```

### Example 2

`Auto` and the rule it ended on. A smooth integrand stops at the first rung.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  integ::QuadraOptions options;
  options.rule = integ::QuadraRule::Auto;
  const integ::QuadraResult r = integ::quadra([](const Series &t) { return t.exp(); }, 0.0, 1.0, options);
  out.writeln("rule used =", static_cast<int>(r.rule_used), "(2 is GaussLegendre), evaluations =", r.evaluations);
}
```

Output

```text
rule used = 2 (2 is GaussLegendre), evaluations = 480
```

### Example 3

An integrand with a kink. `|x − 1/3|` on `[0, 1]` integrates to `5/18`. A kink costs the smooth rules their high order, but the refinement still converges, and the rule that produced the answer is reported (here Gauss-Legendre).

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(9));
  integ::QuadraOptions options;
  options.rule = integ::QuadraRule::Auto;
  options.tolerance = 1e-9;
  const integ::QuadraResult r = integ::quadra([](const Series &t) { return (t - 1.0 / 3.0).abs(); }, 0.0, 1.0, options);
  out.writeln("value =", r.value.item<double>(), ", exact =", 5.0 / 18.0, ", rule used =", static_cast<int>(r.rule_used), ", converged =", r.converged);
}
```

Output

```text
value = 0.277777778 , exact = 0.277777778 , rule used = 2 , converged = true
```

## See Also

- [`quadra`](quadra.md): the engine
- [`QuadraOptions`](QuadraOptions.md): the settings

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="QuadraResult.html">
    <span class="label">Previous</span>
    <span class="title">QuadraResult</span>
  </a>
  <a class="next-link" href="QuadraScalarOptions.html">
    <span class="label">Next</span>
    <span class="title">QuadraScalarOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
