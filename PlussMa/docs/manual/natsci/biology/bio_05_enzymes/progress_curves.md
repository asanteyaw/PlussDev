# Progress curves

[Back to Biology 5: Enzymes](../bio_05_enzymes.md)

The rate laws describe an enzyme at one instant. A progress curve follows the substrate as it is used up. When the enzyme is much scarcer than the substrate, the curve has a closed form, and this page compares it with the full mass-action model of the same reaction.

## How it works

The full model is `E + S <-> C -> E + P`: enzyme and substrate form a complex that either falls apart or makes product. `mass_action_rates(k1, k_minus1, kcat)` returns the rates of the state `[S, E, C, P]`, and `dynamics::simulate` integrates them. When there is little enzyme, the complex settles quickly and the substrate obeys `dS/dt = -vmax S / (km + S)` with `vmax = kcat E` and `km = (k_minus1 + kcat) / k1`; `integrated_michaelis_menten(t, s0, vmax, km)` is the exact solution of that equation, written with the Lambert W function, so a whole curve is one call. It describes the total substrate, free plus the part held in the complex.

## The closed form against the full model

The program takes `10` for `k1`, `5` for `k_minus1` and `kcat`, `0.05` of enzyme and `2` of substrate, evaluates the closed form at five times and then integrates the full model to each of the same times, adding the free and the bound substrate.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace enzymes = biology::systems::enzymes;
namespace dynamics = biology::dynamics;

static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // E + S <-> C -> E + P with k1 = 10, k-1 = 5, kcat = 5, a little enzyme (0.05) and substrate 2.0
  const double k1 = 10.0, k_minus = 5.0, kcat = 5.0, e0 = 0.05, s0 = 2.0;
  const double km = (k_minus + kcat) / k1, vmax = kcat * e0;
  const auto times = vec({0.5, 1.0, 2.0, 4.0, 6.0});
  out.writeln("time                                 ", flat(times));
  out.writeln("closed-form substrate (Lambert W)    ", flat(enzymes::integrated_michaelis_menten(times, s0, vmax, km)));

  // the full mass-action model, integrated to each of those times
  const auto y0 = vec({s0, e0, 0.0, 0.0});
  const auto rate = enzymes::mass_action_rates(k1, k_minus, kcat);
  std::vector<double> full;
  for (double t : flat(times)) {
    const auto y = flat(dynamics::simulate(rate, y0, 0.0, t, 4000));
    full.push_back(y[0] + y[2]);   // free substrate plus the substrate held in the complex
  }
  out.writeln("mass-action substrate (free + bound) ", full);
}
```

Output

```text
time                                 [0.5000, 1.0000, 2.0000, 4.0000, 6.0000]
closed-form substrate (Lambert W)    [1.9173, 1.8357, 1.6765, 1.3748, 1.0989]
mass-action substrate (free + bound) [1.9231, 1.8419, 1.6833, 1.3830, 1.1083]
```

The two rows agree within about `0.01` at every time: `1.9173` against `1.9231` at time `0.5`, and `1.0989` against `1.1083` at time `6`. The remaining difference is the time that the real complex needs to form, which the closed form ignores. The closed form was a single call for all five times; the integration was a separate run for each.

<!-- addex:start -->
## Several starting concentrations

The closed form takes arrays for the time and for the starting concentration, so a family of progress curves is one call per set. The program evaluates four starting concentrations at four times, and then three different starting concentrations at the same time in one call.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace enzymes = biology::systems::enzymes;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // progress curves for four starting substrate concentrations at once: the closed form takes arrays for S0
  const auto t = vec({1.0, 2.0, 4.0, 8.0});
  const double vmax = 0.25, km = 1.0;
  for (double s0 : {0.5, 2.0, 5.0, 10.0}) {
    out.writeln("S0 = ", s0, ": substrate left at t = 1, 2, 4, 8 is ", flat(enzymes::integrated_michaelis_menten(t, s0, vmax, km)));
  }
  // lanes of S0 and time together
  out.writeln("S0 = 1, 3, 9 at t = 4 each: ", flat(enzymes::integrated_michaelis_menten(vec({4.0, 4.0, 4.0}), vec({1.0, 3.0, 9.0}), vmax, km)));
}
```

Output

```text
S0 = 0.5000: substrate left at t = 1, 2, 4, 8 is [0.4213, 0.3517, 0.2388, 0.1009]
S0 = 2.0000: substrate left at t = 1, 2, 4, 8 is [1.8357, 1.6765, 1.3748, 0.8526]
S0 = 5.0000: substrate left at t = 1, 2, 4, 8 is [4.7924, 4.5864, 4.1793, 3.3889]
S0 = 10.0000: substrate left at t = 1, 2, 4, 8 is [9.7730, 9.5464, 9.0949, 8.1986]
S0 = 1, 3, 9 at t = 4 each: [0.5671, 2.2761, 8.1048]
```

