<!-- generated from domains_and_errors.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Domains and errors

[Back to Functions](../calculus_01_functions.md)

A `Function` preserves the real or complex dtype of its `Series` values. For real inputs it treats an operation outside its real domain as an error rather than as a `NaN`. For complex inputs it uses principal branches and rejects singular points such as `log(0)`. A `NaN` that appears in the middle of a computation and travels to the end is the hardest kind of failure to trace; an error that names the operation is easy. This page lists exactly which operations are checked, what is allowed to overflow or propagate, and when each kind of error is raised.

## Which operations are checked

Four primitives test their input on every evaluation and raise a `pluss::Error` if any element is outside the applicable real or complex domain.

| Operation | Real input | Complex input |
|---|---|---|
| `log(a)` | `a > 0` | `a != 0`, principal logarithm |
| `log1p(a)` | `a > -1` | `a != -1`, principal logarithm |
| `sqrt(a)` | `a >= 0` | every finite value, principal square root |
| `a / b` | `b != 0` | `b != 0` |

A `NaN` is neither positive nor non-negative, so a `NaN` reaching `log` or `sqrt` is also reported. The other primitives are not checked, and follow floating-point arithmetic: `exp` overflows to infinity, a negative number to a fractional power is `NaN`, zero to a negative power is infinity, and `tan` is finite near its poles. The program evaluates a function at one point at a time and prints either the value or the error, so each case can be seen.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <cmath>
#include <functional>
#include <string>

namespace math = pluss::math;
using namespace pluss::tils;

