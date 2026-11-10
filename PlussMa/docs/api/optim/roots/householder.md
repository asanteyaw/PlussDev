<!-- generated from householder.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `roots::householder`

Fourth-order Householder iteration, given the function and its first three derivatives; the root is differentiable with respect to anything the function captures.

## Signature

```cpp
roots::Result roots::householder(const roots::DerivativeFn &evaluate, pluss::Series x, const roots::Options &options = {});
```

## What It Does

`householder` uses the value and the first three derivatives at the current point, `f, f′, f″, f‴`, to take a step that cancels the error to fourth order: far fewer iterations than Newton on smooth functions, at the price of supplying the derivatives. The callback `evaluate` takes the current `Series` of iterates and returns a [`Derivatives`](Derivatives.md) holding all four `Series` together, so a caller can share intermediate quantities between them.

The distinctive property is what happens at the end. The converged root is rebuilt as an *implicit function* of every `Series` that `evaluate` captured: if a captured parameter `p` is tracked for gradients, calling `.reverse()` on the root gives `dx/dp = −(∂F/∂p)/(∂F/∂x)` at the solution, exactly, whatever the starting value and however many iterations ran. The iteration tape is not kept, so differentiating through a root costs one extra evaluation, not memory proportional to the iteration count. This is what makes it suitable for implied-volatility style calculations, where the sensitivity of the root to the inputs is wanted.

Invalid options are refused: an empty `evaluate`, a non-positive `max_iterations` or `check_every`, and negative tolerances all throw. `bracketed` in the [`Result`](Result.md) is always true.

## Parameters

`const roots::DerivativeFn &evaluate`
: A function of the current iterates that returns a `Derivatives` of `f` and its first three derivatives, all the same shape as the iterates. Series it captures may be gradient-tracked parameters.

`pluss::Series x`
: The starting value of each lane.

`const roots::Options &options`
: Iteration limit, tolerances and cadence. `derivative_step` is not used.

## Returns

A `roots::Result`: `root` (rebuilt as an implicit function of the captured parameters), `residual` (`f(root)`), `iterations`; `bracketed` is always true.

## Errors

- `roots::householder: evaluator is required`.
- `roots::householder: max_iterations must be positive`.
- `roots::householder: check_every must be positive`.
- `roots::householder: tolerances must be non-negative`.

## Practical Examples

### Example 1

Roots of `x² − p` with the three derivatives `2x`, `2`, `0`. For `p = 2` and `p = 5` in two lanes, a handful of iterations reach `√p` to rounding.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace roots = pluss::subsp::math::roots;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(14));
  const Series p = pluss::make_series({2.0, 5.0});
  const roots::DerivativeFn evaluate = [&](const Series &x) {
    return roots::Derivatives{x * x - p, x * 2.0, pluss::series::full_like(x, 2.0), pluss::series::zeros_like(x)};
  };
  roots::Options options;
  options.x_tolerance = 1e-13;
  options.check_every = 1;
  const roots::Result result = roots::householder(evaluate, pluss::make_series({1.0, 1.0}), options);

  out.writeln("root       =", result.root.items<double>());
  out.writeln("residual   =", result.residual.items<double>());
  out.writeln("iterations =", result.iterations);
}
```

Output

```text
root       = [1.41421356237310, 2.23606797749979]
residual   = [0.00000000000000, 0.00000000000000]
iterations = 4
```

### Example 2

The sensitivity of a root. The parameter `p` is marked as tracking gradients; after solving `x² = p`, `reverse()` on the root fills `p`'s sensitivity with `dx/dp = 1/(2√p)`. For `p = 4` that is `0.25`, independent of the starting value.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace roots = pluss::subsp::math::roots;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(10));
  Series p = pluss::make_series(4.0);
  p.tracks_slope_(true);
  const roots::DerivativeFn evaluate = [p](const Series &x) {
    return roots::Derivatives{x.square() - p, x * 2.0, pluss::series::full_like(x, 2.0), pluss::series::zeros_like(x)};
  };
  roots::Options options;
  options.max_iterations = 12;
  const roots::Result result = roots::householder(evaluate, pluss::make_series(1.0), options);
  result.root.reverse();

  out.writeln("root           =", result.root.item<double>());
  out.writeln("d root / d p   =", p.sensitivity().item<double>(), "  (1 / (2 sqrt p) = 0.25)");
}
```

