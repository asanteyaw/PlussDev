<!-- generated from fractional_calculus.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Fractional calculus

[Back to Operators](../calculus_05_operators.md)

Fractional operators extend the order of an integral or a derivative to any `α > 0`. The result is a `Function` of the evaluation point `x`, measured from a lower terminal (0 unless you give one). Polynomials get exact power-law formulas; any other function becomes an integral that is evaluated when you supply a value.

| Entry point | Definition |
|---|---|
| `fractional::integral(x, α, lower)` | `I^α f(x) = 1/Γ(α) ∫ₐˣ (x−t)^{α−1} f(t) dt` |
| `fractional::differential(x, α, kind, lower)` | `RiemannLiouville`: `dᵐ/dxᵐ I^{m−α} f`; `Caputo`: `I^{m−α} f⁽ᵐ⁾`; `GrunwaldLetnikov`: equal to Riemann–Liouville on the functions both are defined for (`m = ⌈α⌉`) |

```cpp
operators::Operator operators::fractional::integral(std::string_view variable, double alpha, double lower = 0.0);
operators::Operator operators::fractional::differential(std::string_view variable, double alpha, Kind kind = Kind::Caputo, double lower = 0.0);
```

An integer order is the ordinary derivative. The kinds differ on constants: the Riemann–Liouville derivative of 1 is `x^{−α}/Γ(1−α)`, the Caputo derivative is 0. Both operators save and load.

## Power laws and the semigroup

`D^{1/2} x = 2√(x/π)`; `I^{1/2} x² = Γ(3)/Γ(7/2) x^{5/2}`; and integrals compose by adding orders, `I^{0.3} I^{0.4} = I^{0.7}`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace math = pluss::math;
namespace fractional = pluss::operators::fractional;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const math::Function x = math::function({"x"}, [](math::Function v) { return v; });
  const math::Function one = math::function({"x"}, [](math::Function v) { return v * 0.0 + 1.0; });
  const auto at = pluss::make_series(2.0);
  const double pi = std::acos(-1.0);

  const auto RL = fractional::differential("x", 0.5, fractional::Kind::RiemannLiouville);   // staged
  const auto Caputo = fractional::differential("x", 0.5);
  out.writeln("RL D^1/2 x       ", RL(x)({at}).item<double>(), "   2 sqrt(x/pi) = ", 2 * std::sqrt(2.0 / pi));
  out.writeln("RL D^1/2 of 1    ", RL(one)({at}).item<double>(), "   x^-1/2 / Gamma(1/2) = ", std::pow(2.0, -0.5) / std::tgamma(0.5));
  out.writeln("Caputo of 1      ", Caputo(one)({at}).item<double>());
  out.writeln("I^1/2 x^2        ", fractional::integral("x", 0.5)(x * x)({at}).item<double>(), "   exact ", 2.0 / std::tgamma(3.5) * std::pow(2.0, 2.5));
  out.writeln("I^.3 I^.4 x      ", fractional::integral("x", 0.3)(fractional::integral("x", 0.4)(x))({at}).item<double>(), "   exact ", std::pow(2.0, 1.7) / std::tgamma(2.7));
}
```

Output

```text
RL D^1/2 x        1.59576912    2 sqrt(x/pi) =  1.59576912
RL D^1/2 of 1     0.39894228    x^-1/2 / Gamma(1/2) =  0.39894228
Caputo of 1       0.00000000
I^1/2 x^2         3.40430746    exact  3.40430746
I^.3 I^.4 x       2.10334651    exact  2.10334651
```

## A function with no closed form

For `eˣ` the Riemann–Liouville integral is the Mittag-Leffler series `Σ x^{k+α}/Γ(k+α+1)`; the program evaluates the lazy integral and the series side by side.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace math = pluss::math;
namespace fractional = pluss::operators::fractional;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(7));
  const math::Function e = math::function({"x"}, [](math::Function v) { return v.exp(); });
  double series = 0.0;
  for (int k = 0; k < 60; ++k) series += std::pow(1.3, k + 0.5) / std::tgamma(k + 1.5);
  out.writeln("I^1/2 exp(x) at 1.3  ", fractional::integral("x", 0.5)(e)({pluss::make_series(1.3)}).item<double>(), "   series ", series);
}
```

Output

```text
I^1/2 exp(x) at 1.3   3.2771820    series  3.2771820
```

<!-- apiref:toctree:start -->
<!-- apiref:toctree:end -->
