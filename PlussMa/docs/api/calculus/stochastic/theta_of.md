<!-- generated from theta_of.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::stochastic::theta_of`

The `θ` of a convention: where in each step the integrand is read.

## Signature

```cpp
constexpr double operators::stochastic::theta_of(operators::stochastic::Convention convention, double theta = 0.0) noexcept;
```

## What It Does

Each `Convention` reads the integrand at a point `(1 − θ)·left + θ·right` of each step. `theta_of(convention)` returns that `θ`: 0 for `Ito`, ½ for `Stratonovich`, 1 for `Klimontovich`, and for `Theta` the value you pass as the second argument. It is `constexpr` and `noexcept`, so it can be used at compile time and in generic code that takes a convention and needs the number: converting between conventions, computing the correction `(½ − θ)` in Itô's formula, or passing `θ` to an operator that takes a number.

## Parameters

`operators::stochastic::Convention convention`
: `Ito`, `Stratonovich`, `Klimontovich` or `Theta`.

`double theta`
: Used only with `Convention::Theta`; ignored otherwise.

## Returns

The `θ` in `[0, 1]` (for `Theta`, the value you passed, unchecked).

## Practical Examples

### Example 1

The four values: `0`, `0.5`, `1`, and the supplied `θ` for `Theta`. The second argument is ignored for the named conventions.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace stochastic = pluss::operators::stochastic;
namespace sde = pluss::subsp::math::sde;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));
  out.writeln("Ito          :", stochastic::theta_of(stochastic::Convention::Ito));
  out.writeln("Stratonovich :", stochastic::theta_of(stochastic::Convention::Stratonovich));
  out.writeln("Klimontovich :", stochastic::theta_of(stochastic::Convention::Klimontovich));
  out.writeln("Theta, 0.3   :", stochastic::theta_of(stochastic::Convention::Theta, 0.3));
  out.writeln("Ito, 0.3 ignored:", stochastic::theta_of(stochastic::Convention::Ito, 0.3));
}
```

Output

```text
Ito          : 0.00
Stratonovich : 0.50
Klimontovich : 1.00
Theta, 0.3   : 0.30
Ito, 0.3 ignored: 0.00
```

### Example 2

At compile time. The function is `constexpr`, so the values can be checked with `static_assert` or used as template arguments.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace stochastic = pluss::operators::stochastic;
namespace sde = pluss::subsp::math::sde;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));
  static_assert(stochastic::theta_of(stochastic::Convention::Ito) == 0.0);
  static_assert(stochastic::theta_of(stochastic::Convention::Stratonovich) == 0.5);
  static_assert(stochastic::theta_of(stochastic::Convention::Klimontovich) == 1.0);
  static_assert(stochastic::theta_of(stochastic::Convention::Theta, 0.25) == 0.25);
  out.writeln("all four static_asserts hold");
}
```

Output

```text
all four static_asserts hold
```

### Example 3

In generic code. The Itô-formula correction coefficient is `½ − θ`, and the gap between two conventions of `∫ W dW` is `(θ₂ − θ₁)[W]`: Stratonovich minus Itô is `½ [W]`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

namespace stochastic = pluss::operators::stochastic;
namespace sde = pluss::subsp::math::sde;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(2));
  const auto correction = [](stochastic::Convention c, double theta = 0.0) { return 0.5 - stochastic::theta_of(c, theta); };
  out.writeln("Ito formula correction (1/2 - theta): Ito", correction(stochastic::Convention::Ito), " Strat", correction(stochastic::Convention::Stratonovich),
              " Klim", correction(stochastic::Convention::Klimontovich), " theta 0.25:", correction(stochastic::Convention::Theta, 0.25));
  const double gap = stochastic::theta_of(stochastic::Convention::Stratonovich) - stochastic::theta_of(stochastic::Convention::Ito);
  out.writeln("Strat - Ito = gap * [W] with gap =", gap, "  (half the quadratic variation)");
}
```

Output

```text
Ito formula correction (1/2 - theta): Ito 0.50  Strat 0.00  Klim -0.50  theta 0.25: 0.25
Strat - Ito = gap * [W] with gap = 0.50   (half the quadratic variation)
```

## See Also

- [`stochastic_integral`](stochastic_integral.md): uses the convention
- [`stochastic_differential`](stochastic_differential.md): uses θ in the correction term
- [`IntegralOptions`](IntegralOptions.md): holds convention and θ

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="stochastic_integral.html">
    <span class="label">Previous</span>
    <span class="title">stochastic_integral</span>
  </a>
</div>
<!-- apiref:nav:end -->
