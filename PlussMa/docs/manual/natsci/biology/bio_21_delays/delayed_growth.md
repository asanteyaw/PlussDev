# Delayed growth

[Back to Biology 21: Delay Models](../bio_21_delays.md)

Many populations respond to conditions that held some time ago, because it takes time to mature, to be born or to feel crowding. A delay can turn a stable steady state into a sustained oscillation, and you want to know how long a delay is too long. This page simulates the delayed logistic equation and the blowfly model, and computes from the roots of the characteristic equation exactly where the steady state loses stability.

## How it works

The code is in `pluss::ma::natsci::biology::population::delay` (`delay::` below). In Hutchinson's equation {math}`\dot n = r n(1 - n(t - \tau)/K)` the growth responds to the density a time {math}`\tau` ago. `delay::hutchinson_rate(n, n_delayed, r, K)` is the right-hand side, and `delay::integrate(f, history, tau, t_end, steps_per_delay)` integrates it by marching over one delay at a time from a history function, returning the states. Near the steady state the deviation obeys {math}`\dot x = -a x - b\,x(t - \tau)`, with solutions {math}`e^{\lambda t}` for the roots of

```{math}
\lambda + a + b\,e^{-\lambda\tau} = 0 .
```

`delay::rightmost_root(a, b, tau)` returns the root with the largest real part, found with the Lambert W function, as `re` and `im`; a negative real part is stability and the imaginary part is the angular frequency of the oscillation. For {math}`a = 0` the real part changes sign at {math}`\tau = \pi/(2b)`, which `delay::hutchinson_critical_delay(r)` returns.

## The delayed logistic equation

The program runs four delays for `120` time units from a constant history of `0.5` and prints the range of the density over the last third of the run.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace population = biology::population;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace delay = population::delay;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // delayed logistic growth (Hutchinson): growth responds to the density tau ago. With r = 1 the steady state loses stability at tau = pi / (2 r)
  const double r = 1.0;
  out.writeln("critical delay pi / (2 r) = ", delay::hutchinson_critical_delay(r));
  for (double tau : {0.5, 1.2, 2.0, 3.0}) {
    const auto f = [&](const pluss::Series &n, const pluss::Series &nd) { return delay::hutchinson_rate(n, nd, r, 1.0); };
    const auto history = [](double) { return vec({0.5}); };
    const auto path = delay::integrate(f, history, tau, 120.0, 50);
    double lo = 1e9, hi = -1e9;
    for (std::size_t i = path.states.size() * 2 / 3; i < path.states.size(); ++i) { const double x = flat(path.states[i])[0]; lo = std::min(lo, x); hi = std::max(hi, x); }
    out.writeln("delay ", tau, ": density over the last third of the run ranges over [", lo, ", ", hi, "]");
  }
}
```

Output

```text
critical delay pi / (2 r) = 1.5708
delay 0.5000: density over the last third of the run ranges over [1.0000, 1.0000]
delay 1.2000: density over the last third of the run ranges over [1.0000, 1.0000]
delay 2.0000: density over the last third of the run ranges over [0.0685, 2.9018]
delay 3.0000: density over the last third of the run ranges over [0.0000, 7.5817]
```

The critical delay for {math}`r = 1` is `1.5708`. With delays of `0.5` and `1.2` the density settles at the steady state `1.0000`; with `2.0` it oscillates between `0.0685` and `2.9018`, and with `3.0` between nearly zero and `7.5817`, a swing that brings the population close to extinction.

## Stability from the characteristic equation

The roots give the threshold without simulating. The program evaluates the rightmost root for six delays with {math}`a = 0` and {math}`b = 1`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace population = biology::population;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace delay = population::delay;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the linearised equation x' = -a x - b x(t - tau): the rightmost root of lambda + a + b e^{-lambda tau} = 0 decides stability (negative real part: stable)
  const auto tau = vec({0.5, 1.0, 1.5, 1.6, 2.0, 3.0});
  const auto root = delay::rightmost_root(tau.mul(0.0), tau.mul(0.0).add(1.0), tau);   // a = 0, b = 1: the delayed logistic around its steady state
  out.writeln("delay                    ", flat(tau));
  out.writeln("real part of the root    ", flat(root.re));
  out.writeln("imaginary part (angular frequency) ", flat(root.im));
  out.writeln("period of the oscillation (2 pi / frequency) ", flat(root.im.reciprocal().mul(6.283185307179586)));
}
```

Output

```text
delay                    [0.5000, 1.0000, 1.5000, 1.6000, 2.0000, 3.0000]
real part of the root    [-1.5880, -0.3181, -0.0219, 0.0082, 0.0864, 0.1557]
imaginary part (angular frequency) [1.5402, 1.3372, 1.0331, 0.9869, 0.8368, 0.6072]
period of the oscillation (2 pi / frequency) [4.0794, 4.6986, 6.0819, 6.3663, 7.5082, 10.3470]
```

