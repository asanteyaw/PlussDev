<!-- generated from simplification_and_assumptions.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Simplification and assumptions

[Back to Functions](../calculus_01_functions.md)

When an expression is built, Pluss rewrites it to a smaller one that is equal wherever the original is defined. Nothing needs to be called to get this, and it never changes a result that was finite and correct; it removes work, and in a few cases it removes spurious failures. Two ideas are involved: rewrites that are valid for every real input, and rewrites that are valid only under an assumption you state.

## Rewrites that need no assumption

The rewrites that always apply are the ones that are exact for every real value of the variables and that keep the shape of the result:

| Expression | Becomes |
|---|---|
| `a - a` (the same node on both sides) | zeros of the common shape |
| `0 * a`, `a * 0` | zeros of the common shape |
| `-(-a)` | `a` |
| `exp(a)` then `log` | `a` |
| operations on constants only | one constant, computed at construction |
| `a*b + a*c` with the same `a` | `a*(b + c)` |
| `sqrt(a^2)` | `abs(a)` |

"The same node" matters. Pluss recognises that two operands are one object, which happens when you name a sub-expression and use it twice. Two separately written expressions that happen to look alike are different nodes and are left alone, because proving them equal in general is not possible. The program shows the difference at a point where it matters, `x = ∞`, where `x − x` is `NaN` in floating point but is exactly 0 as a mathematical statement.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

#include <cmath>
#include <limits>

namespace math = pluss::math;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto at = pluss::make_series(
      {3.0, 7.0, std::numeric_limits<double>::infinity()});

  const math::Function difference = math::function({"x"}, [](math::Function x) { return x - x; });
  const math::Function times_zero = math::function({"x"}, [](math::Function x) { return 0.0 * x; });
  const math::Function double_negation = math::function({"x"}, [](math::Function x) { return -(-x); });
  const math::Function folded = math::function({"x"}, [](math::Function x) { return x + (math::function(2.0) + 3.0) * 4.0; });

  const math::Function shared = math::function({"x"}, [](math::Function x) {
    const math::Function square = x * x;
    return square - square;  // one node used twice
  });
  const math::Function separate = math::function({"x"}, [](math::Function x) { return x * x - x * x; });  // two nodes

  out.writeln("x                    ", at.pull<double>());
  out.writeln("x - x                ", difference({at}).pull<double>());
  out.writeln("0 * x                ", times_zero({at}).pull<double>());
  out.writeln("-(-x)                ", double_negation({at}).pull<double>());
  out.writeln("x + (2 + 3) * 4      ", folded({at}).pull<double>());
  out.writeln("s - s, s = x * x     ", shared({at}).pull<double>());
  out.writeln("x*x - x*x (two nodes)", separate({at}).pull<double>());

  // log(exp(x)) is x for every real x, including values where exp underflows or overflows.
  const math::Function round_trip = math::function({"x"}, [](math::Function x) { return x.exp().log(); });
  const auto wide = pluss::make_series({-800.0, 0.0, 800.0});
  out.writeln("log(exp(x)) at -800, 0, 800", round_trip({wide}).pull<double>());
}
```

Output

```text
x                     [3.0000, 7.0000, inf]
x - x                 [0.0000, 0.0000, 0.0000]
0 * x                 [0.0000, 0.0000, 0.0000]
-(-x)                 [3.0000, 7.0000, inf]
x + (2 + 3) * 4       [23.0000, 27.0000, inf]
s - s, s = x * x      [0.0000, 0.0000, 0.0000]
x*x - x*x (two nodes) [0.0000, 0.0000, nan]
log(exp(x)) at -800, 0, 800 [-800.0000, 0.0000, 800.0000]
```

The third point is infinite. Computed in floating point, `∞ − ∞` and `0 · ∞` are both `NaN`; here `x − x` and `0 * x` are the exact value 0 at every point, because the expression was simplified before it was evaluated. The shared square gives the same exact 0. The separately written `x*x - x*x` is two nodes, is not simplified, and so reports `NaN` at infinity, as the arithmetic does. If you want the exact behaviour, name the sub-expression once and reuse it, as `square` does. The `exp` and `log` line shows the round trip returning the argument at ±800, where `exp` underflows to 0 and overflows to infinity if it is actually computed; the simplification means it is not.

The reverse composition, `exp(log(a))`, is not rewritten: it equals `a` only for positive `a`, and for other values the original raises a domain error ([Domains and errors](domains_and_errors.md)). A rewrite is applied only when it can never hide a failure the original would have reported.

## Assumptions you declare

Some identities hold only on part of the real line. `sqrt(a²) = |a|` for all `a`, and equals `a` only for `a ≥ 0`; `a / a = 1` only for `a ≠ 0`. You can tell a function what you know about a variable with `assume(variable, assumption)`, and it returns a new function that uses the knowledge to simplify further.

| `Assumption` | Meaning for the variable |
|---|---|
| `Real` | no information (the default) |
| `Zero` | equals 0 |
| `NonZero` | never 0 |
| `NonNegative` | at least 0 |
| `Positive` | greater than 0 |
| `NonPositive` | at most 0 |
| `Negative` | less than 0 |

Assumptions accumulate: stating a second one combines it with the first, and the result is the strongest statement implied by both. Two statements that cannot both hold are an error.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

#include <string>

namespace math = pluss::math;
using namespace pluss::tils;
using A = math::Assumption;

static const char *name(A a) {
  switch (a) {
  case A::Real: return "Real";
  case A::Zero: return "Zero";
  case A::NonZero: return "NonZero";
  case A::NonNegative: return "NonNegative";
  case A::Positive: return "Positive";
  case A::NonPositive: return "NonPositive";
  case A::Negative: return "Negative";
  }
  return "?";
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function f = math::function({"x"}, [](math::Function x) { return x; });

  out.writeln("default:", name(f.assumption("x")));
  const std::pair<A, A> pairs[] = {{A::NonNegative, A::NonZero}, {A::NonPositive, A::NonZero}, {A::NonNegative, A::NonPositive},
                                   {A::Positive, A::NonNegative}, {A::Zero, A::NonZero}, {A::Positive, A::Negative}};
  for (const auto &[first, second] : pairs) {
    try {
      out.writeln(name(first), "then", name(second), "->", name(f.assume("x", first).assume("x", second).assumption("x")));
    } catch (const pluss::Error &e) {
      const std::string text(e.what());
      out.writeln(name(first), "then", name(second), "->", text.substr(0, text.find('\n')));
    }
  }
  try {
    (void)f.assume("y", A::Positive);
  } catch (const pluss::Error &e) {
    const std::string text(e.what());
    out.writeln("unknown variable ->", text.substr(0, text.find('\n')));
  }
}
```

