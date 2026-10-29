<!-- generated from wirtinger_derivatives.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Wirtinger derivatives

[Back to Complex Functions](../calculus_07_complex_functions.md)

A function of a complex variable z = x + iy has two derivatives, not one: the Wirtinger derivatives ∂/∂z and ∂/∂z̄, which treat z and its conjugate as independent. A function is holomorphic (complex-differentiable) exactly where ∂f/∂z̄ = 0, and then ∂f/∂z is the ordinary complex derivative.

```cpp
Operator operators::wirtinger(std::string_view variable, std::int64_t order = 1, DiffOptions options = {});
Operator operators::conjugate_wirtinger(std::string_view variable, std::int64_t order = 1, DiffOptions options = {});
```

Both return a new `Function` that is evaluated at complex points.

## Holomorphic functions

For a function built from polynomials, `exp`, `sin` and the other analytic primitives, ∂/∂z̄ vanishes and ∂/∂z is the usual derivative. The program compares ∂/∂z of z³ + eᶻ + sin z with 3z² + eᶻ + cos z at two points, and prints the conjugate derivative.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;
using namespace std::complex_literals;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function f = math::function({"z"}, [](math::Function z) { return z.pow(3.0) + z.exp() + z.sin(); });
  const math::Function dz = operators::wirtinger("z")(f);
  const math::Function dzbar = operators::conjugate_wirtinger("z")(f);

  for (std::complex<double> z : {0.3 - 0.4i, 1.2 + 0.5i}) {
    out.writeln("z", z, " d/dz", dz(z).item<std::complex<double>>(), " by hand", 3.0 * z * z + std::exp(z) + std::cos(z), " d/dzbar", dzbar(z).item<std::complex<double>>());
  }
}
```

Output

```text
z (0.3,-0.4)  d/dz (2.06609,-1.12427)  by hand (2.06609,-1.12427)  d/dzbar (0,0)
z (1.2,0.5)  d/dz (6.89228,4.70607)  by hand (6.89228,4.70607)  d/dzbar (0,0)
```

∂/∂z agrees with 3z² + eᶻ + cos z to the printed digits at both points, and ∂/∂z̄ is exactly zero: the graph contains only analytic primitives, so it is holomorphic.

## Functions that are not holomorphic

`conj`, `real`, `imag`, `abs` and `angle` are not holomorphic, and the two derivatives separate. At z the rules are:

| f | ∂f/∂z | ∂f/∂z̄ |
|---|---|---|
| `z.conj()` | 0 | 1 |
| `z.real()` | 1/2 | 1/2 |
| `z.imag()` | −i/2 | i/2 |
| `z.abs()` | z̄ / (2\|z\|) | z / (2\|z\|) |
| `z.angle()` | −i / (2z) | i / (2z̄) |

The program evaluates each at z = 3 + 4i and prints the table value beside it.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;
using namespace std::complex_literals;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function z = math::function({"z"}, [](math::Function v) { return v; });
  const std::complex<double> p = 3.0 + 4.0i;
  const double r = std::abs(p);
  auto show = [&](const char *name, const math::Function &f, std::complex<double> dz, std::complex<double> dzbar) {
    out.writeln(name, operators::wirtinger("z")(f)(p).item<std::complex<double>>(), dz, operators::conjugate_wirtinger("z")(f)(p).item<std::complex<double>>(), dzbar);
  };
  out.writeln("          d/dz computed, table      d/dzbar computed, table");
  show("conj  ", z.conj(), 0.0, 1.0);
  show("real  ", z.real(), 0.5, 0.5);
  show("imag  ", z.imag(), -0.5i, 0.5i);
  show("abs   ", z.abs(), std::conj(p) / (2.0 * r), p / (2.0 * r));
  show("angle ", z.angle(), -1i / (2.0 * p), 1i / (2.0 * std::conj(p)));
}
```

Output

```text
          d/dz computed, table      d/dzbar computed, table
conj   (0,-0) (0,0) (1,-0) (1,0)
real   (0.5,0) (0.5,0) (0.5,0) (0.5,0)
imag   (0,-0.5) (-0,-0.5) (0,0.5) (0,0.5)
abs    (0.3,-0.4) (0.3,-0.4) (0.3,0.4) (0.3,0.4)
angle  (-0.08,-0.06) (-0.08,-0.06) (-0.08,0.06) (-0.08,0.06)
```

Every entry matches the table, signs of zero aside. The conjugate has ∂/∂z = 0 and ∂/∂z̄ = 1, which is why it is the standard example of a function that is nowhere holomorphic. For `abs` and `angle` the derivative is a complex number of modulus 1/2 and 1/(2\|z\|) respectively, the Wirtinger form of the gradient of the modulus and of the phase.

## Cauchy–Riemann and the real derivatives

For f = u + iv, the two derivatives combine the partial derivatives in x and y: ∂f/∂z = ½(f_x − i f_y) and ∂f/∂z̄ = ½(f_x + i f_y). So a function is holomorphic exactly when f_x = −i f_y, which is the Cauchy–Riemann system. The program checks both identities for the non-holomorphic f = z·z̄ + z² by taking the real partial derivatives with central differences.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;
using namespace std::complex_literals;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const math::Function f = math::function({"z"}, [](math::Function z) { return z * z.conj() + z * z; });
  const std::complex<double> p = 0.7 - 0.2i;
  const double h = 1e-5;
  auto value = [&](std::complex<double> w) { return f(w).item<std::complex<double>>(); };
  const std::complex<double> fx = (value(p + h) - value(p - h)) / (2.0 * h);
  const std::complex<double> fy = (value(p + 1i * h) - value(p - 1i * h)) / (2.0 * h);

  out.writeln("d/dz     ", operators::wirtinger("z")(f)(p).item<std::complex<double>>(), " (fx - i fy)/2", 0.5 * (fx - 1i * fy));
  out.writeln("d/dzbar  ", operators::conjugate_wirtinger("z")(f)(p).item<std::complex<double>>(), " (fx + i fy)/2", 0.5 * (fx + 1i * fy));
  out.writeln("by hand  ", 2.0 * p + std::conj(p), p);
}
```

Output

```text
d/dz      (2.1,-0.2)  (fx - i fy)/2 (2.1,-0.2)
d/dzbar   (0.7,-0.2)  (fx + i fy)/2 (0.7,-0.2)
by hand   (2.1,-0.2) (0.7,-0.2)
```

The Wirtinger derivatives of z z̄ + z² are 2z + z̄ and z, and both agree with the combinations of the finite-difference partial derivatives to eight decimals. These two identities are the definition of the operators in terms of ordinary calculus in x and y.

## Higher orders

`order` repeats the derivative. For the holomorphic z⁵ the second `wirtinger` derivative is 20z³.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;
using namespace std::complex_literals;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function f = math::function({"z"}, [](math::Function z) { return z.pow(5.0); });
  const std::complex<double> p = 0.5 + 0.5i;
  out.writeln("second derivative", operators::wirtinger("z", 2)(f)(p).item<std::complex<double>>(), " 20 z^3", 20.0 * p * p * p);
  out.writeln("conjugate, order 2", operators::conjugate_wirtinger("z", 2)(f)(p).item<std::complex<double>>());
}
```

Output

```text
second derivative (-5,5)  20 z^3 (-5,5)
conjugate, order 2 (0,0)
```

The second ∂/∂z derivative of z⁵ is 20z³ = −5 + 5i at 0.5 + 0.5i, and the second conjugate derivative is exactly zero.
