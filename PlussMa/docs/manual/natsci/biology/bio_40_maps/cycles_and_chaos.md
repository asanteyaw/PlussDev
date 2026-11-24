# Cycles and chaos in population maps

[Back to Biology 40: Discrete-Time Maps and Chaos](../bio_40_maps.md)

Many populations reproduce in a single season, so the density next year is a function of the density this year. You want to know whether such a population settles at a steady density, flips between two or four values, or wanders without ever repeating, and how far the growth rate can rise before each change happens. This page iterates the standard population maps, detects the period of what they settle into, measures chaos with the Lyapunov exponent, and locates cycles and their stability exactly.

## How it works

The code is in `pluss::ma::natsci::biology::population::maps` (`maps::` below). A map $x_{t+1} = f(x_t)$ is evaluated by `maps::logistic`, `maps::ricker`, `maps::beverton_holt`, `maps::hassell` and `maps::maynard_smith_slatkin`, each with a `_derivative` companion, and a Series of parameters or starting densities is treated as a batch, so one call follows a whole range of growth rates. `maps::iterate(f, x0, steps)` returns the state after `steps` iterations, and `maps::attractor(f, x0, burn, keep)` discards `burn` iterates and stores the next `keep` of them along a new last axis. The long-run behaviour is read from that orbit: `maps::detect_period(orbit, max_period, tol)` returns the smallest $p$ for which the orbit repeats within `tol` after $p$ steps (and `0` when none does), and the Lyapunov exponent

```{math}
\lambda = \lim_{n\to\infty}\frac1n\sum_{t=0}^{n-1}\ln\left|f'(x_t)\right|
```

is computed by `maps::lyapunov(f, df, x0, burn, steps)`. It is negative on a stable cycle (nearby orbits converge), and positive when they separate exponentially, which is chaos. A cycle of period $p$ can also be found without iterating: `maps::periodic_point(f, df, guess, p)` solves $f^{p}(x) = x$ by Newton's method and returns the point and its multiplier $\prod_i f'(x_i)$, which is stable when its modulus is below one. The superstable growth rate, where the cycle passes through the critical point $f'(x_c) = 0$ and its multiplier is zero, comes from `maps::superstable_parameter(map, x_c, p, r_guess)`.

## Following five growth rates in one call

The Ricker map $x_{t+1} = x_t e^{r(1 - x_t)}$ models a population limited by crowding. The program starts five populations with different growth rates at the same density, discards `2000` iterates and detects the period of the next `64`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace maps = biology::population::maps;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // the Ricker map x -> x exp(r (1 - x)) for five growth rates, all started at 0.2 and run in one call
  const auto r = vec({1.5, 2.2, 2.55, 2.8, 3.0});
  const auto f = [&](const pluss::Series &x) { return maps::ricker(x, r); };
  const auto start = vec({0.2, 0.2, 0.2, 0.2, 0.2});
  const auto orbit = maps::attractor(f, start, 2000, 64);     // after 2000 discarded steps keep 64 iterates per growth rate
  out.writeln("growth rates r        = ", flat(r));
  out.writeln("detected period       = ", flat(maps::detect_period(orbit, 16, 1e-6)), "  (0 means no period up to 16)");
  out.writeln("last iterate          = ", flat(orbit.select(-1, 63)));
}
```

Output

```text
growth rates r        = [1.5000, 2.2000, 2.5500, 2.8000, 3.0000]
detected period       = [1.0000, 2.0000, 4.0000, 0.0000, 0.0000]  (0 means no period up to 16)
last iterate          = [1.0000, 1.5029, 1.8301, 1.1023, 2.0117]
```

At $r = 1.5$ the density settles at the carrying capacity `1.0000` (period `1`); at `2.2` it alternates between two values (period `2`); at `2.55` it cycles through four. At `2.8` and `3.0` no period up to `16` is found, and the last iterates, `1.1023` and `2.0117`, are simply points of an orbit that does not repeat: the long-run behaviour is chaotic or has a period too long to see.

## Measuring chaos

The Lyapunov exponent turns that impression into a number. The program computes it for the same five growth rates from `20000` iterates and compares with the Beverton-Holt map, whose density dependence is monotone.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace maps = biology::population::maps;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // Lyapunov exponent = long-run average of log|f'(x)|: negative for a stable cycle, positive for chaos
  const auto r = vec({1.5, 2.2, 2.55, 2.8, 3.0});
  const auto f = [&](const pluss::Series &x) { return maps::ricker(x, r); };
  const auto df = [&](const pluss::Series &x) { return maps::ricker_derivative(x, r); };
  const auto start = vec({0.2, 0.2, 0.2, 0.2, 0.2});
  out.writeln("growth rates r   = ", flat(r));
  out.writeln("Lyapunov exponent = ", flat(maps::lyapunov(f, df, start, 1000, 20000)));
  // the same for the Beverton-Holt map, which can never be chaotic
  const auto bh = [](const pluss::Series &x) { return maps::beverton_holt(x, 8.0); };
  const auto dbh = [](const pluss::Series &x) { return maps::beverton_holt_derivative(x, 8.0); };
  out.writeln("Beverton-Holt (R0 = 8) exponent = ", flat(maps::lyapunov(bh, dbh, vec({0.2}), 1000, 20000)));
}
```

Output

```text
growth rates r   = [1.5000, 2.2000, 2.5500, 2.8000, 3.0000]
Lyapunov exponent = [-0.6931, -0.7669, -0.1052, 0.2342, 0.3904]
Beverton-Holt (R0 = 8) exponent = [-2.0794]
```

