<!-- generated from quadra.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `integ::quadra`

Integrates a function over an interval, or over a box, by a quadrature rule that refines itself until it converges.

## Signature

```cpp
integ::QuadraResult integ::quadra(const integ::IntegrandFn &fn, double a, double b, const integ::QuadraOptions &options = {});
integ::QuadraNdResult integ::quadra(const integ::IntegrandNdFn &fn, const std::vector<std::pair<double, double>> &bounds, const integ::QuadraNdOptions &options = {});
```

## What It Does

**One variable.** `fn` takes a `Series` of nodes and returns the integrand at all of them, in the same shape; `quadra` builds the nodes of the chosen rule ([`QuadraRule`](QuadraRule.md)) over `[a, b]`, calls `fn` **once per resolution level**, and weights and sums the values. With `adaptive` on (the default) it doubles the resolution until the change between two levels is within `tolerance`, and reports that change as `error_estimate`. `a` and `b` may be infinite: `quadra` then maps the interval onto a finite one by a change of variables, so `[0, ∞)` and `(−∞, ∞)` integrals work directly (`infinite_domain_epsilon` sets how close to the singular end of the substitution it goes). `a > b` gives the negative of the integral over `[b, a]`.

`fn` may return a batch: values of shape `[..., n]`, with the node axis last. Every rule reduces along that last axis, so one call integrates a whole grid of integrals (for example, a family indexed by a parameter) that share the same nodes. A complex integrand gives a complex result. Because the integral is made of differentiable operations, a parameter the integrand captures can be differentiated through it.

**Several variables.** The second overload takes the integrand as a function of one coordinate `Series` per axis, each as long as the whole grid, and integrates over the box given by `bounds` with a tensor-product rule. The node count is the product of the per-axis counts, so it is meant for a few dimensions (`max_total_nodes` stops a request that would allocate an enormous grid); for many dimensions use [`aleatoria`](aleatoria.md). It has no adaptive refinement.

## Parameters

`const integ::IntegrandFn &fn`
: `Series -> Series`: the integrand at a whole `Series` of nodes, same shape out, any leading batch axes with the node axis last.

`double a, double b`
: The interval; either may be ±infinity.

`const integ::QuadraOptions &options`
: Rule, resolution, tolerance.

`const integ::IntegrandNdFn &fn`
: `std::vector<Series> -> Series`: one coordinate `Series` per axis.

`const std::vector<std::pair<double, double>> &bounds`
: One `(lower, upper)` pair per axis.

`const integ::QuadraNdOptions &options`
: Rule and resolution per axis.

## Returns

A [`QuadraResult`](QuadraResult.md) for one variable, a [`QuadraNdResult`](QuadraNdResult.md) for several.

## Errors

- A missing integrand, a negative tolerance or refinement count, or `a` and `b` that are infinite on the wrong sides throw.
- The N-dimensional form throws when the grid would exceed `max_total_nodes`.

## Practical Examples

### Example 1

A smooth function with a closed form. `∫₀^π sin x dx = 2`; the result carries its own error estimate and the number of function evaluations.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  const integ::QuadraResult r = integ::quadra([](const Series &t) { return t.sin(); }, 0.0, 3.14159265358979323846);
  out.writeln("value =", r.value.item<double>(), ", error estimate =", r.error_estimate);
  out.writeln("evaluations =", r.evaluations, ", refinements =", r.refinements, ", converged =", r.converged);
}
```

Output

```text
value = 2.0000000000 , error estimate = 0.0000000000
evaluations = 1803 , refinements = 2 , converged = true
```

### Example 2

An infinite domain. `∫ e^{−x²} dx` over the whole line is `√π = 1.7724538509`; `∫₀^∞ e^{−x} dx` is `1`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const integ::QuadraResult all = integ::quadra([](const Series &t) { return (t.square() * -1.0).exp(); }, -INFINITY, INFINITY);
  const integ::QuadraResult half = integ::quadra([](const Series &t) { return (t * -1.0).exp(); }, 0.0, INFINITY);
  out.writeln("whole line :", all.value.item<double>(), ", exact =", std::sqrt(3.14159265358979323846));
  out.writeln("half line  :", half.value.item<double>());
}
```

Output

```text
whole line : 1.77245385 , exact = 1.77245385
half line  : 1.00000000
```

### Example 3

A batch of integrals in one call. The integrand returns one row per exponent, so a single call computes `∫₀¹ xᵏ dx = 1/(k+1)` for `k = 0, 1, 2, 3`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const Series k = pluss::series::make({0.0, 1.0, 2.0, 3.0}).reshape({4, 1});
  const integ::QuadraResult r = integ::quadra([&](const Series &t) { return t.reshape({1, t.numel()}).pow(k); }, 0.0, 1.0);
  out.writeln("integrals =", r.value.items<double>(), ", exact = 1, 1/2, 1/3, 1/4");
}
```

Output

```text
integrals = [1.00000000, 0.50000000, 0.33333333, 0.25000000] , exact = 1, 1/2, 1/3, 1/4
```

### Example 4

A gradient through the integral. With `f(x; a) = e^{ax}`, `I(a) = (e^a − 1)/a`, so `I(1) = 1.71828183` and `I'(1) = 1`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  Series a = pluss::make_series(1.0);
  a.tracks_slope_(true);
  const integ::QuadraResult r = integ::quadra([&](const Series &t) { return (t * a).exp(); }, 0.0, 1.0);
  r.value.sum().reverse();
  out.writeln("I(1) =", r.value.item<double>(), ", dI/da =", a.sensitivity().item<double>());
}
```

Output

```text
I(1) = 1.71828183 , dI/da = 1.00000000
```

### Example 5

Two variables. `∫₀¹∫₀² x y dy dx = 1`: the integrand gets one coordinate `Series` per axis, each as long as the whole grid.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const integ::QuadraNdResult r = integ::quadra([](const std::vector<Series> &c) { return c[0] * c[1]; }, {{0.0, 1.0}, {0.0, 2.0}});
  out.writeln("value =", r.value.item<double>(), ", dimensions =", r.dimensions, ", evaluations =", r.evaluations);
}
```

Output

```text
value = 1.00000000 , dimensions = 2 , evaluations = 625
```

### Example 6

Three variables with Simpson nodes. The volume of the box times a constant: `∫ 2 dV` over `[0,1] × [0,2] × [0,3]` is 12.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace integ = pluss::subsp::math::integ;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  integ::QuadraNdOptions options;
  options.rule = integ::QuadraRule::Simpson;
  options.resolution = 9;
  const integ::QuadraNdResult r = integ::quadra([](const std::vector<Series> &c) { return c[0] * 0.0 + 2.0; }, {{0.0, 1.0}, {0.0, 2.0}, {0.0, 3.0}}, options);
  out.writeln("value =", r.value.item<double>(), ", evaluations =", r.evaluations);
}
```

Output

```text
value = 12.00000000 , evaluations = 1331
```

## See Also

- [`QuadraOptions`](QuadraOptions.md): the settings
- [`aleatoria`](aleatoria.md): Monte Carlo for many dimensions
- [`integra`](integra.md): choosing automatically

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="measure_of.html">
    <span class="label">Previous</span>
    <span class="title">measure_of</span>
  </a>
  <a class="next-link" href="quadra_scalar.html">
    <span class="label">Next</span>
    <span class="title">quadra_scalar</span>
  </a>
</div>
<!-- apiref:nav:end -->
