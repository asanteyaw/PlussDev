<!-- generated from laplacian.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::laplacian`

The Laplacian of a scalar function: `Σ ∂²f/∂xᵢ²`.

## Signature

```cpp
operators::Operator operators::laplacian(pluss::StackArray<std::string_view> variables, operators::DiffOptions options = {});
```

## What It Does

`laplacian({"x", "y"})` is the operator `f ↦ ∂²f/∂x² + ∂²f/∂y²`. It measures how much the value at a point differs from the average of its neighbours: positive at a local minimum, negative at a maximum, zero for *harmonic* functions. It appears in the heat equation `u_t = κ∇²u`, the wave equation, Poisson's equation, the Schrödinger equation and in the diffusion part of the generator of Brownian motion. The Laplacian is the divergence of the gradient and the trace of the Hessian.

The input must be scalar-valued. `variables` names the coordinates to sum over; other variables of the function (a parameter, time) are left alone.

## Parameters

`pluss::StackArray<std::string_view> variables`
: The coordinates to sum the second derivatives over.

`operators::DiffOptions options`
: Non-smooth policy, fallback and size limit.

## Returns

An `Operator` whose application returns the Laplacian as a scalar `Function`.

## Errors

- `the function must be scalar-valued`.

## Practical Examples

### Example 1

The Laplacian of `f = x² + y³ + z` in three coordinates is `2 + 6y`, which is `14` at `(1, 2, 3)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x", "y", "z"}, [](math::Function x, math::Function y, math::Function z) { return x * x + y * y * y + z; });
  const math::Function lap = operators::laplacian({"x", "y", "z"})(f);
  out.writeln("laplacian at (1, 2, 3) =", lap({pluss::make_series(1.0), pluss::make_series(2.0), pluss::make_series(3.0)}).item<double>(), "  (2 + 6 y)");
}
```

Output

```text
laplacian at (1, 2, 3) = 14.0000   (2 + 6 y)
```

### Example 2

Harmonic functions have zero Laplacian: `x² − y²`, `x y`, and the two-dimensional logarithmic potential `½ log(x² + y²)` away from the origin.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const auto lap = operators::laplacian({"x", "y"});
  const auto point = {pluss::make_series(0.7), pluss::make_series(-1.3)};
  out.writeln("x^2 - y^2:        ", lap(math::function({"x", "y"}, [](math::Function x, math::Function y) { return x * x - y * y; }))(point).item<double>());
  out.writeln("x y:              ", lap(math::function({"x", "y"}, [](math::Function x, math::Function y) { return x * y; }))(point).item<double>());
  out.writeln("0.5 log(x^2+y^2): ", lap(math::function({"x", "y"}, [](math::Function x, math::Function y) { return 0.5 * (x * x + y * y).log(); }))(point).item<double>());
}
```

Output

```text
x^2 - y^2:         0.0000000000
x y:               0.0000000000
0.5 log(x^2+y^2):  0.0000000000
```

### Example 3

Other variables are left alone: the Laplacian in `(x, y)` of a function of `(x, y, t)` is still a function of `t`. The two-dimensional heat kernel `exp(−r²/(4t))/(4πt)` satisfies the heat equation `u_t = ∇²u`, so its Laplacian equals its time derivative.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const double pi = std::acos(-1.0);
  const math::Function u = math::function({"x", "y", "t"}, [&](math::Function x, math::Function y, math::Function t) { return (-(x * x + y * y) / (4.0 * t)).exp() / (4.0 * pi * t); });
  const math::Function lap = operators::laplacian({"x", "y"})(u);
  const math::Function u_t = operators::differential("t")(u);
  const auto point = {pluss::make_series(0.8), pluss::make_series(0.3), pluss::make_series(0.5)};
  out.writeln("variables of the result:", lap.arity());
  out.writeln("laplacian u:", lap(point).item<double>(), "  u_t:", u_t(point).item<double>());
}
```

Output

```text
variables of the result: 3
laplacian u: -0.14031573   u_t: -0.14031573
```

### Example 4

The Laplacian is the divergence of the gradient (and the trace of the Hessian): the three constructions agree.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const math::Function f = math::function({"x", "y"}, [](math::Function x, math::Function y) { return (x * y).sin() + x * x * y; });
  const auto point = {pluss::make_series(0.9), pluss::make_series(1.1)};
  out.writeln("laplacian:       ", operators::laplacian({"x", "y"})(f)(point).item<double>());
  out.writeln("div grad:        ", operators::compose(operators::divergence({"x", "y"}), operators::gradient())(f)(point).item<double>());
  out.writeln("trace of hessian:", operators::hessian()(f).trace()(point).item<double>());
}
```

Output

```text
laplacian:        0.51122752
div grad:         0.51122752
trace of hessian: 0.51122752
```

### Example 5

The function must be scalar-valued.

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

  const math::Function field = math::function({"x", "y"}, [](math::Function x, math::Function y) { return math::stack({x, y}); });
  try { (void)operators::laplacian({"x", "y"})(field); } catch (const pluss::Error &e) { out.writeln(first_line(e)); }
}
```

Output

```text
operators::laplacian: the function must be scalar-valued
```

## See Also

- [`divergence`](divergence.md): with the gradient makes the Laplacian
- [`hessian`](../differential/hessian.md): its trace is the Laplacian
- [`gradient`](../differential/gradient.md): the first-order operator

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="divergence.html">
    <span class="label">Previous</span>
    <span class="title">divergence</span>
  </a>
  <a class="next-link" href="material.html">
    <span class="label">Next</span>
    <span class="title">material</span>
  </a>
</div>
<!-- apiref:nav:end -->
