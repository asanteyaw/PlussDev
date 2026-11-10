<!-- generated from covariant.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::covariant`

The covariant derivative of a scalar field along one variable: `∂f/∂x + Γ f`.

## Signature

```cpp
operators::Operator operators::covariant(std::string_view variable, math::Function connection, operators::DiffOptions options = {});
```

## What It Does

`covariant("x", Γ)` is the operator `f ↦ ∂f/∂x + Γ(x) f`: the ordinary derivative plus a correction proportional to the field itself. The correction `Γ` is the *connection coefficient* (a function), which says how the reference frame or phase changes from point to point.

This is the covariant derivative of a *scalar section*: a field with a single component, such as a wave function with a local phase or a density weight. With a complex connection it is the gauge-covariant derivative of physics, `D = ∂ − iA`. Its defining property is that it commutes with the gauge transformation: if `ψ → e^{iθ} ψ` and the connection changes accordingly, `Dψ` transforms the same way.

Both the section and the connection must be scalar-valued (rank 0). For a vector or tensor field with several components, use [`geometry::covariant`](../geometry/covariant.md), which takes a connection with one index per tensor axis. The coefficient's variables are matched to the section's by name. The operator holds a function and cannot be saved.

## Parameters

`std::string_view variable`
: The variable to differentiate along.

`math::Function connection`
: The connection coefficient `Γ`, a scalar function whose variables are matched to the section's by name.

`operators::DiffOptions options`
: Non-smooth policy, fallback and size limit for the underlying derivative.

## Returns

An `Operator` whose application returns `∂f/∂x + Γ f`.

## Errors

- The section and the connection must both be scalar-valued: `the built-in connection contract requires scalar-valued sections and coefficients`.
- An undefined connection is refused.

## Practical Examples

### Example 1

A connection coefficient `Γ(x) = 3x` and the section `f = x²`: `∇f = f′ + Γ f = 2x + 3x³`, which is `28` at `x = 2`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x * x; });
  const math::Function gamma = math::function({"x"}, [](math::Function x) { return 3.0 * x; });
  const math::Function nabla = operators::covariant("x", gamma)(f);
  out.writeln("nabla f at 2 =", nabla(2.0).item<double>(), "  (2x + 3x * x^2 = 4 + 24)");
}
```

Output

```text
nabla f at 2 = 28.0000   (2x + 3x * x^2 = 4 + 24)
```

### Example 2

The gauge-covariant derivative `D = ∂ − iA`. A wave function with a local phase, `ψ = e^{iθ(x)}`, in the field `A = θ′(x)` is exactly parallel: `Dψ = 0`. With `θ = x²` the potential is `A = 2x` and the connection coefficient is `−iA`.

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
  const math::Function psi = math::function({"x"}, [&](math::Function x) { return (i * x * x).exp(); });
  const math::Function connection = math::function({"x"}, [&](math::Function x) { return -i * 2.0 * x; });
  const math::Function D_psi = operators::covariant("x", connection)(psi);
  const std::complex<double> value = D_psi(1.3).item<std::complex<double>>();
  const std::complex<double> plain = operators::differential("x")(psi)(1.3).item<std::complex<double>>();
  out.writeln("|D psi| at 1.3     =", std::abs(value), "  (zero: the field is parallel)");
  out.writeln("|d psi/dx| at 1.3  =", std::abs(plain), "  (2x = 2.6)");
}
```

Output

```text
|D psi| at 1.3     = 0.0000   (zero: the field is parallel)
|d psi/dx| at 1.3  = 2.6000   (2x = 2.6)
```

### Example 3

The contract is scalar sections with a scalar connection. A vector-valued section is refused and the message points to the tensor form.

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

  const math::Function vector = math::function({"x"}, [](math::Function x) { return math::stack({x, x * x}); });
  const math::Function gamma = math::function({"x"}, [](math::Function x) { return x; });
  try { (void)operators::covariant("x", gamma)(vector); } catch (const pluss::Error &e) { out.writeln(first_line(e)); }
}
```

Output

```text
operators::covariant: the built-in connection contract requires scalar-valued sections and coefficients
```

## See Also

- [`geometry::covariant`](../geometry/covariant.md): tensor fields and Christoffel connections
- [`differential`](differential.md): the ordinary derivative

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="conjugate_wirtinger.html">
    <span class="label">Previous</span>
    <span class="title">conjugate_wirtinger</span>
  </a>
  <a class="next-link" href="differential.html">
    <span class="label">Next</span>
    <span class="title">differential</span>
  </a>
</div>
<!-- apiref:nav:end -->
