<!-- generated from Scheme.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `rates::Scheme`

The time-stepping scheme of the rate simulators: Euler, Milstein or the exact transition.

`Euler` advances by the drift and a Gaussian shock scaled by the diffusion at the start of the step. `Milstein` adds the second-order correction `½ σ σ′ (ΔW² − Δt)`; for Vasicek and Hull-White the diffusion is constant, so it equals Euler, and it matters for CIR (`σ√r`). `Transition` samples the exact distribution of the next rate: the Gaussian transition for Vasicek and Hull-White (no discretisation error whatever the step) and the noncentral chi-square transition for CIR (which also keeps the rate positive).

## Fields

### `Euler`

`enumerator`, default `0`.

Euler-Maruyama step.

### `Milstein`

`enumerator`, default `1`.

Euler plus the Milstein correction (a change for CIR only).

### `Transition`

`enumerator`, default `2`.

Exact transition distribution: Gaussian (Vasicek, Hull-White) or noncentral chi-square (CIR).

## Guidance

Set as `SimOptions::scheme`.

## Practical Examples

### Example 1

Euler against the exact transition for a Vasicek rate with a long step. The mean of `r_T` is `θ + (r₀ − θ)e^{−κT}`; Euler with a step of one year misses it, the transition scheme does not.

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
  const rt::VasicekParams p{.kappa = 1.5, .theta = 0.03, .sigma = 0.01, .r0 = 0.08};
  for (const rt::Scheme scheme : {rt::Scheme::Euler, rt::Scheme::Milstein, rt::Scheme::Transition}) {
    rt::SimOptions options;
    options.scheme = scheme;
    options.seed = 3;
    const rt::RatePaths sim = rt::vasicek_paths(2, 100000, 1.0, p, options);
    out.writeln(static_cast<int>(scheme), ": mean r_2 =", sim.rate.select(0, 2).mean().item<double>());
  }
  out.writeln("exact =", 0.03 + (0.08 - 0.03) * std::exp(-1.5 * 2.0));
}
```

Output

```text
0 : mean r_2 = 0.04246
1 : mean r_2 = 0.04246
2 : mean r_2 = 0.03248
exact = 0.03249
```

### Example 2

CIR with a long step and a high volatility. Both schemes keep the rate non-negative (Euler by flooring it), but the floor biases the Euler mean upward (0.046 against the exact 0.030), while the exact transition scheme has no bias.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;

namespace fin = pluss::subsp::math::finance;
namespace rt = fin::rates;
using pluss::Series;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const rt::CirParams p{.kappa = 0.5, .theta = 0.03, .sigma = 0.4, .r0 = 0.03, .rate_floor = 1e-12};
  for (const rt::Scheme scheme : {rt::Scheme::Euler, rt::Scheme::Transition}) {
    rt::SimOptions options;
    options.scheme = scheme;
    options.seed = 3;
    const rt::RatePaths sim = rt::cir_paths(4, 50000, 0.5, p, options);
    out.writeln(static_cast<int>(scheme), ": smallest rate =", sim.rate.amin().item<double>(), ", mean r_T =", sim.rate.select(0, 4).mean().item<double>(), ", exact mean =", 0.03);
  }
}
```

Output

```text
0 : smallest rate = 0.000000 , mean r_T = 0.046138 , exact mean = 0.030000
2 : smallest rate = 0.000000 , mean r_T = 0.030489 , exact mean = 0.030000
```

### Example 3

CIR with Milstein: the correction changes the path, not the mean much.

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
  const rt::CirParams p{.kappa = 0.7, .theta = 0.03, .sigma = 0.12, .r0 = 0.05, .rate_floor = 1e-12};
  for (const rt::Scheme scheme : {rt::Scheme::Euler, rt::Scheme::Milstein}) {
    rt::SimOptions options;
    options.scheme = scheme;
    options.seed = 3;
    out.writeln(static_cast<int>(scheme), ": mean r_1 =", rt::cir_paths(20, 100000, 0.05, p, options).rate.select(0, 20).mean().item<double>(), ", exact =", 0.03 + 0.02 * std::exp(-0.7));
  }
}
```

Output

```text
0 : mean r_1 = 0.039800 , exact = 0.039932
1 : mean r_1 = 0.039799 , exact = 0.039932
```

## See Also

- [`SimOptions`](SimOptions.md): holds it

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="RatePaths.html">
    <span class="label">Previous</span>
    <span class="title">RatePaths</span>
  </a>
  <a class="next-link" href="SimOptions.html">
    <span class="label">Next</span>
    <span class="title">SimOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
