<!-- generated from weighted.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::measure::weighted`

Integration against a density: `∫ f(x) ρ(x) dx`.

## Signature

```cpp
operators::Operator operators::measure::weighted(std::string_view variable, const math::Function &density, double lower, double upper, pluss::RangeView<double> null_points = {});
```

## What It Does

`weighted("x", ρ, a, b)` is the operator `f ↦ ∫ₐᵇ f(x) ρ(x) dx`: integration against the measure with density `ρ` with respect to Lebesgue measure, `dμ = ρ dx`. This is how expectations are written. If `ρ` is a probability density, the result is `E[f(X)]`; with `f(x) = x` it is the mean, with `f = x²` the second moment. The density is a function whose variables are matched, by name, to the integrand's.

A change of measure is a different density: integrating against `ρ · (dQ/dP)` is the expectation under the new measure. Limits may be infinite and `null_points` work as for [`lebesgue`](lebesgue.md). The density's normalisation is not checked; integrate the constant function 1 to find the total mass.

## Parameters

`std::string_view variable`
: The integration variable.

`const math::Function &density`
: The density `ρ`, a function whose variables are matched by name to the integrand's.

`double lower`
: The lower limit; may be `-inf`.

`double upper`
: The upper limit; may be `+inf`.

`pluss::RangeView<double> null_points`
: Points the function is never evaluated at.

## Returns

An `Operator` whose application returns `∫ f ρ dx` as a function of the remaining variables.

## Errors

- `needs a variable, a density and lower < upper`.

## Practical Examples

### Example 1

Moments of the density `ρ(x) = 2x` on `[0, 1]`: total mass 1, mean `∫ x·2x = 2/3`, second moment `∫ x²·2x = 1/2`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace measure = pluss::operators::measure;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function rho = math::function({"x"}, [](math::Function x) { return 2.0 * x; });
  const math::Function one = math::function({"x"}, [](math::Function x) { return x * 0.0 + 1.0; });
  const math::Function id = math::function({"x"}, [](math::Function x) { return x; });
  const math::Function square = math::function({"x"}, [](math::Function x) { return x * x; });
  const operators::Operator expectation = operators::measure::weighted("x", rho, 0.0, 1.0);
  out.writeln("mass:           ", expectation(one)().item<double>());
  out.writeln("mean:           ", expectation(id)().item<double>(), "  (2/3)");
  out.writeln("second moment:  ", expectation(square)().item<double>(), "  (1/2)");
}
```

Output

```text
mass:            1.000000
mean:            0.666667   (2/3)
second moment:   0.500000   (1/2)
```

### Example 2

The standard normal density over the whole line: `E[X²] = 1`, `E[X⁴] = 3`, `E[cos X] = e^{−1/2}`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <limits>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace measure = pluss::operators::measure;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const double inf = std::numeric_limits<double>::infinity();

  const math::Function phi = math::function({"x"}, [](math::Function x) { return (-(x * x) / 2.0).exp() / std::sqrt(2.0 * std::acos(-1.0)); });
  const operators::Operator expectation = operators::measure::weighted("x", phi, -inf, inf);
  out.writeln("E[X^2]  =", expectation(math::function({"x"}, [](math::Function x) { return x * x; }))().item<double>());
  out.writeln("E[X^4]  =", expectation(math::function({"x"}, [](math::Function x) { return x.pow(4); }))().item<double>());
  out.writeln("E[cos X] =", expectation(math::function({"x"}, [](math::Function x) { return x.cos(); }))().item<double>(), "  e^(-1/2) =", std::exp(-0.5));
}
```

Output

```text
E[X^2]  = 1.000000
E[X^4]  = 3.000000
E[cos X] = 0.606531   e^(-1/2) = 0.606531
```

### Example 3

A change of measure. Multiplying the normal density by the likelihood ratio `exp(θx − θ²/2)` gives the normal with mean `θ`; the mean under the new measure is `θ = 0.7`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <limits>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace measure = pluss::operators::measure;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const double inf = std::numeric_limits<double>::infinity();

  const double theta = 0.7;
  const math::Function tilted = math::function({"x"}, [&](math::Function x) {
    return (-(x * x) / 2.0 + theta * x - 0.5 * theta * theta).exp() / std::sqrt(2.0 * std::acos(-1.0));   // one exponent: no 0 * inf in the tails
  });
  const math::Function id = math::function({"x"}, [](math::Function x) { return x; });
  out.writeln("mean under the tilted measure:", operators::measure::weighted("x", tilted, -inf, inf)(id)().item<double>());
}
```

Output

```text
mean under the tilted measure: 0.700000
```

### Example 4

A missing density or a bad interval is refused.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

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

  try { (void)operators::measure::weighted("x", math::Function{}, 0.0, 1.0); } catch (const pluss::Error &e) { out.writeln(first_line(e)); }
}
```

Output

```text
operators::measure::weighted: needs a variable, a density and lower < upper
```

## See Also

- [`lebesgue`](lebesgue.md): density 1
- [`stieltjes`](stieltjes.md): a density plus jumps
- [`integral`](../integral/integral.md): the plain integral

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="stieltjes.html">
    <span class="label">Previous</span>
    <span class="title">stieltjes</span>
  </a>
</div>
<!-- apiref:nav:end -->