With {math}`V_{max} = 0.25` and {math}`K_m = 1`, a low starting concentration is used up quickly in proportion (`0.5` falls to `0.1009` by time `8`) while a high one is nearly saturated and falls almost linearly (`10` falls to `8.1986`, a loss of about `1.8`, close to {math}`V_{max} t = 2`). The last line shows the three starting concentrations `1`, `3` and `9` at time `4` giving `0.5671`, `2.2761` and `8.1048`.

## When the closed form fails

The closed form assumes the enzyme is much scarcer than the substrate. The program raises the enzyme concentration from `0.02` through `0.1` to `0.4` with a starting substrate of `2`, and compares the closed form with the mass-action model at time `4`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace enzymes = biology::systems::enzymes;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the quasi-steady-state closed form assumes little enzyme. Compare it with the full mass-action model as the enzyme amount rises
  const double k1 = 10.0, k_minus = 5.0, kcat = 5.0, s0 = 2.0, km = (k_minus + kcat) / k1;
  for (double e0 : {0.02, 0.1, 0.4}) {
    const auto y0 = vec({s0, e0, 0.0, 0.0});
    const auto y = flat(dynamics::simulate(enzymes::mass_action_rates(k1, k_minus, kcat), y0, 0.0, 4.0, 4000));
    const auto closed = flat(enzymes::integrated_michaelis_menten(vec({4.0}), s0, kcat * e0, km))[0];
    out.writeln("enzyme ", e0, ": substrate at t = 4: mass action (free + bound) = ", y[0] + y[2], ", closed form = ", closed);
  }
}
```

Output

```text
enzyme 0.0200: substrate at t = 4: mass action (free + bound) = 1.7422, closed form = 1.7395
enzyme 0.1000: substrate at t = 4: mass action (free + bound) = 0.8735, closed form = 0.8526
enzyme 0.4000: substrate at t = 4: mass action (free + bound) = 0.0146, closed form = 0.0049
```

With little enzyme (`0.02`) the two agree closely, `1.7422` and `1.7395`. As the enzyme grows, the closed form increasingly overestimates the speed: at `0.1` the mass-action substrate is `0.8735` against `0.8526`, and at `0.4`, an enzyme concentration that is a fifth of the substrate, it is `0.0146` against `0.0049`. The assumption behind the closed form is breaking down, and the full model should be used.

<!-- addex:end -->

## Decisions

- Use `integrated_michaelis_menten` when the enzyme is much scarcer than the substrate and you want a whole curve in one call; integrate `mass_action_rates` when the enzyme is a sizeable fraction of the substrate.
- Compute `km` from the elementary rates as {math}`(k_{-1} + k_{cat})/k_1` and `vmax` as {math}`k_{cat} E_0`.

## Pitfalls

The Michaelis constant is not the dissociation constant of the complex. The program uses the page's rates to compute {math}`K_m = (k_{-1} + k_{cat})/k_1` and, wrongly, {math}`k_{-1}/k_1`, and evaluates the progress curve with each.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace enzymes = biology::systems::enzymes;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the Michaelis constant is (k-1 + kcat)/k1, not the dissociation constant k-1/k1
  const double k1 = 10.0, k_minus = 5.0, kcat = 5.0, e0 = 0.05, s0 = 2.0;
  const auto times = vec({0.5, 2.0, 6.0});
  out.writeln("km = (k-1 + kcat)/k1 = ", (k_minus + kcat) / k1, ": ", flat(enzymes::integrated_michaelis_menten(times, s0, kcat * e0, (k_minus + kcat) / k1)));
  out.writeln("km = k-1/k1 (wrong)  = ", k_minus / k1, ": ", flat(enzymes::integrated_michaelis_menten(times, s0, kcat * e0, k_minus / k1)));
}
```

Output

```text
km = (k-1 + kcat)/k1 = 1.0000: [1.9173, 1.6765, 1.0989]
km = k-1/k1 (wrong)  = 0.5000: [1.9005, 1.6088, 0.8995]
```

With the right constant (`1.0000`) the substrate at times `0.5`, `2` and `6` is `1.9173`, `1.6765` and `1.0989`. With `0.5000` it is `1.9005`, `1.6088` and `0.8995`: the enzyme looks faster, and the error grows with time. The catalytic step belongs in {math}`K_m`.

## Where next

[Rate laws](rate_laws.md) shows how inhibitors change these constants, and [Biology 25: Reaction Networks](../bio_25_networks/mass_action_networks.md) builds the full model as a network.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rate_laws.html">
    <span class="label">Previous</span>
    <span class="title">Rate laws</span>
  </a>
</div>
<!-- apiref:nav:end -->
