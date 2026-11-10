<!-- generated from convolution.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::convolution`

The convolution of a function with a fixed kernel over the real line: `(f ∗ k)(x) = ∫ f(t) k(x − t) dt`.

## Signature

```cpp
operators::Operator operators::convolution(std::string_view variable, math::Function kernel);
```

## What It Does

`convolution("x", k)` is the operator `f ↦ f ∗ k`, where `k` is a function of the same variable name. The result is a function of `x`: at each `x`, the kernel slid to `x` and flipped, multiplied by `f` and integrated over the whole real line.

Convolution is how smoothing, filtering and the distribution of a sum are written. Convolving with a bell-shaped kernel smooths; convolving two densities gives the density of the sum of independent variables; the response of a linear time-invariant system is the input convolved with its impulse response. Convolution is commutative and associative, and it moves a derivative onto either factor: `(f ∗ k)′ = f′ ∗ k = f ∗ k′`.

The integral over the whole line has no closed form in general, so the result is an unresolved integral evaluated numerically when values arrive (`Quadrature`); it differentiates and composes like any function. Both factors must decay fast enough for the integral to converge, or the evaluation fails with a convergence error rather than returning a wrong number. The kernel's variable is matched to the input's by name. The operator holds a function and cannot be saved.

## Parameters

`std::string_view variable`
: The variable of the convolution. Both the input and the kernel must be functions of it.

`math::Function kernel`
: The kernel `k`, a function of the same variable name as the input.

## Returns

An `Operator` whose application returns `f ∗ k` as a `Function` of the variable.

## Errors

- A non-convergent integral raises `unresolved integral quadrature did not converge`.
- A kernel whose variable the input lacks raises `cannot rebind variable '<name>'`.

## Practical Examples

### Example 1

Two Gaussians. The convolution of `exp(−x²)` with itself is `√(π/2) exp(−x²/2)`; the numerical value matches the closed form.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function gauss = math::function({"x"}, [](math::Function x) { return (-(x * x)).exp(); });
  const math::Function smoothed = operators::convolution("x", gauss)(gauss);
  const auto x = pluss::make_series({0.0, 0.5, 1.5});
  out.writeln("convolution: ", smoothed({x}).items<double>());
  out.writeln("closed form: ", (std::sqrt(std::acos(-1.0) / 2.0) * (-(x * x) / 2.0).exp()).items<double>());
}
```

Output

```text
convolution:  [1.253314, 1.106046, 0.406892]
closed form:  [1.253314, 1.106046, 0.406892]
```

### Example 2

Commutativity and the derivative rule. `f ∗ k = k ∗ f`, and `(f ∗ k)′ = f′ ∗ k`: the derivative can be moved onto either factor.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function f = math::function({"x"}, [](math::Function x) { return (-(x * x)).exp(); });
  const math::Function k = math::function({"x"}, [](math::Function x) { return (-(x * x) / 4.0).exp(); });
  const math::Function f_k = operators::convolution("x", k)(f);
  const math::Function k_f = operators::convolution("x", f)(k);
  const math::Function d_f = operators::differential("x")(f);
  out.writeln("f * k at 0.7:     ", f_k(0.7).item<double>());
  out.writeln("k * f at 0.7:     ", k_f(0.7).item<double>());
  out.writeln("(f * k)' at 0.7:  ", operators::differential("x")(f_k)(0.7).item<double>());
  out.writeln("f' * k at 0.7:    ", operators::convolution("x", k)(d_f)(0.7).item<double>());
}
```

Output

```text
f * k at 0.7:      1.437339
k * f at 0.7:      1.437339
(f * k)' at 0.7:   -0.402455
f' * k at 0.7:     -0.402455
```

### Example 3

Smoothing a step. Convolving a steep logistic step with a narrow Gaussian gives a smoother step, and the value at the centre stays one half by symmetry. The result is `Quadrature`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

static const char *name(math::Provenance p) {
  switch (p) {
  case math::Provenance::Structural: return "Structural";
  case math::Provenance::Opaque: return "Opaque";
  case math::Provenance::Quadrature: return "Quadrature";
  case math::Provenance::Autodiff: return "Autodiff";
  }
  return "?";
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function step = math::function({"x"}, [](math::Function x) { return 1.0 / (1.0 + (-(20.0 * x)).exp()); });
  const math::Function bump = math::function({"x"}, [](math::Function x) { return (-(x * x) * 50.0).exp() * std::sqrt(50.0 / std::acos(-1.0)); });
  const math::Function smooth = operators::convolution("x", bump)(step);
  const auto x = pluss::make_series({-0.2, 0.0, 0.2});
  out.writeln("step:   ", step({x}).items<double>());
  out.writeln("smooth: ", smooth({x}).items<double>(), "  ", name(smooth.provenance()));
}
```

Output

```text
step:    [0.0180, 0.5000, 0.9820]
smooth:  [0.0677, 0.5000, 0.9323]    Quadrature
```

### Example 4

Things that cannot work are reported rather than computed: convolving two functions that do not decay (`exp(−x)` grows as `x → −∞`, so the integral diverges) fails to converge, and a kernel in a different variable cannot be matched.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto first_line = [](const pluss::Error &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };

  const math::Function growing = math::function({"x"}, [](math::Function x) { return (-x).exp(); });
  const math::Function diverging = operators::convolution("x", growing)(growing);
  try { (void)diverging(1.0); } catch (const pluss::Error &e) { out.writeln("diverging integral:", first_line(e)); }
  const math::Function gauss = math::function({"x"}, [](math::Function x) { return (-(x * x)).exp(); });
  const math::Function other_variable = math::function({"t"}, [](math::Function t) { return t; });
  try { (void)operators::convolution("x", other_variable)(gauss); } catch (const pluss::Error &e) { out.writeln("kernel in t:       ", first_line(e)); }
}
```

Output

```text
diverging integral: math::Function: unresolved integral quadrature did not converge; error estimate=nan
kernel in t:        math::Function: cannot rebind variable 't' onto the target Function
```

## See Also

- [`integral`](integral.md): integration over a range
- [`fourier::transform`](../fourier/transform.md): turns convolution into multiplication
- [`laplace::transform`](../laplace/transform.md): the one-sided version

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="antiderivative.html">
    <span class="label">Previous</span>
    <span class="title">antiderivative</span>
  </a>
  <a class="next-link" href="integral.html">
    <span class="label">Next</span>
    <span class="title">integral</span>
  </a>
</div>
<!-- apiref:nav:end -->
