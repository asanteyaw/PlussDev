<!-- generated from define.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `operators::define`

Builds your own `Operator` from a callable that turns a function into a function.

## Signature

```cpp
template <typename Callable>
operators::Operator operators::define(std::string_view key, Callable &&callable, operators::Options options = {}, operators::Operator adjoint = {});
template <typename Callable>
operators::Operator operators::define(Callable &&callable);
```

## What It Does

`define` is how you extend the operator set. The callable takes a `const math::Function &` and returns a `math::Function`; the resulting `Operator` is the same type as the built-in ones, so it can be applied, composed, combined, saved (if you allow it) and handed to anything that takes an operator.

The callable runs at *application time*, once, when `op(f)` is called: it receives `f` as a value and builds the result from `f` with the operations the library already has, including other operators. What it returns is an ordinary `Function`. Parameterise an operator by writing a factory function that captures the parameters in the callable. The callable may own move-only state.

`define(key, callable, options, adjoint)` gives the operator a *key* (its name for saving and diagnostics), declares its [`Options`](Options.md) (version, linearity, savability) and, optionally, supplies its formal adjoint, which is what makes [`adjoint`](adjoint.md) work on the new operator. `define(callable)` makes an anonymous operator with the default, cautious options. The library trusts the flags you declare; it checks the contracts it can: an empty callable, contradictory flags, a savable operator without a key, version zero, and `adjoint_known` without an adjoint operator are errors at definition, and a body that returns nothing is an error at application.

## Parameters

`std::string_view key`
: The operator's name. Required if the operator is `serializable`; otherwise may be empty. Choose one that cannot clash with another library's.

`Callable &&callable`
: A callable taking `const math::Function &` and returning `math::Function`. It may capture state, including move-only state.

`operators::Options options`
: Version, capability flags and payload. See `Options`.

`operators::Operator adjoint`
: The formal adjoint of the operator being defined. Supplying it sets `adjoint_known`; omit it when the adjoint is not known. Declaring `Options::adjoint_known` without passing it is refused.

## Returns

An `Operator` that applies the callable.

## Errors

- An empty callable, `structural` together with `evaluated_only`, a `serializable` operator without a key, `version = 0`, and `adjoint_known` without an adjoint operator are refused.
- A body that returns an undefined function raises `application returned an undefined Function` when applied.

## Practical Examples

### Example 1

A first operator: the logarithmic derivative `f′/f`, built from `differential` inside the callable. The callable runs once, when the operator meets the function; evaluating the result never calls it again.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  int applications = 0;
  const operators::Operator log_derivative = operators::define("example.log_derivative", [&applications](const math::Function &f) {
    ++applications;
    return operators::differential("x")(f) / f;
  });
  const math::Function gaussian = math::function({"x"}, [](math::Function x) { return (-(x * x)).exp(); });
  const math::Function rate = log_derivative(gaussian);
  const auto x = pluss::make_series({-1.0, 0.5, 2.0});
  out.writeln("f'/f for exp(-x^2):", rate({x}).items<double>(), " = -2x:", (x * -2.0).items<double>());
  out.writeln("evaluated twice; callable ran", applications, "time");
}
```

Output

```text
f'/f for exp(-x^2): [2.0000, -1.0000, -4.0000]  = -2x: [2.0000, -1.0000, -4.0000]
evaluated twice; callable ran 1 time
```

### Example 2

A parameterised operator from a factory, with move-only state and composed with a built-in. `power(n)` raises a function to the power `n`; the exponent lives in a `unique_ptr` that the operator owns.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <memory>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto power = [](double n) {
    auto exponent = std::make_unique<double>(n);
    return operators::define([exponent = std::move(exponent)](const math::Function &g) { return g.pow(*exponent); });
  };
  const math::Function g = math::function({"x"}, [](math::Function x) { return x * x + x; });
  const operators::Operator cube = power(3.0);
  out.writeln("g^3 at 1             =", cube(g)(1.0).item<double>(), "  (8)");
  out.writeln("d/dx g^3 at 1        =", operators::compose(operators::differential("x"), cube)(g)(1.0).item<double>(), "  (3 g^2 g' = 36)");
}
```

