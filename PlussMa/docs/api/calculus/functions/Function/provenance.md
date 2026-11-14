<!-- generated from provenance.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 36d80ba59611 -->

# `math::Function::provenance`

[Back to `math::Function`](../Function.md)

Reports how a function's formula is known: exactly, as an unopened callable, through a lazy integral, or through automatic differentiation.

## Signature

```cpp
math::Provenance math::Function::provenance() const;
```

## What It Does

Not every `Function` is equally transparent. One built from `Function` arithmetic is a recorded formula that the library can differentiate, integrate and save exactly. One built from a callable that works on `Series` directly is a black box: it can be evaluated, but nothing inside it can be inspected. `provenance()` tells you which situation you are in, with one of four values:

| `Provenance` | Meaning |
|---|---|
| `Structural` | Built entirely from the recorded vocabulary. Exact derivatives and closed forms are available and the function can be saved. |
| `Opaque` | Contains a `Series` callable the library cannot look inside. It can be evaluated and, with an explicit fallback, differentiated numerically. It cannot be saved. |
| `Quadrature` | Contains an unresolved integral, for example the result of a transform or of an integral operator, that is evaluated numerically when values are supplied. |
| `Autodiff` | Contains a derivative of an opaque callable, obtained by automatic differentiation rather than by a symbolic rule. |

When several apply, the strongest label wins in the order `Autodiff`, `Quadrature`, `Opaque`, `Structural`: a function that is mostly structural but contains one opaque piece is `Opaque`.

Call it when what you are allowed to do depends on how the function is known: check for `Structural` before [`io::save`](../../io/save.md), or before relying on an exact derivative; warn the user when a result is `Quadrature` or `Autodiff` because it carries numerical error that a structural function would not.

## Returns

A `math::Provenance`: `Structural`, `Opaque`, `Quadrature` or `Autodiff`.

## Errors

- The function must be defined.

## Practical Examples

### Example 1

Recorded algebra is `Structural`, and stays so under derivatives, products and compositions with other structural functions.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

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
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.sin() * x.exp() + x * x; });
  const math::Function df = operators::differential("x")(f);
  const math::Function d2f = operators::differential("x", 2)(f);
  const math::Function scaled = 3.0 * df + d2f;

  out.writeln("f:        ", name(f.provenance()));
  out.writeln("f':       ", name(df.provenance()));
  out.writeln("f'':      ", name(d2f.provenance()));
  out.writeln("3 f' + f'':", name(scaled.provenance()));
}
```

Output

```text
f:         Structural
f':        Structural
f'':       Structural
3 f' + f'': Structural
```

Differentiating a structural function applies derivative rules to the recorded graph, so the derivative is structural too. Nothing here is approximated.

### Example 2

A callable that takes and returns a `Series` is `Opaque`: the library cannot see inside it. Differentiating it needs an explicit fallback and the result is `Autodiff`; the value it gives is the derivative of the C++ code as written.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

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
  const math::Function black_box = math::function({"x"}, [](const pluss::Series &x) { return x * x * x; });
  out.writeln("black box:   ", name(black_box.provenance()), "  f(2) =", black_box(2.0).item<double>());

  const math::Function derivative = operators::differential("x", 1, {.fallback = math::Fallback::Autodiff})(black_box);
  out.writeln("derivative:  ", name(derivative.provenance()), "  f'(2) =", derivative(2.0).item<double>());

  const math::Function mixed = 2.0 * black_box + 1.0;
  out.writeln("2 f + 1:     ", name(mixed.provenance()), "  value at 2 =", mixed(2.0).item<double>());
}
```

Output

```text
black box:    Opaque   f(2) = 8.0000
derivative:   Autodiff   f'(2) = 12.0000
2 f + 1:      Opaque   value at 2 = 17.0000
```

`d/dx x³ = 3x² = 12` at 2. Arithmetic with constants keeps the function opaque: one piece the library cannot see inside is enough.

### Example 3

An integral operator whose antiderivative is not available in closed form leaves an unresolved integral that is evaluated by quadrature when values arrive. Its provenance is `Quadrature`, and it outranks `Opaque`.

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
  const math::Function f = math::function({"x", "t"}, [](math::Function x, math::Function t) { return (t * x).sin(); });
  const math::Function transformed = operators::integral("t", 0.0, 1.0)(f);
  out.writeln("f(x, t):             ", name(f.provenance()));
  out.writeln("integral over t:     ", name(transformed.provenance()), "  arity", transformed.arity());
  out.writeln("value at x = 2:      ", transformed(2.0).item<double>(), " (exact (1 - cos 2) / 2 =", (1.0 - std::cos(2.0)) / 2.0, ")");

  const math::Function black_box = math::function({"x"}, [](const pluss::Series &x) { return x.sin(); });
  const math::Function area = operators::integral("x", 0.0, 3.0)(black_box);
  out.writeln("integral of black box:", name(area.provenance()), "  value", area().item<double>(), "(exact", 1.0 - std::cos(3.0), ")");
}
```

Output

```text
f(x, t):              Structural
integral over t:      Quadrature   arity 1
value at x = 2:       0.7081  (exact (1 - cos 2) / 2 = 0.7081 )
integral of black box: Quadrature   value 1.9900 (exact 1.9900 )
```

Both integrals are evaluated numerically at the point the values are supplied. The integral of `sin(t x)` over `t` is left unresolved and evaluated numerically once `x` arrives, which the label states; the value agrees with the exact `(1 − cos 2) / 2` to the printed digits. The same holds for the opaque integrand, whose antiderivative cannot be found at all.

### Example 4

A guard for saving. `io::save` accepts only functions that are fully recorded, so check `provenance()` first and give the user a reason instead of an exception from deep inside the writer.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <sstream>

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
  const auto try_save = [&](const char *label, const math::Function &f) {
    if (f.provenance() != math::Provenance::Structural) {
      out.writeln(label, "-> not saved:", name(f.provenance()), "function");
      return;
    }
    std::ostringstream bytes;
    math::io::save(f, bytes);
    out.writeln(label, "-> saved", bytes.str().size(), "bytes");
  };

  try_save("sin(x) exp(x)", math::function({"x"}, [](math::Function x) { return x.sin() * x.exp(); }));
  try_save("black box   ", math::function({"x"}, [](const pluss::Series &x) { return x * x; }));
}
```

Output

```text
saved 195 bytes sin(x) exp(x)
black box    -> not saved: Opaque function
```

The byte count depends on the format version and is not a number to rely on; what matters is that the opaque function is declined before any writing starts.

## See Also

- [`operators::differential`](../../differential/differential.md): the `Fallback::Autodiff` option that makes an opaque derivative possible
- [`io::save`](../../io/save.md): requires a structural function
- [`math::function`](../function_factory.md): the callable forms that produce structural and opaque functions

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pow.html">
    <span class="label">Previous</span>
    <span class="title">pow</span>
  </a>
  <a class="next-link" href="rank.html">
    <span class="label">Next</span>
    <span class="title">rank</span>
  </a>
</div>
<!-- apiref:nav:end -->
