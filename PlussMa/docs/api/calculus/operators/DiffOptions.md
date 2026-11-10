<!-- generated from DiffOptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::DiffOptions`

How the differential operators treat non-smooth points, opaque functions and runaway expression growth.

`DiffOptions` is the last argument of the differential operators (`differential`, `wirtinger`, `gradient`, `jacobian`, `hessian`, the vector-calculus and geometry operators, and the Taylor operators). It has three fields, each answering one question about what a derivative should do when the plain rules run out.

**`nonsmooth`** decides what the derivative of a function with a corner, such as `|x|`, is at the corner. **`fallback`** decides what happens when part of the function is an opaque callable the rules cannot see inside. **`max_nodes`** is a safety limit on how large the derivative expression may grow.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `nonsmooth` | `math::NonSmooth` | `Strict` | `Strict`: refuse at a non-differentiable point. `Piecewise`: apply the formula as written (which yields `NaN` where it is undefined). `Subgradient`: use 0 at the corner. `Autodiff`: use the automatic-differentiation value. |
| `fallback` | `math::Fallback` | `Reject` | `Reject`: refuse to differentiate an opaque callable. `Autodiff`: differentiate it automatically. |
| `max_nodes` | `std::size_t` | `100000` | The largest number of nodes the derivative expression may reach before the operator refuses. |

## Guidance

Leave the defaults unless you have a reason: `Strict` refuses to guess at a corner, `Reject` refuses to differentiate what it cannot see, and `max_nodes = 100000` is generous for ordinary use. Choose `Subgradient` for optimisation with kinks, `Piecewise` when `NaN` at the corner is the signal you want, and `Autodiff` for the corner only when you want the value automatic differentiation gives. Choose `Fallback::Autodiff` when part of a model is an existing numerical routine you cannot rewrite.

## Practical Examples

### Example 1

`nonsmooth` at a corner. The derivative of `|x|` is `−1` and `1` away from 0. At 0 `Strict` refuses, `Piecewise` returns `NaN` (the formula `x/|x|` has no value there), `Subgradient` returns 0 (the midpoint of the subdifferential), and `Autodiff` returns 0 as well.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

static const char *name(math::NonSmooth n) {
  switch (n) {
  case math::NonSmooth::Strict: return "Strict";
  case math::NonSmooth::Piecewise: return "Piecewise";
  case math::NonSmooth::Subgradient: return "Subgradient";
  case math::NonSmooth::Autodiff: return "Autodiff";
  }
  return "?";
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.abs(); });
  const auto x = pluss::make_series({-1.0, 0.0, 2.0});
  for (const math::NonSmooth policy : {math::NonSmooth::Strict, math::NonSmooth::Piecewise, math::NonSmooth::Subgradient, math::NonSmooth::Autodiff}) {
    const math::Function d = operators::differential("x", 1, {.nonsmooth = policy})(f);
    try {
      out.writeln(name(policy), d({x}).items<double>());
    } catch (const pluss::Error &e) {
      out.writeln(name(policy), "refused at 0");
    }
  }
}
```

Output

```text
Strict refused at 0
Piecewise [-1.0000, nan, 1.0000]
Subgradient [-1.0000, 0.0000, 1.0000]
Autodiff [-1.0000, 0.0000, 1.0000]
```

### Example 2

`fallback` for an opaque function. A callable on `Series` cannot be differentiated by rule. With `Fallback::Reject` (the default) the operator refuses; with `Fallback::Autodiff` it differentiates the callable automatically, to any order.

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

  const math::Function opaque = math::function({"x"}, [](const pluss::Series &x) { return x * x * x; });
  try {
    (void)operators::differential("x")(opaque);
  } catch (const pluss::Error &e) {
    out.writeln("Reject:   ", first_line(e));
  }
  const operators::DiffOptions autodiff{.fallback = math::Fallback::Autodiff};
  out.writeln("Autodiff, f'(2)  =", operators::differential("x", 1, autodiff)(opaque)(2.0).item<double>(), "  (3 x^2 = 12)");
  out.writeln("Autodiff, f''(2) =", operators::differential("x", 2, autodiff)(opaque)(2.0).item<double>(), "  (6 x = 12)");
}
```

Output

```text
Reject:    operators::differential: opaque Function requires explicit DiffOptions{.fallback = math::Fallback::Autodiff}
Autodiff, f'(2)  = 12.0000   (3 x^2 = 12)
Autodiff, f''(2) = 12.0000   (6 x = 12)
```

### Example 3

`max_nodes` bounds the size of the derivative expression. Repeated differentiation of a product-and-quotient grows quickly; a small limit turns runaway growth into an error that names the safeguard, and raising it allows the computation.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x"}, [](math::Function x) { return (x.sin() * x.exp()).tan(); });
  for (const std::size_t limit : {std::size_t{20}, std::size_t{200}, std::size_t{100000}}) {
    for (const std::int64_t order : {1, 2, 4}) {
      try {
        const double value = operators::differential("x", order, {.max_nodes = limit})(f)(0.3).item<double>();
        out.writeln("max_nodes", limit, "order", order, "-> f^(n)(0.3) =", value);
      } catch (const pluss::Error &) {
        out.writeln("max_nodes", limit, "order", order, "-> graph exceeds the max_nodes safeguard");
      }
    }
  }
}
```

Output

```text
max_nodes 20 order 1 -> f^(n)(0.3) = 1.9885
max_nodes 20 order 2 -> graph exceeds the max_nodes safeguard
max_nodes 20 order 4 -> graph exceeds the max_nodes safeguard
max_nodes 200 order 1 -> f^(n)(0.3) = 1.9885
max_nodes 200 order 2 -> f^(n)(0.3) = 5.8678
max_nodes 200 order 4 -> graph exceeds the max_nodes safeguard
max_nodes 100000 order 1 -> f^(n)(0.3) = 1.9885
max_nodes 100000 order 2 -> f^(n)(0.3) = 5.8678
max_nodes 100000 order 4 -> f^(n)(0.3) = 270.9378
```

## See Also

- [`differential`](../differential/differential.md): the main consumer
- [`abs`](../functions/Function/abs.md): the standard non-smooth function
- [`provenance`](../functions/Function/provenance.md): reports `Autodiff` after a fallback

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Capabilities.html">
    <span class="label">Previous</span>
    <span class="title">Capabilities</span>
  </a>
  <a class="next-link" href="Operator.html">
    <span class="label">Next</span>
    <span class="title">Operator</span>
  </a>
</div>
<!-- apiref:nav:end -->
