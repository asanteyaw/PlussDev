<!-- generated from numerical_integrals_and_batches.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Numerical integrals and batches

[Back to Integrals](../calculus_04_integrals.md)

When an integrand has no exact primitive in the rule table, the integral is kept as an unevaluated node and computed when the function is evaluated. This page describes what that computation is, what it needs from its arguments, and how it behaves on batches, precision and failure.

## What is computed

The integral is evaluated by `integ::quadra`, the library's adaptive composite quadrature, with its default rule (Simpson's rule) starting from 257 nodes, doubling the number of nodes until the error estimate reaches the tolerance or ten refinements are used. The tolerance follows the precision of the argument:

| Precision of the integrated variable's argument | Tolerance |
|---|---|
| `Float64` | 1e-10 |
| `Float32`, `Complex64` | 1e-4 |
| `Float16`, `BFloat16`, `Float8` | 1e-2 |

If the estimate does not reach the tolerance, evaluation fails with `unresolved integral quadrature did not converge`, followed by the error estimate. A result is never returned from an unconverged integral.

The integrand is called once per refinement with all nodes together, as an array, with every other variable broadcast against the nodes. That is what makes batches cheap: the cost of a batch of parameter values is that of one integral on larger arrays, not of many integrals.

## Anchored integrals need a single upper limit

The anchored form `integral(x, a)` is a function of its upper limit. For a numerical integral that limit must be one number: the argument for the integrated variable must have exactly one element. A definite integral has fixed limits and has no such requirement, so a batch of integrals over a parameter is written as a definite integral with the parameter as a variable.

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
  const math::Function gaussian = math::function({"x"}, [](math::Function x) { return (-x.pow(2)).exp(); });
  const math::Function anchored = operators::integral("x", 0.0)(gaussian);

  out.writeln("scalar upper limit 1:", anchored({pluss::make_series(1.0, as_f64())}).item<double>());
  out.writeln("one-element array   :", anchored({pluss::make_series({1.0})}).host<double>().front());
  try {
    (void)anchored({pluss::make_series({0.5, 1.0})});
  } catch (const pluss::Error &e) {
    const std::string text(e.what());
    out.writeln("two upper limits    :", text.substr(0, text.find('\n')));
  }

  // The batch form: a definite integral with the limits fixed and a parameter that varies.
  const math::Function f = math::function({"x", "upper"}, [](math::Function x, math::Function upper) { return (-(x * upper).pow(2)).exp(); });
  const math::Function area = operators::integral("x", 0.0, 1.0)(f);
  const auto upper = pluss::make_series({0.5, 1.0, 1.5, 2.0});
  out.writeln("batch of four:", area({upper}).pull<double>());
}
```

Output

```text
scalar upper limit 1: 0.74682413
one-element array   : 0.74682413
two upper limits    : math::Function: unresolved anchored integrals require a scalar upper-bound argument
batch of four: [0.92256201, 0.74682413, 0.57079226, 0.44104070]
```

A scalar and a one-element array both give 0.74682413, and two upper limits are refused with a message that names the rule. The batch form at the bottom evaluates four parameter values in one call and returns four integrals; their values 0.9226, 0.7468, 0.5708 and 0.4410 decrease as the integrand narrows with growing `upper`.

## Precision and platform

The integral is evaluated in the precision and on the platform of its Series arguments, and the result has that precision and platform. A zero-argument definite integral defaults to CPU `Float64`; the example keeps a unit-valued parameter so it can select `Float64` or `Float32` explicitly. In `Float32` the tolerance is looser (1e-4), so the answer is correct to about four digits, not ten.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/auxiliary/output.h>

namespace math = pluss::math;
namespace operators = pluss::operators;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const math::Function gaussian = math::function({"x", "scale"}, [](math::Function x, math::Function scale) { return (-x.pow(2)).exp() * scale; });
  const math::Function area = operators::integral("x", 0.0, 1.0)(gaussian);
  const pluss::Config f32 = pluss::Config{}.with_dtype(pluss::ScalarType::Float32);

  const auto high = area({pluss::make_series(1.0, as_f64())});
  const auto low = area({pluss::make_series(1.0, f32)});
  out.writeln("Float64:", high.item<double>(), " dtype Float64:", high.scalar_type() == pluss::ScalarType::Float64 ? "yes" : "no");
  out.writeln("Float32:", static_cast<double>(low.item<float>()), " dtype Float32:", low.scalar_type() == pluss::ScalarType::Float32 ? "yes" : "no");
  out.writeln("true value 0.7468241328");
}
```

Output

```text
Float64: 0.7468241328  dtype Float64: yes
Float32: 0.7468242049  dtype Float32: yes
true value 0.7468241328
```

The `Float64` integral is 0.7468241328, correct to ten digits. The `Float32` integral is 0.7468242049: the same to six digits, with the error of single precision in the seventh, and it comes back as a `Float32` series. The looser tolerance is what lets a single-precision evaluation converge at all.

## Differentiating a numerical integral

The derivative of an unresolved integral is built from the integrand's derivative, not by differencing numbers, so it is exact in structure and numerical only in the final quadrature. The next page shows the two cases, differentiation with respect to the upper limit and with respect to a parameter.

<!-- apiref:toctree:start -->
<!-- apiref:toctree:end -->
