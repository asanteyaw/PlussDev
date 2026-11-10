<!-- generated from capabilities.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1524b66bd3bf -->

# `operators::Operator::capabilities`

[Back to `operators::Operator`](../Operator.md)

Reports the properties the operator promises: linearity, whether it can be saved, whether its adjoint is known.

## Signature

```cpp
operators::Capabilities operators::Operator::capabilities() const;
```

## What It Does

Different operators promise different things about themselves, and some of the library's behaviour depends on those promises. `capabilities()` returns the [`Capabilities`](../Capabilities.md) record: whether the operator is *linear* (`L(af + bg) = aLf + bLg`), whether its *adjoint* is known (so [`adjoint`](../adjoint.md) can build it), whether it can be *serialized* (so `io::save` accepts it), whether it builds a *structural* result or only an evaluated one, and whether it needs a sampled path axis.

Ask it before relying on a property: that an operator can be saved, that a composition is linear, that taking the adjoint will work. The flags of a combined operator are computed from its parts (a composition is linear only if every part is, and savable only if every part is), so asking the combination is the way to know.

For operators you define yourself the flags are exactly what you declared; the library trusts them. See [`Options`](../Options.md).

## Returns

An `operators::Capabilities` record of boolean flags.

## Errors

- The operator must be defined.

## Practical Examples

### Example 1

The capabilities of several built-in operators. Differentiation is linear and has a known adjoint; the magnitude of a derivative is not linear; `multiply` holds a function coefficient and cannot be saved.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

static void show(OutputEngine &out, const char *label, const operators::Operator &op) {
  const operators::Capabilities c = op.capabilities();
  out.writeln(label, " structural", c.structural, " linear", c.linear, " adjoint_known", c.adjoint_known, " serializable", c.serializable);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  show(out, "differential:  ", operators::differential("x"));
  show(out, "magnitude:     ", operators::magnitude_differential("x"));
  show(out, "scale(2):      ", operators::scale(2.0));
  show(out, "multiply(x):   ", operators::multiply(math::function({"x"}, [](math::Function x) { return x; })));
  show(out, "gradient:      ", operators::gradient());
}
```

Output

```text
differential:    structural true  linear true  adjoint_known true  serializable true
magnitude:       structural true  linear false  adjoint_known false  serializable true
scale(2):        structural true  linear true  adjoint_known true  serializable true
multiply(x):     structural true  linear true  adjoint_known true  serializable false
gradient:        structural true  linear true  adjoint_known false  serializable true
```

### Example 2

Combined operators compute their flags from their parts. A composition of linear operators is linear and has an adjoint; put one non-linear operator in the chain and it is neither.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

static void show(OutputEngine &out, const char *label, const operators::Operator &op) {
  const operators::Capabilities c = op.capabilities();
  out.writeln(label, " structural", c.structural, " linear", c.linear, " adjoint_known", c.adjoint_known, " serializable", c.serializable);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const operators::Operator d = operators::differential("x");
  show(out, "d then scale:      ", operators::compose(operators::scale(3.0), d));
  show(out, "magnitude then d:  ", operators::compose(d, operators::magnitude_differential("x")));
  show(out, "d + 2 shift:       ", operators::linear_combination({{1.0, d}, {2.0, operators::shift("x", 1.0)}}));
}
```

Output

```text
d then scale:        structural true  linear true  adjoint_known true  serializable true
magnitude then d:    structural true  linear false  adjoint_known false  serializable true
d + 2 shift:         structural true  linear true  adjoint_known true  serializable false
```

### Example 3

Deciding what to do from the flags: save a pipeline only if it is serializable, and take its adjoint only if that is known.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <sstream>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const operators::Operator pipeline = operators::compose(operators::scale(2.0), operators::differential("x"));
  const operators::Capabilities c = pipeline.capabilities();
  if (c.serializable) {
    std::ostringstream bytes(std::ios::binary);
    math::io::save(pipeline, bytes);
    out.writeln("saved, size > 0:", bytes.str().size() > 0);
  }
  if (c.adjoint_known) {
    const math::Function f = math::function({"x"}, [](math::Function x) { return x * x; });
    out.writeln("adjoint applied to x^2 at 3:", operators::adjoint(pipeline)(f)(3.0).item<double>(), "  (-2 * d/dx x^2 = -12)");
  }
}
```

Output

```text
saved, size > 0: true
adjoint applied to x^2 at 3: -12.0000   (-2 * d/dx x^2 = -12)
```

## See Also

- [`Capabilities`](../Capabilities.md): what each flag means
- [`Options`](../Options.md): declaring capabilities for your own operator

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="call.html">
    <span class="label">Previous</span>
    <span class="title">call</span>
  </a>
  <a class="next-link" href="defined.html">
    <span class="label">Next</span>
    <span class="title">defined</span>
  </a>
</div>
<!-- apiref:nav:end -->
