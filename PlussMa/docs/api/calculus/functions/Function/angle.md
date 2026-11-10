<!-- generated from angle.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 29c9ea0f3976 -->

# `math::Function::angle`

[Back to `math::Function`](../Function.md)

The argument (phase angle) of a complex function, in radians in `(−π, π]`.

## Signature

```cpp
math::Function math::Function::angle() const;
```

## What It Does

`f.angle()` builds `x ↦ arg f(x)`, the angle the point `f(x)` makes with the positive real axis, measured in radians. It is the principal value, in `(−π, π]`: the negative real axis maps to `π`, and an angle that goes past `π` wraps around to `−π`. For a positive real number the angle is 0 and for a negative real number it is `π`. The result is real-valued.

Together with [`abs`](abs.md) it gives the polar form `z = |z| e^{i arg z}`. The angle is undefined at `z = 0`, so differentiating `angle` at 0 follows the `NonSmooth` rule of the differential operator, as for `abs`. Away from 0 the derivative with respect to a real parameter is `Im(z̄ z′)/|z|²`.

## Returns

A new real-valued `Function` with values in `(−π, π]`.

## Errors

- The function must be defined.
- Differentiating at 0 under `NonSmooth::Strict` raises `the derivative is undefined at a zero denominator`.

## Practical Examples

### Example 1

The angle of a number in each quadrant and on the axes: `1 → 0`, `i → π/2`, `−1 → π`, `−i → −π/2`, `1 + i → π/4`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"z"}, [](math::Function z) { return z.angle(); });
  const auto z = pluss::make_series({std::complex<double>{1.0, 0.0}, std::complex<double>{0.0, 1.0}, std::complex<double>{-1.0, 0.0},
                                     std::complex<double>{0.0, -1.0}, std::complex<double>{1.0, 1.0}, std::complex<double>{-1.0, -1.0}});
  out.writeln("arg z:        ", f({z}).items<double>());
  out.writeln("in units of pi:", (f({z}) / std::acos(-1.0)).items<double>());
}
```

Output

```text
arg z:         [0.0000, 1.5708, 3.1416, -1.5708, 0.7854, -2.3562]
in units of pi: [0.0000, 0.5000, 1.0000, -0.5000, 0.2500, -0.7500]
```

### Example 2

The principal branch. The angle of `e^{it}` equals `t` only while `t` stays in `(−π, π]`; past `π` it wraps by `2π`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>
#include <complex>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const std::complex<double> i{0.0, 1.0};
  const math::Function f = math::function({"t"}, [&](math::Function t) { return (i * t).exp().angle(); });
  const auto t = pluss::make_series({1.0, 3.0, 4.0, -4.0, 7.0});
  out.writeln("t:                ", t.items<double>());
  out.writeln("arg e^{it}:       ", f({t}).items<double>());
  out.writeln("t wrapped by 2 pi:", pluss::make_series({1.0, 3.0, 4.0 - 2.0 * std::acos(-1.0), -4.0 + 2.0 * std::acos(-1.0), 7.0 - 2.0 * std::acos(-1.0)}).items<double>());
}
```

Output

```text
t:                 [1.0000, 3.0000, 4.0000, -4.0000, 7.0000]
arg e^{it}:        [1.0000, 3.0000, -2.2832, 2.2832, 0.7168]
t wrapped by 2 pi: [1.0000, 3.0000, -2.2832, 2.2832, 0.7168]
```

### Example 3

The derivative of the phase. For `z(t) = (1 + t) + i t`, `d/dt arg z = (x y′ − y x′)/|z|²`, which is `1/(2t² + 2t + 1)` here; evaluated at `t = 1` that is `1/5`.

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
  const math::Function phase = math::function({"t"}, [&](math::Function t) { return ((1.0 + t) + i * t).angle(); });
  const math::Function rate = operators::differential("t")(phase);
  out.writeln("arg z(1)   =", phase(1.0).item<double>());
  out.writeln("d/dt arg   =", rate(1.0).item<double>());
  out.writeln("1/5        =", 1.0 / 5.0);
}
```

Output

```text
arg z(1)   = 0.4636
d/dt arg   = 0.2000
1/5        = 0.2000
```

## See Also

- [`abs`](abs.md): the modulus
- [`real`](real.md): the real part
- [`imag`](imag.md): the imaginary part

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="add.html">
    <span class="label">Previous</span>
    <span class="title">add</span>
  </a>
  <a class="next-link" href="arg.html">
    <span class="label">Next</span>
    <span class="title">arg</span>
  </a>
</div>
<!-- apiref:nav:end -->
