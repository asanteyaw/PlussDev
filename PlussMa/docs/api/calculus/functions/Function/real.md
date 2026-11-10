<!-- generated from real.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bef9e479987b -->

# `math::Function::real`

[Back to `math::Function`](../Function.md)

The real part of a function.

## Signature

```cpp
math::Function math::Function::real() const;
```

## What It Does

`f.real()` builds `x ↦ Re f(x)`. The result is a real-valued function even when `f` is complex; for a real `f` it is `f`. Use it to extract the physical, observable or plotted quantity from a complex-valued model: the real part of a phasor, of a characteristic function, of a transform.

It is a linear, real-valued, non-holomorphic operation: `Re z = (z + z̄)/2`. Its Wirtinger derivatives are `∂/∂z Re z = ½` and `∂/∂z̄ Re z = ½`.

## Returns

A new real-valued `Function`.

## Errors

- The function must be defined.

## Practical Examples

### Example 1

The real parts of a few complex numbers; a real number is its own real part.

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
  const math::Function f = math::function({"z"}, [](math::Function z) { return z.real(); });
  const auto z = pluss::make_series({std::complex<double>{3.0, 4.0}, std::complex<double>{-1.5, 2.0}, std::complex<double>{2.0, 0.0}});
  out.writeln("Re z:", f({z}).items<double>());
}
```

Output

```text
Re z: [3.0000, -1.5000, 2.0000]
```

### Example 2

`Re(z²) = x² − y²` for `z = x + iy`, checked on three points.

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
  const math::Function f = math::function({"z"}, [](math::Function z) { return (z * z).real(); });
  const auto z = pluss::make_series({std::complex<double>{1.0, 2.0}, std::complex<double>{3.0, -1.0}, std::complex<double>{0.5, 0.5}});
  out.writeln("Re(z^2):    ", f({z}).items<double>());
  out.writeln("x^2 - y^2:  ", pluss::make_series({1.0 - 4.0, 9.0 - 1.0, 0.25 - 0.25}).items<double>());
}
```

Output

```text
Re(z^2):     [-3.0000, 8.0000, 0.0000]
x^2 - y^2:   [-3.0000, 8.0000, 0.0000]
```

### Example 3

The real part of a phasor `e^{it}` is `cos t`; differentiating with respect to the real parameter `t` gives `−sin t`.

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
  const std::complex<double> i{0.0, 1.0};
  const math::Function phasor = math::function({"t"}, [&](math::Function t) { return (i * t).exp().real(); });
  const math::Function slope = operators::differential("t")(phasor);
  const auto t = pluss::make_series({0.5, 1.0, 2.0});
  out.writeln("Re e^{it}:    ", phasor({t}).items<double>());
  out.writeln("cos t:        ", t.cos().items<double>());
  out.writeln("d/dt Re e^{it}:", slope({t}).items<double>());
  out.writeln("-sin t:       ", (-t.sin()).items<double>());
}
```

Output

```text
Re e^{it}:     [0.8776, 0.5403, -0.4161]
cos t:         [0.8776, 0.5403, -0.4161]
d/dt Re e^{it}: [-0.4794, -0.8415, -0.9093]
-sin t:        [-0.4794, -0.8415, -0.9093]
```

## See Also

- [`imag`](imag.md): the imaginary part
- [`conj`](conj.md): the conjugate
- [`abs`](abs.md): the modulus

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rank.html">
    <span class="label">Previous</span>
    <span class="title">rank</span>
  </a>
  <a class="next-link" href="select.html">
    <span class="label">Next</span>
    <span class="title">select</span>
  </a>
</div>
<!-- apiref:nav:end -->