static void probe(OutputEngine &out, const char *label, const math::Function &f, double at) {
  try {
    const auto value = f({pluss::make_series(at, as_f64())}).item<double>();
    out.writeln(label, "->", value);
  } catch (const pluss::Error &e) {
    const std::string text(e.what());
    out.writeln(label, "-> error:", text.substr(text.find(' ') + 1, text.find('\n') - text.find(' ') - 1));
  }
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const math::Function id = math::function({"x"}, [](math::Function x) { return x; });

  out.writeln("-- checked --");
  probe(out, "log(1)           ", id.log(), 1.0);
  probe(out, "log(0)           ", id.log(), 0.0);
  probe(out, "log(-1)          ", id.log(), -1.0);
  probe(out, "log(nan)         ", id.log(), NAN);
  probe(out, "log1p(-0.5)      ", id.log1p(), -0.5);
  probe(out, "log1p(-1)        ", id.log1p(), -1.0);
  probe(out, "sqrt(4)          ", id.sqrt(), 4.0);
  probe(out, "sqrt(-1)         ", id.sqrt(), -1.0);
  probe(out, "1 / x at x = 2   ", 1.0 / id, 2.0);
  probe(out, "1 / x at x = 0   ", 1.0 / id, 0.0);

  out.writeln("-- not checked: ordinary floating-point arithmetic --");
  probe(out, "exp(1000)        ", id.exp(), 1000.0);
  probe(out, "(-1)^0.5         ", id.pow(0.5), -1.0);
  probe(out, "(-2)^3           ", id.pow(3.0), -2.0);
  probe(out, "0^(-1)           ", id.pow(-1.0), 0.0);
  probe(out, "tan(pi/2)        ", id.tan(), M_PI / 2.0);
  probe(out, "x + 1 at nan     ", id + 1.0, NAN);
}
```

Output

```text
-- checked --
log(1)            -> 0.000000
log(0)            -> error: log domain excludes zero and requires positive real values
log(-1)           -> error: log domain excludes zero and requires positive real values
log(nan)          -> error: log domain excludes zero and requires positive real values
log1p(-0.5)       -> -0.693147
log1p(-1)         -> error: log1p domain excludes -1 and requires real values greater than -1
sqrt(4)           -> 2.000000
sqrt(-1)          -> error: sqrt domain requires nonnegative values
1 / x at x = 2    -> 0.500000
1 / x at x = 0    -> error: division domain excludes zero denominators
-- not checked: ordinary floating-point arithmetic --
exp(1000)         -> inf
(-1)^0.5          -> nan
(-2)^3            -> -8.000000
0^(-1)            -> inf
tan(pi/2)         -> 16331239353195370.000000
x + 1 at nan      -> nan
```

The checked group matches the table: the boundary values 0 for `log`, −1 for `log1p` and 0 for division are errors, interior values are not, and `log(nan)` is reported. The unchecked group is what the underlying arithmetic does. In particular `(-1)^0.5` is `NaN`, not an error, because a power with a fractional exponent has no inexpensive exact domain test; if the base of such a power can be negative, constrain it yourself. `tan(π/2)` is about 1.6 × 10¹⁶, a large finite number, because π/2 is not exactly representable.

The check is applied to every element of the argument, so one bad point in an array fails the whole evaluation, and the message does not say which element. If a few points of a batch may be invalid, filter them out first, or evaluate where the function is defined.

## When each error is raised

Errors come at three distinct moments, and knowing which one you are in tells you where to look.

| Moment | Examples | Cause |
|---|---|---|
| Building a function | variable list errors; arity mismatch; incompatible shapes; combining independent traces; unknown variable in `arg` or `assume` | the description of the function is wrong |
| Calling a function | wrong number of arguments; unknown, missing or repeated name; an undefined `Series` | the arguments do not match the function |
| Evaluating a function | the four domain errors above | the values are outside the domain |

The program shows one of each, and the case that surprises people: a domain error in an expression built from constants is raised when the function is *evaluated*, not when it is built.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

#include <functional>
#include <string>

namespace math = pluss::math;
using namespace pluss::tils;

static void attempt(OutputEngine &out, const char *label, const std::function<void()> &call) {
  try {
    call();
    out.writeln(label, "-> no error");
  } catch (const pluss::Error &e) {
    const std::string text(e.what());
    out.writeln(label, "->", text.substr(0, text.find('\n')));
  }
}

int main() {
  OutputEngine out;
  const math::Function f = math::function({"x"}, [](math::Function x) { return x.log(); });
  const auto one = pluss::make_series({1.0});
  const auto negative = pluss::make_series({-1.0});

  attempt(out, "building, two variables but one parameter ", [] {
    (void)math::function({"x", "y"}, [](math::Function x) { return x; });
  });
  attempt(out, "building, shapes [2] and [3]               ", [] {
    (void)(math::function(pluss::make_series({1.0, 2.0})) +
           math::function(pluss::make_series({1.0, 2.0, 3.0})));
  });
  attempt(out, "building, log of the constant 0            ", [] { (void)math::function(0.0).log(); });
  attempt(out, "calling with a name that does not exist    ", [&] { (void)f({{"y", one}}); });
  attempt(out, "evaluating log(1)                          ", [&] { (void)f({one}); });
  attempt(out, "evaluating log(-1)                         ", [&] { (void)f({negative}); });
  attempt(out, "evaluating log of the constant 0           ", [] { (void)math::function(0.0).log()(); });
  attempt(out, "an undefined Function                      ", [] { (void)math::Function{}.arity(); });
}
```

Output

```text
building, two variables but one parameter  -> math::function: variable count 2 does not match callable arity 1
building, shapes [2] and [3]                -> math::Function: output shapes are not broadcast-compatible
building, log of the constant 0             -> no error
calling with a name that does not exist     -> math::Function: unknown variable binding 'y'
evaluating log(1)                           -> no error
evaluating log(-1)                          -> math::Function: log domain excludes zero and requires positive real values
evaluating log of the constant 0            -> math::Function: log domain excludes zero and requires positive real values
an undefined Function                       -> math::Function: operation requires a defined function
```

The first two are errors in the description of the function, found at construction. The third line is the instructive one: the logarithm of the constant 0 can be seen to be invalid when it is built, yet it builds, and the error comes on the last-but-three line, when the function is first evaluated. Domain errors are about values, and values exist only at evaluation. The same function evaluated at 1 gives no error, and at −1 it does. Calling with a wrong name is a binding error, raised before anything is computed.

## Catching errors

Every error described here is a `pluss::Error` (a `std::exception`), so one handler covers them. The message begins with the operation (`math::Function`, `math::function`, `operators::differential`, ...), followed by the reason; `what()` also carries the source location of the check, which is why the programs above print only the first line. Errors never leave a `Function` or an `Operator` in a damaged state: an object that failed to build does not exist, and a failed evaluation leaves the function usable for the next call.

<!-- apiref:toctree:start -->
<!-- apiref:toctree:end -->
