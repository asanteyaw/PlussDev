<!-- generated from atomic.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::measure::atomic`

Integration against a finite set of point masses: the exact sum `Σ wᵢ f(xᵢ)`.

## Signature

```cpp
operators::Operator operators::measure::atomic(std::string_view variable, pluss::RangeView<double> points, pluss::RangeView<double> weights);
```

## What It Does

`atomic("x", points, weights)` is the operator `f ↦ Σᵢ wᵢ f(xᵢ)`: integration against the measure with mass `wᵢ` at each point `xᵢ`. There is no integral to approximate; the result is an exact finite sum of the function composed with each point, and it is a function of the remaining variables.

It is the expectation of a discrete random variable (probabilities as weights), a quadrature rule (nodes and weights), a scenario analysis, or a Dirac mass (one point, weight 1). The points and the weights must be non-empty and have the same length. The weights need not sum to 1 or be positive; they are masses.

## Parameters

`std::string_view variable`
: The variable the function is evaluated at.

`pluss::RangeView<double> points`
: The locations `xᵢ` of the masses.

`pluss::RangeView<double> weights`
: The masses `wᵢ`, one per point.

## Returns

An `Operator` whose application returns `Σ wᵢ f(xᵢ)` as a function of the remaining variables.

## Errors

- `needs matching, non-empty points and weights`.

## Practical Examples

### Example 1

Expectations of a discrete distribution with values `1, 2, 4` and probabilities `½, ¼, ¼`: `E[X] = ½ + ½ + 1 = 2`, `E[X²] = ½ + 1 + 4 = 5.5`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace measure = pluss::operators::measure;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const std::vector<double> values = {1.0, 2.0, 4.0}, probabilities = {0.5, 0.25, 0.25};
  const operators::Operator E = operators::measure::atomic("x", pluss::RangeView<double>(values), pluss::RangeView<double>(probabilities));
  const math::Function id = math::function({"x"}, [](math::Function x) { return x; });
  const math::Function square = math::function({"x"}, [](math::Function x) { return x * x; });
  out.writeln("E[X]   =", E(id)().item<double>());
  out.writeln("E[X^2] =", E(square)().item<double>());
  out.writeln("Var    =", E(square)().item<double>() - E(id)().item<double>() * E(id)().item<double>());
}
```

Output

```text
E[X]   = 2.0000
E[X^2] = 5.5000
Var    = 1.5000
```

### Example 2

A Dirac mass: one point with weight 1 evaluates the function there. And a quadrature rule: the three-point Simpson weights `1/6, 4/6, 1/6` at `0, ½, 1` integrate `x²` over `[0, 1]` exactly.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace measure = pluss::operators::measure;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const std::vector<double> at_three = {3.0}, one = {1.0};
  const math::Function f = math::function({"x"}, [](math::Function x) { return x * x + 1.0; });
  out.writeln("Dirac at 3:    ", operators::measure::atomic("x", pluss::RangeView<double>(at_three), pluss::RangeView<double>(one))(f)().item<double>(), "  (f(3) = 10)");

  const std::vector<double> nodes = {0.0, 0.5, 1.0}, weights = {1.0 / 6.0, 4.0 / 6.0, 1.0 / 6.0};
  const math::Function square = math::function({"x"}, [](math::Function x) { return x * x; });
  out.writeln("Simpson, x^2:  ", operators::measure::atomic("x", pluss::RangeView<double>(nodes), pluss::RangeView<double>(weights))(square)().item<double>(), "  (1/3)");
}
```

Output

```text
Dirac at 3:     10.000000   (f(3) = 10)
Simpson, x^2:   0.333333   (1/3)
```

### Example 3

Scenario valuation. Four equally likely terminal prices and a call payoff with strike 100: `E[max(S − K, 0)]` is a weighted sum, and a function of the strike stays free if it is a variable.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace measure = pluss::operators::measure;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const std::vector<double> prices = {80.0, 95.0, 110.0, 130.0}, probabilities = {0.25, 0.25, 0.25, 0.25};
  const math::Function payoff = math::function({"s", "k"}, [](math::Function s, math::Function k) { return (s - k) * 0.5 + (s - k).abs() * 0.5; });
  const math::Function value = operators::measure::atomic("s", pluss::RangeView<double>(prices), pluss::RangeView<double>(probabilities))(payoff);
  out.writeln("variables left:", value.arity());
  out.writeln("E[max(S - K, 0)] for K = 90, 100, 120:", value({pluss::make_series({90.0, 100.0, 120.0})}).items<double>());
}
```

Output

```text
variables left: 1
E[max(S - K, 0)] for K = 90, 100, 120: [16.2500, 10.0000, 2.5000]
```

### Example 4

The points and the weights must match and be non-empty.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>
#include <vector>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace measure = pluss::operators::measure;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto first_line = [](const pluss::Error &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };

  const std::vector<double> points = {1.0, 2.0, 4.0}, short_weights = {1.0}, none;
  try { (void)operators::measure::atomic("x", pluss::RangeView<double>(points), pluss::RangeView<double>(short_weights)); } catch (const pluss::Error &e) { out.writeln("lengths differ:", first_line(e)); }
  try { (void)operators::measure::atomic("x", pluss::RangeView<double>(none), pluss::RangeView<double>(none)); } catch (const pluss::Error &e) { out.writeln("empty:        ", first_line(e)); }
}
```

Output

```text
lengths differ: operators::measure::atomic: needs matching, non-empty points and weights
empty:         operators::measure::atomic: needs matching, non-empty points and weights
```

## See Also

- [`weighted`](weighted.md): a continuous density
- [`stieltjes`](stieltjes.md): a density plus point masses
- [`lebesgue`](lebesgue.md): Lebesgue measure

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="lebesgue.html">
    <span class="label">Next</span>
    <span class="title">lebesgue</span>
  </a>
</div>
<!-- apiref:nav:end -->
