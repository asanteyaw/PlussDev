<!-- generated from real_parameters_and_integrals.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Real parameters and integrals

[Back to Complex Functions](../calculus_07_complex_functions.md)

Many complex functions depend on a *real* parameter: the characteristic function of a distribution, a Fourier integrand, a wave e^{ikx}. For those, the ordinary operators (`differential`, `integral`) apply, and they return complex results.

## Moments from a characteristic function

The characteristic function of a normal variable is φ(t) = exp(iμt − σ²t²/2), and its derivatives at zero give the raw moments: E[Xᵏ] = φ⁽ᵏ⁾(0) / iᵏ. The program differentiates φ four times symbolically, evaluates each derivative at t = 0 and divides by iᵏ. For μ = 1.5 and σ² = 4 the moments are μ, μ² + σ², μ³ + 3μσ² and μ⁴ + 6μ²σ² + 3σ⁴, which the program computes by hand for comparison.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;
using namespace std::complex_literals;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const double mu = 1.5, var = 4.0;
  const math::Function phi = math::function({"t"}, [&](math::Function t) { return (t * 1i * mu - t.pow(2) * (var / 2.0)).exp(); });

  const double m1 = mu, m2 = mu * mu + var, m3 = mu * mu * mu + 3.0 * mu * var, m4 = std::pow(mu, 4) + 6.0 * mu * mu * var + 3.0 * var * var;
  const double exact[] = {m1, m2, m3, m4};
  std::complex<double> power = 1.0;
  for (int k = 1; k <= 4; ++k) {
    power *= 1i;
    const std::complex<double> moment = operators::differential("t", k)(phi)(0.0).item<std::complex<double>>() / power;
    out.writeln("E[X^", k, "] ", moment, " exact", exact[k - 1]);
  }
}
```

Output

```text
E[X^ 1 ]  (1.5,0)  exact 1.500000
E[X^ 2 ]  (6.25,0)  exact 6.250000
E[X^ 3 ]  (21.375,0)  exact 21.375000
E[X^ 4 ]  (107.062,0)  exact 107.062500
```

All four moments equal the closed forms (1.5, 6.25, 21.375 and 107.0625) with a zero imaginary part, as they must for a real random variable. The derivatives are exact symbolic derivatives of the traced graph, evaluated once at t = 0; the scalar `0.0` is passed directly.

## Complex integrands

`integral` accepts a complex integrand and returns a complex value. The integral of e^{ix} from 0 to π is 2i, and the real and imaginary parts are the integrals of cos and sin. The antiderivative evaluated at the bounds gives the same number.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;
using namespace std::complex_literals;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const double pi = std::acos(-1.0);
  const math::Function wave = math::function({"x"}, [](math::Function x) { return (x * 1i).exp(); });

  out.writeln("integral of exp(ix) over [0, pi] ", operators::integral("x", 0.0, pi)(wave)().item<std::complex<double>>(), " exact (0,2)");
  const math::Function cosine = math::function({"x"}, [](math::Function x) { return x.cos(); });
  out.writeln("real part is the integral of cos ", operators::integral("x", 0.0, pi)(cosine)().item<double>(), " exact 0");
  const math::Function quarter = operators::integral("x", 0.0, pi / 2.0)(wave);
  out.writeln("over [0, pi/2] ", quarter().item<std::complex<double>>(), " exact (1,1)");
}
```

Output

```text
integral of exp(ix) over [0, pi]  (-1.85929e-16,2)  exact (0,2)
real part is the integral of cos  0.00000000  exact 0
over [0, pi/2]  (1,1)  exact (1,1)
```

The integral of e^{ix} over [0, π] is 2i up to a rounding error of 2 × 10⁻¹⁶ in the real part, the integral of cos over the same interval is zero, and over [0, π/2] the value is 1 + i. The result of `integral` is itself a `Function`; calling it with no arguments evaluates it.

## Saving a complex function

A complex function persists like a real one, and the restored function gives the same values.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <sstream>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;
using namespace std::complex_literals;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const math::Function f = math::function({"z"}, [](math::Function z) { return z.pow(3.0) + z.conj() * z; });
  const math::Function df = operators::wirtinger("z")(f);
  std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
  math::io::save(df, stream);
  const math::Function restored = math::io::load_function(stream);
  const std::complex<double> z = 0.4 + 0.9i;
  out.writeln("original ", df(z).item<std::complex<double>>(), " restored", restored(z).item<std::complex<double>>(), " by hand", 3.0 * z * z + std::conj(z));
}
```

Output

```text
original  (-1.55,1.26)  restored (-1.55,1.26)  by hand (-1.55,1.26)
```

The saved and restored derivative give the same value, −1.55 + 1.26i, which is also 3z² + z̄ computed by hand.