Output

```text
default: Real
NonNegative then NonZero -> Positive
NonPositive then NonZero -> Negative
NonNegative then NonPositive -> Zero
Positive then NonNegative -> Positive
Zero then NonZero -> math::Function: contradictory variable assumptions
Positive then Negative -> math::Function: contradictory variable assumptions
unknown variable -> math::Function::assume: unknown variable 'y'
```

`NonNegative` and `NonZero` together give `Positive`, `NonPositive` and `NonZero` give `Negative`, and `NonNegative` with `NonPositive` leaves only `Zero`. Restating something weaker (`Positive` then `NonNegative`) keeps the stronger statement. `Zero` with `NonZero`, and `Positive` with `Negative`, contradict each other and raise an error, as does naming a variable the function does not have. `assume` returns a new function; the original keeps its own assumptions.

### What an assumption changes

The next program applies the two examples. Without information `sqrt(x²)` is `|x|` and `x / x` fails at 0. With the assumption the expression collapses to `x` and to the constant 1.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

#include <string>

namespace math = pluss::math;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const math::Function root = math::function({"x"}, [](math::Function x) { return x.pow(2).sqrt(); });
  const math::Function ratio = math::function({"x"}, [](math::Function x) { return x / x; });
  const math::Function root_nonneg = root.assume("x", math::Assumption::NonNegative);
  const math::Function ratio_nonzero = ratio.assume("x", math::Assumption::NonZero);

  const auto values = pluss::make_series({1.0, 2.0, 3.0});
  const auto mixed = pluss::make_series({-2.0, 0.0, 3.0});

  out.writeln("sqrt(x^2) on 1,2,3:                 ", root({values}).pull<double>());
  out.writeln("sqrt(x^2) on -2,0,3:                ", root({mixed}).pull<double>());
  out.writeln("sqrt(x^2) | NonNegative on 1,2,3:   ", root_nonneg({values}).pull<double>());
  out.writeln("x / x | NonZero on 1,2,3:           ", ratio_nonzero({values}).pull<double>());
  try {
    (void)ratio({mixed});
  } catch (const pluss::Error &e) {
    const std::string text(e.what());
    out.writeln("x / x on -2,0,3 without assumption: ", text.substr(0, text.find('\n')));
  }

  // The assumption is a promise, not a check.
  out.writeln("sqrt(x^2) | NonNegative on -2,0,3:  ", root_nonneg({mixed}).pull<double>());
  out.writeln("x / x | NonZero on -2,0,3:          ", ratio_nonzero({mixed}).pull<double>());
}
```

Output

```text
sqrt(x^2) on 1,2,3:                  [1.0000, 2.0000, 3.0000]
sqrt(x^2) on -2,0,3:                 [2.0000, 0.0000, 3.0000]
sqrt(x^2) | NonNegative on 1,2,3:    [1.0000, 2.0000, 3.0000]
x / x | NonZero on 1,2,3:            [1.0000, 1.0000, 1.0000]
x / x on -2,0,3 without assumption:  math::Function: division domain excludes zero denominators
sqrt(x^2) | NonNegative on -2,0,3:   [-2.0000, 0.0000, 3.0000]
x / x | NonZero on -2,0,3:           [1.0000, 1.0000, 1.0000]
```

With the assumption the expressions simplify to `x` and to the constant 1, exactly as the first four lines of values show. Without it, `sqrt(x²)` stays `|x|` (the second line, with the correct 2 for −2), and `x / x` is a division that fails at the zero denominator.

The last two lines are a warning that deserves to be read twice. **An assumption is a promise, not a check.** The function does not test your data against it. At −2 the assumption-simplified `sqrt(x²)` returns −2 instead of 2, and at 0 the assumption-simplified `x / x` returns 1 instead of failing, because both functions were simplified into `x` and `1` before they ever saw the data. Use an assumption when the property is a structural fact of your problem (a rate that is positive by construction, a variance, a distance), and not as a way to silence an error you do not understand. Domain errors for `log`, `sqrt` and division are not removed by an assumption on a variable they are applied to; they are checked on the values that reach them.

<!-- apiref:toctree:start -->
<!-- apiref:toctree:end -->
