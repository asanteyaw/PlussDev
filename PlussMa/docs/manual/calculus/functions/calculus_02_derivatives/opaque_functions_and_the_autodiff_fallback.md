<!-- generated from opaque_functions_and_the_autodiff_fallback.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Opaque functions and the autodiff fallback

[Back to Derivatives](../calculus_02_derivatives.md)

A function built from a `Series` callable ([Functions from Series code](../calculus_01_functions/functions_from_series_code.md)) has no recorded structure to differentiate. By default `differential` refuses it, with a message that names the remedy. The remedy is an explicit choice, `DiffOptions::fallback = Fallback::Autodiff`, which differentiates the callable with the `Series` automatic-differentiation engine at evaluation time.

| `Fallback` | Opaque function |
|---|---|
| `Reject` (default) | error: `opaque Function requires explicit DiffOptions{.fallback = math::Fallback::Autodiff}` |
| `Autodiff` | differentiated by automatic differentiation when the derivative is evaluated |

The fallback applies only to the opaque part of a function. Structural parts of the same function are still differentiated exactly, and a function that is entirely structural is unaffected by the option.

## The fallback in use

The program differentiates `sin(x) · exp(x)`, written as `Series` code, to first, second and third order, on a vector of points, and compares each with the exact derivative of the same formula written with `Function` arithmetic.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

#include <string>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const math::Function opaque = math::function({"x"}, [](const pluss::Series &x) { return x.sin().mul(x.exp()); });
  const math::Function structural = math::function({"x"}, [](math::Function x) { return x.sin() * x.exp(); });
  const auto x = pluss::make_series({0.5, 1.0, 1.5});

  try {
    (void)operators::differential("x")(opaque);
  } catch (const pluss::Error &e) {
    const std::string text(e.what());
    out.writeln("default:", text.substr(0, text.find('\n')));
  }

  const operators::DiffOptions autodiff{.fallback = math::Fallback::Autodiff};
  for (const std::int64_t order : {1, 2, 3}) {
    const math::Function by_autodiff = operators::differential("x", order, autodiff)(opaque);
    const math::Function exact = operators::differential("x", order)(structural);
    out.writeln("order", order, " autodiff", by_autodiff({x}).pull<double>());
    out.writeln("        exact   ", exact({x}).pull<double>());
  }
}
```

Output

```text
default: operators::differential: opaque Function requires explicit DiffOptions{.fallback = math::Fallback::Autodiff}
order 1  autodiff [2.23732812, 3.75604923, 4.78748452]
        exact    [2.23732812, 3.75604923, 4.78748452]
order 2  autodiff [2.89377807, 2.93738788, 0.63404429]
        exact    [2.89377807, 2.93738788, 0.63404429]
order 3  autodiff [1.31289991, -1.63732269, -8.30688047]
        exact    [1.31289991, -1.63732269, -8.30688047]
```

Without the option the request is refused, and the message contains the exact option to write. With it, the first, second and third derivatives of the `Series` callable agree with the exact derivatives of the equivalent `Function` to all eight printed digits, at every point. Each order is obtained by differentiating the callable that many times, so the cost of the fallback grows with the order, while the cost of the structural derivative is paid once, when it is built.

The derivative of an array argument is taken pointwise, like every other derivative in this chapter: for three points the result has three values. (The callable must therefore act elementwise on its argument, which `sin(x) exp(x)` does.)

## Where the fallback is allowed

The option belongs to `DiffOptions`, so it is accepted by every differential operator, including `gradient`, `jacobian` and `hessian` of the next chapter. The result of an opaque derivative is marked `Provenance::Autodiff`; it can be differentiated again, evaluated anywhere, and combined with structural functions, but it cannot be saved. The program shows the provenance of each stage, and that a function that mixes an opaque part with ordinary arithmetic differentiates, with only the opaque part handled by the fallback.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

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
  out.print_defaults(PrintOptions{}.fixed(8));
  const operators::DiffOptions autodiff{.fallback = math::Fallback::Autodiff};
  const math::Function opaque = math::function({"x"}, [](const pluss::Series &x) { return x.sin().mul(x.exp()); });
  const auto point = pluss::make_series(0.5, as_f64());

  const math::Function mixed = opaque * 3.0 + 1.0;  // 3 f(x) + 1, still containing the opaque part
  out.writeln("provenance: opaque", name(opaque.provenance()), " mixed", name(mixed.provenance()));
  const math::Function d1 = operators::differential("x", 1, autodiff)(opaque);
  const math::Function d_mixed = operators::differential("x", 1, autodiff)(mixed);
  out.writeln("provenance: derivative", name(d1.provenance()), " derivative of mixed", name(d_mixed.provenance()));
  out.writeln("f'(0.5)              ", d1({point}).item<double>());
  out.writeln("(3 f + 1)'(0.5)      ", d_mixed({point}).item<double>(), " = 3 f'(0.5):", 3.0 * d1({point}).item<double>());

  // The gradient, Jacobian and Hessian honour the same option.
  const math::Function gradient = operators::gradient({"x"}, autodiff)(opaque);
  const math::Function hessian = operators::hessian({"x"}, autodiff)(opaque);
  out.writeln("gradient (rank", gradient.rank(), ")", gradient({point}).host<double>().front());
  out.writeln("hessian  (rank", hessian.rank(), ")", hessian({point}).host<double>().front());
  const math::Function second = operators::differential("x", 2, autodiff)(opaque);
  out.writeln("f''(0.5)             ", second({point}).item<double>());

  // The rule is not weakened by the fallback for ordinary functions: an exact function stays exact.
  const math::Function exact = math::function({"x"}, [](math::Function x) { return x.sin() * x.exp(); });
  out.writeln("provenance of an exact derivative with the option set:", name(operators::differential("x", 1, autodiff)(exact).provenance()));
}
```

Output

```text
provenance: opaque Opaque  mixed Opaque
provenance: derivative Autodiff  derivative of mixed Autodiff
f'(0.5)               2.23732812
(3 f + 1)'(0.5)       6.71198436  = 3 f'(0.5): 6.71198436
gradient (rank 1 ) 2.23732812
hessian  (rank 2 ) 2.89377807
f''(0.5)              2.89377807
provenance of an exact derivative with the option set: Structural
```

The opaque function and the mixed function are both `Opaque`, and the derivative of either is `Autodiff`. The mixed function `3 f + 1` differentiates with the fallback to 3 f′, which the second-to-last comparison confirms to all printed digits. `gradient` returns a rank-1 function and `hessian` a rank-2 function, as they do for structural functions, and their values at the point match the first and second derivatives. The last line is the guarantee in the other direction: setting the option on a function that is entirely structural changes nothing, and the result is still `Structural`.

## When to use it

The fallback is a bridge, not a replacement. Use it when a derivative is needed of code you cannot restate, as in a model evaluated by a library routine, and use the structural vocabulary everywhere else, where the derivative is exact symbolically, the result can be saved, and `Strict` mode still tells you about corners. The explicit option is required deliberately: an exact derivative that quietly became an automatic-differentiation derivative would change what the number means.

<!-- apiref:toctree:start -->
<!-- apiref:toctree:end -->