Output

```text
root           = 2.0000000000
d root / d p   = 0.2500000000   (1 / (2 sqrt p) = 0.25)
```

### Example 3

Fewer iterations than Newton. For `x³ − 2` (root `2^(1/3) = 1.259921…`) the residual after 1 to 4 iterations is far smaller for `householder` than for `newton` from the same start.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(14));
  using pluss::subsp::math::roots::Derivatives;
  const pluss::subsp::math::roots::DerivativeFn evaluate = [](const Series &x) {
    return Derivatives{x * x * x - 2.0, x * x * 3.0, x * 6.0, pluss::series::full_like(x, 6.0)};
  };
  const pluss::subsp::math::roots::Fn f = [](const Series &x) { return x * x * x - 2.0; };
  const pluss::subsp::math::roots::Fn df = [](const Series &x) { return x * x * 3.0; };
  for (const std::int64_t iterations : {1, 2, 3, 4}) {
    pluss::subsp::math::roots::Options options;
    options.max_iterations = iterations;
    const auto fourth = pluss::subsp::math::roots::householder(evaluate, pluss::make_series(2.0), options);
    const auto second = pluss::subsp::math::roots::newton(f, pluss::make_series(2.0), df, options);
    out.writeln("iterations =", iterations, ": householder |f| =", std::abs(fourth.residual.item<double>()), ", newton |f| =", std::abs(second.residual.item<double>()));
  }
}
```

Output

```text
iterations = 1 : householder |f| = 0.00149248890448 , newton |f| = 1.37500000000000
iterations = 2 : householder |f| = 0.00000000000000 , newton |f| = 0.17827566935935
iterations = 3 : householder |f| = 0.00000000000000 , newton |f| = 0.00481928579257
iterations = 4 : householder |f| = 0.00000000000000 , newton |f| = 0.00000386058273
```

### Example 4

The checks: an empty callback, a non-positive iteration limit or cadence, and negative tolerances are each refused with a message that names the problem.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace roots = pluss::subsp::math::roots;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const auto first_line = [](const std::exception &e) {
    const std::string text(e.what());
    return text.substr(0, text.find('\n'));
  };

  const roots::DerivativeFn ok = [](const Series &x) { return roots::Derivatives{x, x, x, x}; };
  roots::Options bad_iterations, bad_cadence, bad_tolerance;
  bad_iterations.max_iterations = 0;
  bad_cadence.check_every = 0;
  bad_tolerance.x_tolerance = -1.0;
  const Series start = pluss::make_series(1.0);
  try { (void)roots::householder({}, start); } catch (const std::exception &e) { out.writeln("empty callback :", first_line(e)); }
  try { (void)roots::householder(ok, start, bad_iterations); } catch (const std::exception &e) { out.writeln("max_iterations 0:", first_line(e)); }
  try { (void)roots::householder(ok, start, bad_cadence); } catch (const std::exception &e) { out.writeln("check_every 0   :", first_line(e)); }
  try { (void)roots::householder(ok, start, bad_tolerance); } catch (const std::exception &e) { out.writeln("negative tol    :", first_line(e)); }
}
```

Output

```text
empty callback : roots::householder: evaluator is required
max_iterations 0: roots::householder: max_iterations must be positive
check_every 0   : roots::householder: check_every must be positive
negative tol    : roots::householder: tolerances must be non-negative
```

## See Also

- [`Derivatives`](Derivatives.md): the value and three derivatives the callback returns
- [`newton`](newton.md): second order, one derivative
- [`Result`](Result.md): what the call returns

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bisect.html">
    <span class="label">Previous</span>
    <span class="title">bisect</span>
  </a>
  <a class="next-link" href="illinois.html">
    <span class="label">Next</span>
    <span class="title">illinois</span>
  </a>
</div>
<!-- apiref:nav:end -->