Output

```text
g^3 at 1             = 8.0000   (8)
d/dx g^3 at 1        = 36.0000   (3 g^2 g' = 36)
```

### Example 3

Declaring a key and options, and supplying the adjoint. Doubling is self-adjoint, so its adjoint is a `scale(2)`. Passing the adjoint makes `adjoint_known` true and lets `operators::adjoint` work on the operator; without it the operator reports no adjoint.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto doubler = [](const math::Function &f) { return f * 2.0; };
  const math::Function x = math::function({"x"}, [](math::Function v) { return v; });
  const operators::Operator with_adjoint = operators::define("example.doubler", doubler, operators::Options{.version = 2, .linear = true}, operators::scale(2.0));
  const operators::Operator without = operators::define("example.doubler", doubler, operators::Options{.linear = true});
  out.writeln("with adjoint:    key", with_adjoint.key(), " version", with_adjoint.version(), " linear", with_adjoint.capabilities().linear, " adjoint_known", with_adjoint.capabilities().adjoint_known);
  out.writeln("without adjoint: adjoint_known", without.capabilities().adjoint_known);
  out.writeln("adjoint(with_adjoint)(x)(3) =", operators::adjoint(with_adjoint)(x)(3.0).item<double>());
}
```

Output

```text
with adjoint:    key example.doubler  version 2  linear true  adjoint_known true
without adjoint: adjoint_known false
adjoint(with_adjoint)(x)(3) = 6.0000
```

The adjoint operator you pass is returned by `operators::adjoint`; it is your statement of the adjoint, not a computed one.

### Example 4

The checks `define` makes: an empty `std::function`, contradictory flags, a savable operator without a key, version zero, and a declared adjoint with no adjoint operator fail at definition; a body that returns nothing fails when the operator is applied.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <functional>
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

  const auto pass_through = [](const math::Function &f) { return f; };
  const math::Function f = math::function({"x"}, [](math::Function x) { return x; });
  try { (void)operators::define("e", std::function<math::Function(const math::Function &)>{}); } catch (const pluss::Error &e) { out.writeln("empty callable: ", first_line(e)); }
  try { (void)operators::define("b", pass_through, operators::Options{.structural = true, .evaluated_only = true}); } catch (const pluss::Error &e) { out.writeln("both flags:     ", first_line(e)); }
  try { (void)operators::define({}, pass_through, operators::Options{.serializable = true}); } catch (const pluss::Error &e) { out.writeln("savable, no key:", first_line(e)); }
  try { (void)operators::define("v", pass_through, operators::Options{.version = 0}); } catch (const pluss::Error &e) { out.writeln("version zero:   ", first_line(e)); }
  try { (void)operators::define("a", pass_through, operators::Options{.adjoint_known = true}); } catch (const pluss::Error &e) { out.writeln("adjoint, none:  ", first_line(e)); }
  const operators::Operator bad = operators::define("bad", [](const math::Function &) { return math::Function{}; });
  try { (void)bad(f); } catch (const pluss::Error &e) { out.writeln("returns nothing:", first_line(e)); }
}
```

Output

```text
empty callable:  operators::define: callable must not be empty
both flags:      operators::define: structural and evaluated_only cannot both be true
savable, no key: operators::define: a serializable custom operator requires a stable key
version zero:    operators::define: version must be positive
adjoint, none:   operators::define: adjoint_known requires the adjoint operator to be passed to define
returns nothing: operators::Operator: application returned an undefined Function
```

## See Also

- [`Options`](Options.md): the declarations
- [`adjoint`](adjoint.md): uses the adjoint you pass
- [`bind`](bind.md): register a loader so the operator can be loaded
- [`Operator::capabilities`](Operator/capabilities.md): what was declared

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="compose.html">
    <span class="label">Previous</span>
    <span class="title">compose</span>
  </a>
  <a class="next-link" href="identity.html">
    <span class="label">Next</span>
    <span class="title">identity</span>
  </a>
</div>
<!-- apiref:nav:end -->
