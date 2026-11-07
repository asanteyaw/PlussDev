<!-- generated from complex_values.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Complex values

[Back to Complex Functions](../calculus_07_complex_functions.md)

A `Function` can carry complex numbers: a complex constant, a complex argument, or a complex result. The same traced body works for real and complex arguments, and a complex scalar can be passed directly.

```cpp
Function math::function(std::complex<double> constant);
Series   Function::operator()(std::complex<double> argument) const;
Function Function::conj() const;   // complex conjugate
Function Function::real() const;   // real part
Function Function::imag() const;   // imaginary part
Function Function::abs() const;    // modulus
Function Function::angle() const;  // principal argument, in (-pi, pi]
```

Write imaginary literals with `std::literals::complex_literals` (`3.0 + 4.0i`). The result of evaluating a complex function is a `Series` of complex dtype; `.item<std::complex<double>>()` reads one value.

## One function, real and complex points

A polynomial and an exponential are written once. At a complex point the same graph gives the complex values; `exp(iπ) + 1` is zero to rounding, which checks the evaluation.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
using namespace pluss::tils;
using namespace std::complex_literals;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function f = math::function({"z"}, [](math::Function z) { return z * z + z.exp(); });

  out.writeln("f(0.5)         ", f(0.5).item<double>());
  out.writeln("f(0.5 - 1.25i) ", f(0.5 - 1.25i).item<std::complex<double>>());
  const std::complex<double> z = 0.5 - 1.25i;
  out.writeln("z*z + exp(z)   ", z * z + std::exp(z));

  const math::Function e = math::function({"z"}, [](math::Function z) { return (z * 1i).exp() + 1.0; });
  out.writeln("exp(i*pi) + 1  ", e(std::acos(-1.0)).item<std::complex<double>>());
}
```

Output

```text
f(0.5)          1.898721
f(0.5 - 1.25i)  (-0.792621,-2.81461)
z*z + exp(z)    (-0.792621,-2.81461)
exp(i*pi) + 1   (0,1.22465e-16)
```

The real argument 0.5 gives a real result, 1.898721. The complex argument gives −0.792621 − 2.81461i, exactly what `z*z + std::exp(z)` computes with the standard library. The last line is Euler's identity: `exp(iπ) + 1` is 1.2 × 10⁻¹⁶ i, rounding error and nothing else. A complex literal combines with a `Function` in `+`, `-`, `*` and `/` like a `double` does.

## Parts of a complex number

`conj`, `real`, `imag`, `abs` and `angle` are recorded like any other node. `real`, `imag`, `abs` and `angle` return real values stored in complex dtype with a zero imaginary part. The program takes 3 + 4i apart and checks the polar form: `abs` times `exp(i·angle)` rebuilds the number.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
using namespace pluss::tils;
using namespace std::complex_literals;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function id = math::function({"z"}, [](math::Function z) { return z; });
  const std::complex<double> z = 3.0 + 4.0i;

  out.writeln("conj  ", id.conj()(z).item<std::complex<double>>());
  out.writeln("real  ", id.real()(z).item<std::complex<double>>());
  out.writeln("imag  ", id.imag()(z).item<std::complex<double>>());
  out.writeln("abs   ", id.abs()(z).item<std::complex<double>>());
  out.writeln("angle ", id.angle()(z).item<std::complex<double>>(), " atan2(4, 3) =", std::atan2(4.0, 3.0));

  const math::Function polar = math::function({"z"}, [](math::Function w) {
    return w.abs() * (w.angle() * 1i).exp();
  });
  out.writeln("abs * exp(i angle) ", polar(z).item<std::complex<double>>());
}
```

Output

```text
conj   (3,-4)
real   (3,0)
imag   (4,0)
abs    (5,0)
angle  (0.927295,0)  atan2(4, 3) = 0.927295
abs * exp(i angle)  (3,4)
```

`conj` flips the sign of the imaginary part, `real` and `imag` return 3 and 4, `abs` returns 5 and `angle` returns 0.927295, the same as `atan2(4, 3)`. Each of the four real-valued parts comes back with a zero imaginary part because the whole graph is complex. `abs * exp(i·angle)` rebuilds 3 + 4i.

## Principal branches

`log`, `sqrt` and a fractional `pow` are multivalued on the complex plane. The library returns the principal value, with the cut along the negative real axis and the argument in (−π, π]. The program evaluates them just above and just below the cut at −4: the two sides differ in the sign of the imaginary part, which is what a branch cut is.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
using namespace pluss::tils;
using namespace std::complex_literals;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function id = math::function({"z"}, [](math::Function z) { return z; });
  const std::complex<double> above = -4.0 + 1e-12i;
  const std::complex<double> below = -4.0 - 1e-12i;

  out.writeln("log  above/below ", id.log()(above).item<std::complex<double>>(), id.log()(below).item<std::complex<double>>());
  out.writeln("sqrt above/below ", id.sqrt()(above).item<std::complex<double>>(), id.sqrt()(below).item<std::complex<double>>());
  out.writeln("pow(z, 0.5) above", id.pow(0.5)(above).item<std::complex<double>>(), " std::sqrt", std::sqrt(above));
  out.writeln("log(-4) = ln 4 + i pi:", std::log(4.0), std::acos(-1.0));
}
```

Output

```text
log  above/below  (1.38629,3.14159) (1.38629,-3.14159)
sqrt above/below  (2.50145e-13,2) (2.50145e-13,-2)
pow(z, 0.5) above (2.50145e-13,2)  std::sqrt (2.50145e-13,2)
log(-4) = ln 4 + i pi: 1.386294 3.141593
```

Just above the cut the logarithm of −4 has imaginary part +π and just below it −π, with real part ln 4 = 1.386294 on both sides. The square root is +2i above and −2i below, and the fractional power `pow(0.5)` agrees with `std::sqrt` on the principal branch. The residual 2.5 × 10⁻¹³ in the real part is the distance 10⁻¹² of the test points from the axis. A function built from these primitives is analytic everywhere except on the cut, which matters for the derivatives of the next page.
