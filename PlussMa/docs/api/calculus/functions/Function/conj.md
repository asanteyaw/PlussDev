<!-- generated from conj.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d0235a4020f6 -->

# `math::Function::conj`

[Back to `math::Function`](../Function.md)

The complex conjugate of a function: the same real part and the opposite imaginary part.

## Signature

```cpp
math::Function math::Function::conj() const;
```

## What It Does

`f.conj()` builds `x ↦ conj f(x)`, which flips the sign of the imaginary part: `conj(a + bi) = a − bi`. For a real function it is the function itself. The conjugate is the operation behind squared magnitudes, since `z · conj(z) = |z|²`, and behind inner products and Hermitian forms.

The conjugate is *not* complex-differentiable: `conj(z)` depends on `z̄`, not `z`. In [Wirtinger calculus](../../differential/wirtinger.md) that is expressed exactly: `∂/∂z conj(z) = 0` and `∂/∂z̄ conj(z) = 1`. The rules for `conj` are built in, so differentiating an expression that contains it works.

## Returns

A new `Function` whose value is the complex conjugate.

## Errors

- The function must be defined.

## Practical Examples

### Example 1

Conjugating a few complex numbers. A real input is unchanged.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"z"}, [](math::Function z) { return z.conj(); });
  const auto z = pluss::make_series({std::complex<double>{1.0, 2.0}, std::complex<double>{-3.0, -0.5}, std::complex<double>{4.0, 0.0}});
  out.writeln("z:      real", z.real().items<double>(), " imag", z.imag().items<double>());
  out.writeln("conj z: real", f({z}).real().items<double>(), " imag", f({z}).imag().items<double>());
}
```

Output

```text
z:      real [1.0000, -3.0000, 4.0000]  imag [2.0000, -0.5000, 0.0000]
conj z: real [1.0000, -3.0000, 4.0000]  imag [-2.0000, 0.5000, -0.0000]
```

### Example 2

`z · conj(z) = |z|²`: the product is real and equals the squared modulus. The imaginary part of the product is exactly zero.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function product = math::function({"z"}, [](math::Function z) { return z * z.conj(); });
  const math::Function modulus = math::function({"z"}, [](math::Function z) { return z.abs(); });
  const auto z = pluss::make_series({std::complex<double>{3.0, 4.0}, std::complex<double>{1.0, -1.0}});
  out.writeln("z conj(z): real", product({z}).real().items<double>(), "imag", product({z}).imag().items<double>());
  out.writeln("|z|^2:         ", (modulus({z}) * modulus({z})).items<double>());
}
```

Output

```text
z conj(z): real [25.0000, 2.0000] imag [0.0000, 0.0000]
|z|^2:          [25.0000, 2.0000]
```

### Example 3

Wirtinger derivatives of the conjugate at `z = 1 + 2i`: `conj` has no dependence on `z`, so `∂/∂z` is 0, and depends on `z̄` with slope 1, so `∂/∂z̄` is 1.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <complex>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"z"}, [](math::Function z) { return z.conj(); });
  const std::complex<double> point{1.0, 2.0};
  const std::complex<double> dz = operators::wirtinger("z")(f)(point).item<std::complex<double>>();
  const std::complex<double> dzbar = operators::conjugate_wirtinger("z")(f)(point).item<std::complex<double>>();
  out.writeln("|d/dz conj(z)|     =", std::abs(dz));
  out.writeln("|d/dzb conj(z) - 1|=", std::abs(dzbar - 1.0));
}
```

Output

```text
|d/dz conj(z)|     = 0.0000
|d/dzb conj(z) - 1|= 0.0000
```

## See Also

- [`real`](real.md): the real part
- [`imag`](imag.md): the imaginary part
- [`abs`](abs.md): the modulus
- [`operators::wirtinger`](../../differential/wirtinger.md): complex derivatives

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="compose.html">
    <span class="label">Previous</span>
    <span class="title">compose</span>
  </a>
  <a class="next-link" href="contract.html">
    <span class="label">Next</span>
    <span class="title">contract</span>
  </a>
</div>
<!-- apiref:nav:end -->
