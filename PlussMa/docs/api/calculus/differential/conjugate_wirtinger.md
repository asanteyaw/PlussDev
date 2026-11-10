<!-- generated from conjugate_wirtinger.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::conjugate_wirtinger`

The conjugate Wirtinger derivative `∂/∂z̄`; it vanishes exactly where a function is holomorphic.

## Signature

```cpp
operators::Operator operators::conjugate_wirtinger(std::string_view variable, std::int64_t order = 1, operators::DiffOptions options = {});
```

## What It Does

`conjugate_wirtinger("z")` is the operator `f ↦ ∂f/∂z̄ = ½(∂f/∂x + i ∂f/∂y)`. It measures how much a function depends on the conjugate of `z`. A function is *holomorphic* (complex-differentiable) in `z` exactly where this derivative is zero, so it doubles as the test for holomorphy: apply it and check the result is zero.

Use it to verify that a model respects complex differentiability, to compute the gradient of a real-valued function of a complex variable (for real `f`, the direction of steepest ascent is `2 ∂f/∂z̄`), and in complex optimisation where `∂/∂z` and `∂/∂z̄` are both needed. The same built-in rules as for [`wirtinger`](wirtinger.md) apply.

## Parameters

`std::string_view variable`
: The complex variable.

`std::int64_t order`
: How many times to differentiate. Positive; default 1.

`operators::DiffOptions options`
: Non-smooth policy, fallback and size limit; see `DiffOptions`.

## Returns

An `Operator` whose application returns `∂ⁿf/∂z̄ⁿ` as a `Function`.

## Errors

- A non-positive order is refused.
- A variable the function lacks is refused when applied.

## Practical Examples

### Example 1

Testing holomorphy. `z²` and `exp(z)` depend on `z` only: their `∂/∂z̄` is zero. `z̄`, `Re z` and `|z|²` depend on `z̄`: theirs is not.

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
  const std::complex<double> point{0.5, 1.5};
  const auto test = [&](const char *label, const math::Function &f) {
    const std::complex<double> d = operators::conjugate_wirtinger("z")(f)(point).item<std::complex<double>>();
    out.writeln(label, "|d/dzb| =", std::abs(d), std::abs(d) < 1e-12 ? "  holomorphic" : "  not holomorphic");
  };
  test("z^2    ", math::function({"z"}, [](math::Function z) { return z * z; }));
  test("exp z  ", math::function({"z"}, [](math::Function z) { return z.exp(); }));
  test("conj z ", math::function({"z"}, [](math::Function z) { return z.conj(); }));
  test("Re z   ", math::function({"z"}, [](math::Function z) { return z.real(); }));
  test("|z|^2  ", math::function({"z"}, [](math::Function z) { return z * z.conj(); }));
}
```

Output

```text
z^2     |d/dzb| = 0.0000   holomorphic
exp z   |d/dzb| = 0.0000   holomorphic
conj z  |d/dzb| = 1.0000   not holomorphic
Re z    |d/dzb| = 0.5000   not holomorphic
|z|^2   |d/dzb| = 1.5811   not holomorphic
```

### Example 2

The values of the derivative where it is not zero. `∂(z̄)/∂z̄ = 1`, `∂(Re z)/∂z̄ = ½`, `∂(Im z)/∂z̄ = i/2`.

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
  const std::complex<double> point{0.5, 1.5};
  const auto value = [&](const math::Function &f) { return operators::conjugate_wirtinger("z")(f)(point).item<std::complex<double>>(); };
  const std::complex<double> a = value(math::function({"z"}, [](math::Function z) { return z.conj(); }));
  const std::complex<double> b = value(math::function({"z"}, [](math::Function z) { return z.real(); }));
  const std::complex<double> c = value(math::function({"z"}, [](math::Function z) { return z.imag(); }));
  out.writeln("d conj/dzb =", a.real(), "+", std::abs(a.imag()), "i");
  out.writeln("d Re/dzb   =", b.real(), "+", std::abs(b.imag()), "i");
  out.writeln("d Im/dzb   =", c.real(), c.imag());
}
```

Output

```text
d conj/dzb = 1.0000 + 0.0000 i
d Re/dzb   = 0.5000 + 0.0000 i
d Im/dzb   = 0.0000 0.5000
```

### Example 3

The gradient of a real-valued function of a complex variable. For `f(z) = |z − c|²` the steepest-ascent direction is `2 ∂f/∂z̄ = 2(z − c)`; a gradient-descent step of size 0.25 on that gradient halves the distance to `c`.

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
  const std::complex<double> c{1.0, -1.0};
  const math::Function f = math::function({"z"}, [&](math::Function z) { return (z - c) * (z - c).conj(); });
  std::complex<double> z{3.0, 2.0};
  const math::Function grad = 2.0 * operators::conjugate_wirtinger("z")(f);
  for (int step = 0; step < 4; ++step) {
    const std::complex<double> g = grad(z).item<std::complex<double>>();
    z -= 0.25 * g;
    out.writeln("step", step + 1, ": z =", z.real(), z.imag(), "  |z - c| =", std::abs(z - c));
  }
}
```

Output

```text
step 1 : z = 2.0000 0.5000   |z - c| = 1.8028
step 2 : z = 1.5000 -0.2500   |z - c| = 0.9014
step 3 : z = 1.2500 -0.6250   |z - c| = 0.4507
step 4 : z = 1.1250 -0.8125   |z - c| = 0.2253
```

Each step multiplies the distance by `1 − 2·0.25 = 0.5`.

## See Also

- [`wirtinger`](wirtinger.md): the derivative with respect to `z`
- [`math::Function::conj`](../functions/Function/conj.md): the conjugate

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="covariant.html">
    <span class="label">Next</span>
    <span class="title">covariant</span>
  </a>
</div>
<!-- apiref:nav:end -->