At $r = 1.5$ the exponent is $\ln|1 - r| = \ln 0.5 = -0.6931$ exactly, the derivative at the fixed point. It is negative for the cycles at `2.2` and `2.55` (`-0.7669`, `-0.1052`) and positive, `0.2342` and `0.3904`, at `2.8` and `3.0`: nearby densities separate and the dynamics is chaotic. The Beverton-Holt map with $R_0 = 8$ has exponent $-\ln 8 = -2.0794$ and cannot be anything else, because it never overshoots the carrying capacity.

## Cycles and their stability, exactly

Iterating only finds stable cycles. The program solves for the fixed point and the 2-cycle of the Ricker map at $r = 2.3$ by Newton's method, reads off their multipliers, and finds the growth rates at which the logistic map $r x(1 - x)$ has superstable cycles of period `1`, `2` and `4`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace maps = biology::population::maps;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  // the fixed point and the 2-cycle of the Ricker map at r = 2.3, with their multipliers; |multiplier| < 1 means the cycle is stable
  const double r = 2.3;
  const auto f = [&](const pluss::Series &x) { return maps::ricker(x, r); };
  const auto df = [&](const pluss::Series &x) { return maps::ricker_derivative(x, r); };
  const auto fixed = maps::periodic_point(f, df, vec({0.9}), 1);
  out.writeln("fixed point ", flat(fixed.point), ", multiplier ", flat(fixed.multiplier), ", converged ", fixed.converged ? "yes" : "no");
  const auto cycle = maps::periodic_point(f, df, vec({0.4}), 2);
  out.writeln("2-cycle point ", flat(cycle.point), ", multiplier ", flat(cycle.multiplier), ", converged ", cycle.converged ? "yes" : "no");
  // superstable cycles of the logistic map x -> r x (1 - x): the growth rate at which the critical point x = 1/2 returns to itself after 1, 2 and 4 steps
  const auto map = [](const pluss::Series &x, const pluss::Series &rr) { return maps::logistic(x, rr); };
  const double guess[] = {1.9, 3.2, 3.49};
  int k = 0;
  for (int period : {1, 2, 4})
    out.writeln("superstable growth rate of period ", period, ": ", flat(maps::superstable_parameter(map, 0.5, period, vec({guess[k++]})))[0]);
}
```

Output

```text
fixed point [1.000000], multiplier [-1.300000], converged yes
2-cycle point [0.407845], multiplier [-0.164925], converged yes
superstable growth rate of period 1: 2.000000
superstable growth rate of period 2: 3.236068
superstable growth rate of period 4: 3.498562
```

The fixed point is $x = 1$ with multiplier $1 - r = -1.3000$: its modulus exceeds one, so it is unstable and the population cannot rest there. The 2-cycle through `0.407845` has multiplier `-0.164925` and attracts. The superstable growth rates are `2.000000`, `3.236068` (that is $1 + \sqrt5$) and `3.498562`, each the centre of a window in which the cycle contracts fastest.

## Decisions

- Use `attractor` plus `detect_period` to see what a map settles into over a range of parameters, `lyapunov` to tell a long cycle from chaos, and `periodic_point` when you need the cycle itself, including an unstable one, and its multiplier.
- Choose the map from the biology: Ricker and Hassell allow overcompensation and so cycles and chaos, while Beverton-Holt and Maynard Smith-Slatkin with $b \le 1$ do not.
- Put the parameter values on a leading axis and run the sweep as one batch; every function here accepts it.

## Pitfalls

Period detection reads a settled orbit. The program asks for the period at $r = 2.55$ (true period `4`) with too short a burn-in, with a tolerance tighter than the orbit has converged to, and with a sample no longer than `max_period`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace maps = biology::population::maps;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // period detection reads a settled orbit: two ways of getting it wrong, at r = 2.55 (true period 4)
  const auto f = [](const pluss::Series &x) { return maps::ricker(x, 2.55); };
  const auto start = vec({0.2});
  out.writeln("burn 2000, keep 64 : period ", flat(maps::detect_period(maps::attractor(f, start, 2000, 64), 16, 1e-6))[0]);
  out.writeln("burn 5, keep 64    : period ", flat(maps::detect_period(maps::attractor(f, start, 5, 64), 16, 1e-6))[0], "  (still in the transient)");
  out.writeln("tolerance 1e-12    : period ", flat(maps::detect_period(maps::attractor(f, start, 2000, 64), 16, 1e-12))[0]);
  try {
    maps::detect_period(maps::attractor(f, start, 2000, 8), 16, 1e-6);
  } catch (const pluss::Error &e) { out.writeln("error: ", e.message()); }
}
```

Output

```text
burn 2000, keep 64 : period 4.0000
burn 5, keep 64    : period 0.0000  (still in the transient)
tolerance 1e-12    : period 4.0000
error: natsci: maps::detect_period: the sample must be longer than max_period
```

With a burn-in of `5` the orbit is still in its transient and no period is found (`0`). With a burn-in of `2000` the answer is `4` for both `1e-6` and `1e-12`. A sample of `8` points cannot show a period of up to `16`, and the call is rejected with an error. Always discard enough iterates and check that the answer does not change when you discard more.

## Where next

[Biology 21: Delay Models](../bio_21_delays.md) shows how a delay produces the same oscillations in continuous time, [Biology 41: Host-Parasitoid Maps](../bio_41_parasitoids.md) iterates a two-species map, and [Biology 1: Growth](../bio_01_growth.md) gives the logistic growth these maps discretise.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
