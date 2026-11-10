<!-- generated from cir_paths.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `rates::cir_paths`

Simulates paths of the Cox-Ingersoll-Ross short rate `dr = κ(θ − r)dt + σ√r dW`.

## Signature

```cpp
rates::RatePaths rates::cir_paths(std::int64_t steps, std::int64_t paths, double dt, const rates::CirParams &params, const rates::SimOptions &options, pluss::Config cfg = {}, std::uint64_t seed = 0);
rates::RatePaths rates::cir_paths(std::int64_t steps, std::int64_t paths, double dt, const rates::CirParams &params, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

The square-root diffusion: non-negative paths, with the exact noncentral chi-square transition available as `Scheme::Transition`. With `Euler` or `Milstein` the rate is floored at `rate_floor` to keep `√r` defined. The two signatures differ in where the random-number settings come from, as for [`vasicek_paths`](vasicek_paths.md).

## Parameters

`std::int64_t steps`
: Number of time steps, positive.

`std::int64_t paths`
: Number of paths, positive.

`double dt`
: Step length in years, positive and finite.

`const rates::CirParams &params`
: The model.

`const rates::SimOptions &options`
: Scheme, random numbers, layout, seed (first form).

`pluss::Config cfg`
: Device (and pinned memory) of the result; the type is Float64.

`std::uint64_t seed`
: Seed of the pseudo-random stream in the second form (the first reads `options.seed`).

## Returns

A [`RatePaths`](RatePaths.md).

## Errors

- A non-positive `steps`, `paths` or `dt`, invalid parameters, or invalid options throw (`pluss::Error`).

## Practical Examples

### Example 1

The mean and variance at maturity. The exact scheme gives `E r_T = θ + (r₀ − θ)e^{−κT}` and `Var r_T = r₀ σ²(e^{−κT} − e^{−2κT})/κ + θσ²(1 − e^{−κT})²/(2κ)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(7));
  const rt::CirParams p{.kappa = 0.7, .theta = 0.03, .sigma = 0.12, .r0 = 0.06, .rate_floor = 1e-12};
  rt::SimOptions options;
  options.scheme = rt::Scheme::Transition;
  options.seed = 3;
  const Series rT = rt::cir_paths(2, 200000, 1.0, p, options).rate.select(0, 2);
  const double k = 0.7, t = 0.03, s2 = 0.0144, r0 = 0.06, T = 2.0;
  const double var = r0 * s2 * (std::exp(-k * T) - std::exp(-2 * k * T)) / k + t * s2 * std::pow(1 - std::exp(-k * T), 2) / (2 * k);
  out.writeln("mean", rT.mean().item<double>(), "exact", t + (r0 - t) * std::exp(-k * T));
  out.writeln("var ", rT.var().item<double>(), "exact", var);
}
```

Output

```text
mean 0.0373931 exact 0.0373979
var  0.0004053 exact 0.0004045
```

### Example 2

The second signature: default options with a seed. The rate never drops below zero.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(8));
  const rt::CirParams p{};
  const rt::RatePaths sim = rt::cir_paths(50, 2000, 0.1, p, pluss::Config{}, 5);
  out.writeln("smallest rate on any path:", sim.rate.amin().item<double>());
}
```

Output

```text
smallest rate on any path: 0.00000000
```

### Example 3

Euler and exact transition as the step is refined, for the mean at two years.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rt::CirParams p{.kappa = 1.0, .theta = 0.03, .sigma = 0.15, .r0 = 0.08, .rate_floor = 1e-12};
  for (const std::int64_t steps : {2, 8, 64}) {
    rt::SimOptions options;
    options.seed = 3;
    out.writeln("steps", steps, ": Euler mean =", rt::cir_paths(steps, 100000, 2.0 / steps, p, options).rate.select(0, steps).mean().item<double>(), ", exact =", 0.03 + 0.05 * std::exp(-2.0));
  }
}
```

Output

```text
steps 2 : Euler mean = 0.03246 , exact = 0.03677
steps 8 : Euler mean = 0.03496 , exact = 0.03677
steps 64 : Euler mean = 0.03657 , exact = 0.03677
```

### Example 4

Invalid arguments are refused: a non-positive step count, path count or step.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <string>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const rt::CirParams p{};
  try {
    (void)(rt::cir_paths(0, 10, 0.1, p));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
  try {
    (void)(rt::cir_paths(10, 10, -0.1, p));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::finance::rates::cir_paths: steps must be > 0
subsp::math::finance::rates::cir_paths: dt must be finite and > 0
```

## See Also

- [`SimOptions`](SimOptions.md): the settings
- [`CirParams`](CirParams.md): the model
- [`cir_zcb`](cir_zcb.md): the closed-form bond price

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="VasicekParams.html">
    <span class="label">Previous</span>
    <span class="title">VasicekParams</span>
  </a>
  <a class="next-link" href="cir_zcb.html">
    <span class="label">Next</span>
    <span class="title">cir_zcb</span>
  </a>
</div>
<!-- apiref:nav:end -->
