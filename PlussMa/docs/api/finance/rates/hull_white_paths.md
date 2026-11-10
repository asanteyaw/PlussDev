<!-- generated from hull_white_paths.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `rates::hull_white_paths`

Simulates paths of the Hull-White short rate `dr = κ(θ₀ + θ₁t − r)dt + σ dW`.

## Signature

```cpp
rates::RatePaths rates::hull_white_paths(std::int64_t steps, std::int64_t paths, double dt, const rates::HullWhiteParams &params, const rates::SimOptions &options, pluss::Config cfg = {}, std::uint64_t seed = 0);
rates::RatePaths rates::hull_white_paths(std::int64_t steps, std::int64_t paths, double dt, const rates::HullWhiteParams &params, pluss::Config cfg = {}, std::uint64_t seed = 0);
```

## What It Does

Vasicek with a linearly moving level. `Scheme::Transition` is exact (the level is integrated over each step); `Euler` has a discretisation error of order `dt`. The two signatures differ in where the random-number settings come from, as for [`vasicek_paths`](vasicek_paths.md).

## Parameters

`std::int64_t steps`
: Number of time steps, positive.

`std::int64_t paths`
: Number of paths, positive.

`double dt`
: Step length in years, positive and finite.

`const rates::HullWhiteParams &params`
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

The distribution at maturity with a flat level: the mean is `θ₀ + (r₀ − θ₀)e^{−κT}`.

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
  out.print_defaults(PrintOptions{}.fixed(6));
  const rt::HullWhiteParams p{.kappa = 0.7, .sigma = 0.01, .r0 = 0.06, .theta0 = 0.03, .theta_slope = 0.0};
  rt::SimOptions options;
  options.scheme = rt::Scheme::Transition;
  options.seed = 3;
  out.writeln("mean r_3 =", rt::hull_white_paths(3, 100000, 1.0, p, options).rate.select(0, 3).mean().item<double>(), ", exact =", 0.03 + 0.03 * std::exp(-0.7 * 3.0));
}
```

Output

```text
mean r_3 = 0.033639 , exact = 0.033674
```

### Example 2

A rising level, second signature: the mean drifts up with the slope.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4));
  const rt::HullWhiteParams p{.kappa = 2.0, .sigma = 0.005, .r0 = 0.03, .theta0 = 0.03, .theta_slope = 0.005};
  const rt::RatePaths sim = rt::hull_white_paths(5, 20000, 1.0, p, pluss::Config{}, 4);
  out.writeln("mean r_t at t = 0..5:", sim.rate.mean(1, false).items<double>());
}
```

Output

```text
mean r_t at t = 0..5: [0.0300, 0.0300, 0.0400, 0.0400, 0.0501, 0.0499]
```

### Example 3

Euler against the exact scheme with a coarse step.

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
  const rt::HullWhiteParams p{.kappa = 1.5, .sigma = 0.01, .r0 = 0.08, .theta0 = 0.03, .theta_slope = 0.0};
  for (const rt::Scheme scheme : {rt::Scheme::Euler, rt::Scheme::Transition}) {
    rt::SimOptions options;
    options.scheme = scheme;
    options.seed = 3;
    out.writeln(static_cast<int>(scheme), ": mean r_2 =", rt::hull_white_paths(2, 100000, 1.0, p, options).rate.select(0, 2).mean().item<double>(), ", exact =", 0.03 + 0.05 * std::exp(-3.0));
  }
}
```

Output

```text
0 : mean r_2 = 0.04248 , exact = 0.03249
2 : mean r_2 = 0.03248 , exact = 0.03249
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
  const rt::HullWhiteParams p{};
  try {
    (void)(rt::hull_white_paths(0, 10, 0.1, p));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
  try {
    (void)(rt::hull_white_paths(10, 10, -0.1, p));
  } catch (const std::exception &e) {
    const std::string text(e.what());
    out.writeln(text.substr(0, text.find('\n')));
  }
}
```

Output

```text
subsp::math::finance::rates::hull_white_paths: steps must be > 0
subsp::math::finance::rates::hull_white_paths: dt must be finite and > 0
```

## See Also

- [`SimOptions`](SimOptions.md): the settings
- [`HullWhiteParams`](HullWhiteParams.md): the model
- [`hull_white_zcb`](hull_white_zcb.md): the closed-form bond price

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hjm_zcb_mc.html">
    <span class="label">Previous</span>
    <span class="title">hjm_zcb_mc</span>
  </a>
  <a class="next-link" href="hull_white_zcb.html">
    <span class="label">Next</span>
    <span class="title">hull_white_zcb</span>
  </a>
</div>
<!-- apiref:nav:end -->