The real part is negative, `-1.5880`, `-0.3181` and `-0.0219`, for delays of `0.5`, `1.0` and `1.5`, and positive, `0.0082`, `0.0864` and `0.1557`, for `1.6`, `2.0` and `3.0`: it changes sign at {math}`\pi/2 = 1.5708`, as the simulation showed. The period {math}`2\pi/\text{im}` lengthens from `4.0794` to `10.3470` as the delay grows.

## Nicholson's blowflies

Production that falls at high density gives {math}`\dot n = -\delta n + P\,n(t - \tau)e^{-n(t-\tau)/N_0}`. `delay::nicholson_rate` is the right-hand side and `delay::nicholson_equilibrium(delta, P, N0)` is the steady state {math}`N_0\ln(P/\delta)`. The program runs delays `1`, `3` and `6` with {math}`\delta = 0.2`, {math}`P = 8`, {math}`N_0 = 1`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace population = biology::population;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace delay = population::delay;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // Nicholson's blowflies: n' = -delta n + P n(t - tau) exp(-n(t - tau) / N0). The steady state is N0 ln(P / delta)
  const double death = 0.2, production = 8.0, scale = 1.0;
  out.writeln("steady state N0 ln(P / delta) = ", delay::nicholson_equilibrium(death, production, scale));
  for (double tau : {1.0, 3.0, 6.0}) {
    const auto f = [&](const pluss::Series &n, const pluss::Series &nd) { return delay::nicholson_rate(n, nd, death, production, scale); };
    const auto history = [](double) { return vec({1.0}); };
    const auto path = delay::integrate(f, history, tau, 300.0, 60);
    double lo = 1e9, hi = -1e9;
    for (std::size_t i = path.states.size() * 2 / 3; i < path.states.size(); ++i) { const double x = flat(path.states[i])[0]; lo = std::min(lo, x); hi = std::max(hi, x); }
    out.writeln("delay ", tau, ": population ranges over [", lo, ", ", hi, "]");
  }
}
```

Output

```text
steady state N0 ln(P / delta) = 3.6889
delay 1.0000: population ranges over [3.6889, 3.6889]
delay 3.0000: population ranges over [3.6888, 3.6889]
delay 6.0000: population ranges over [1.7802, 7.9915]
```

The population is steady at `3.6889`, the value {math}`N_0\ln(P/\delta)`, for delays of `1` and `3`, and oscillates between `1.7802` and `7.9915` for a delay of `6`.

## Decisions

- Use `rightmost_root` to decide whether a delay destabilises a steady state, and simulate with `integrate` only when you need the shape of the oscillation.
- Give `integrate` a history function over the whole delay interval, and enough steps per delay (at least `2`; `50` is smooth for these examples).
- Judge the long-run behaviour from the last part of the run, not the whole of it, as the programs do, since the start still carries the transient from the history.

## Pitfalls

The integrator needs a positive delay and a positive number of steps per delay, and the model has a steady state at zero density that a zero history never leaves.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace population = biology::population;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

namespace delay = population::delay;
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto f = [](const pluss::Series &n, const pluss::Series &nd) { return delay::hutchinson_rate(n, nd, 1.0, 1.0); };
  const auto history = [](double) { return vec({0.5}); };
  try { (void)delay::integrate(f, history, 0.0, 10.0, 50); } catch (const pluss::Error &e) { out.writeln("zero delay:   ", e.message()); }
  try { (void)delay::integrate(f, history, -1.0, 10.0, 50); } catch (const pluss::Error &e) { out.writeln("negative delay: ", e.message()); }
  try { (void)delay::integrate(f, history, 1.0, 10.0, 0); } catch (const pluss::Error &e) { out.writeln("no steps per delay: ", e.message()); }
  // a history of zero density never grows: the delayed logistic has n = 0 as a steady state
  const auto zero = [](double) { return vec({0.0}); };
  const auto path = delay::integrate(f, zero, 1.0, 20.0, 50);
  out.writeln("history 0: density at the end = ", flat(path.states.back())[0], " (extinction is a steady state)");
}
```

Output

```text
zero delay:   natsci: delay::integrate: need tau > 0, t_end >= 0 and at least two steps per delay
negative delay: natsci: delay::integrate: need tau > 0, t_end >= 0 and at least two steps per delay
no steps per delay: natsci: delay::integrate: need tau > 0, t_end >= 0 and at least two steps per delay
history 0: density at the end = 0.0000 (extinction is a steady state)
```

A zero, negative or too-coarse setting is rejected with the three rules in one message. The zero history is not an error, but the population stays at zero for ever: growth proportional to density cannot create individuals, so start from a positive history.

## Where next

[Biology 19: Diffusion and Fronts](../bio_19_diffusion.md) lets delayed growth spread in space, and [Biology 22: Size-Structured Populations](../bio_22_size.md) replaces the delay by structure in size.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
