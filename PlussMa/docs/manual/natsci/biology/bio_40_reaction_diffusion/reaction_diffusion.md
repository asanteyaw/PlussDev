# Reaction and diffusion on a grid

[Back to Biology 40: Simulating Reaction-Diffusion](../bio_40_reaction_diffusion.md)

Species in a tissue or a habitat react where they are and spread out between places. You want to follow that in space and time: how a spike of material spreads, how fast an invading population advances, and whether two interacting species form a stationary pattern. This page simulates reaction-diffusion systems on a grid with chosen boundary conditions, measures the speed of a bistable front against its exact value, and grows a pattern in a two-species system.

## How it works

The code is in `pluss::ma::natsci::biology::spatial` (`spatial::` below). A system of species {math}`u_i(x, t)` obeys

```{math}
\frac{\partial u_i}{\partial t} = D_i\,\nabla^2 u_i + f_i(u)
```

where {math}`f` is the local reaction. `spatial::box(cells, lengths, boundary)` makes a grid of one to three axes, with `spatial::Boundary::Neumann` (no flux through the wall, the default), `Dirichlet` (the value is pinned at zero outside the wall) or `Periodic`. The state is a Series of shape {math}`[\text{species}, \text{cells}\ldots]` and the diffusion coefficients a Series with one entry per species. `spatial::laplacian(u, grid)` is the discrete Laplacian, `spatial::rate(u, diffusion, grid, reaction)` the right-hand side, and `spatial::integrate(u0, diffusion, grid, reaction, t0, t1, steps)` integrates it with a fourth-order Runge-Kutta rule; `reaction` is a function from the state to {math}`f(u)`, or `nullptr` for pure diffusion. The explicit rule is stable only for steps below about {math}`1.39` times `spatial::max_stable_dt(grid, D_max)`, and `integrate` refuses to run with a larger step.

## Diffusion and the boundary

A unit amount of material is released in the middle of a line of length `10` and diffuses with {math}`D = 0.5`. The program compares walls that let nothing through with walls that absorb, printing the total amount and the standard deviation of the distribution at four times.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace biology = natsci::biology;
namespace spatial = biology::spatial;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // one species diffusing (D = 0.5) on a line of length 10 with 100 cells; start with a unit amount in the middle
  const std::int64_t cells = 100;
  const double length = 10.0, h = length / cells, D = 0.5;
  std::vector<double> u0(cells, 0.0);
  u0[cells / 2] = 1.0 / h;                                      // total amount 1
  const auto start = vec(u0).unsqueeze(0);                      // species axis first: shape [1, cells]
  const auto amount = [&](const pluss::Series &u) { return u.sum().item<double>() * h; };
  const auto spread = [&](const pluss::Series &u) {             // standard deviation of the distribution
    const auto v = flat(u); double m = 0, s2 = 0, x0 = 0;
    for (std::int64_t i = 0; i < cells; ++i) m += v[i] * h;
    for (std::int64_t i = 0; i < cells; ++i) x0 += v[i] * h * (i + 0.5) * h / m;
    for (std::int64_t i = 0; i < cells; ++i) s2 += v[i] * h * std::pow((i + 0.5) * h - x0, 2) / m;
    return std::sqrt(s2);
  };
  for (const auto &[name, kind] : {std::pair<const char *, spatial::Boundary>{"no-flux walls", spatial::Boundary::Neumann}, {"absorbing walls", spatial::Boundary::Dirichlet}}) {
    const auto grid = spatial::box({cells}, {length}, kind);
    out.writeln(name, ": ");
    for (double t : {0.0, 1.0, 4.0, 16.0}) {
      const auto u = t == 0.0 ? start : spatial::integrate(start, vec({D}), grid, nullptr, 0.0, t, static_cast<std::int64_t>(t / 0.004));
      if (t > 0 && t < 5) out.writeln("  time ", t, ": amount ", amount(u), ", spread ", spread(u), ", free-space value sqrt(2 D t) = ", std::sqrt(2 * D * t));
      else out.writeln("  time ", t, ": amount ", amount(u), ", spread ", spread(u));
    }
  }
}
```

Output

```text
no-flux walls: 
  time 0.0000: amount 1.0000, spread 0.0000
  time 1.0000: amount 1.0000, spread 1.0000, free-space value sqrt(2 D t) = 1.0000
  time 4.0000: amount 1.0000, spread 1.9592, free-space value sqrt(2 D t) = 2.0000
  time 16.0000: amount 1.0000, spread 2.8109
absorbing walls: 
  time 0.0000: amount 1.0000, spread 0.0000
  time 1.0000: amount 1.0000, spread 1.0000, free-space value sqrt(2 D t) = 1.0000
  time 4.0000: amount 0.9751, spread 1.8558, free-space value sqrt(2 D t) = 2.0000
  time 16.0000: amount 0.5777, spread 2.1739
```

With no-flux walls the total amount stays `1.0000` for ever and the distribution spreads like {math}`\sqrt{2Dt}` until the walls interfere (`1.0000` at time `1`, `1.9592` at time `4` against `2.0000`) and tends to the uniform distribution (`2.8109`, close to the uniform value {math}`L/\sqrt{12} = 2.8868`). With absorbing walls the material leaks out as soon as the distribution reaches them: `0.9751` is left at time `4` and `0.5777` at time `16`.

## A bistable front

A population with an Allee effect obeys {math}`u_t = D u_{xx} + r u(1-u)(u-a)`: the state {math}`u = 1` invades the empty state when the threshold {math}`a` is below one half, and retreats when it is above. The program starts the left half of a line of length `80` at {math}`u = 1` and measures how far the front (where {math}`u` falls through one half) moves in `10` time units, for four thresholds, against the exact speed {math}`\sqrt{rD/2}\,(1 - 2a)`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace biology = natsci::biology;
namespace spatial = biology::spatial;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a bistable (Allee) reaction u_t = D u_xx + r u (1 - u)(u - a): the state u = 1 invades u = 0 when a < 1/2 and retreats when a > 1/2, at speed sqrt(r D / 2) (1 - 2a)
  const std::int64_t cells = 400;
  const double length = 80.0, h = length / cells, D = 1.0, r = 1.0;
  const auto grid = spatial::box({cells}, {length});
  for (double a : {0.1, 0.3, 0.5, 0.7}) {
    const auto reaction = [&](const pluss::Series &u) { return u.mul(u.neg().add(1.0)).mul(u.sub(a)).mul(r); };
    std::vector<double> u0(cells, 0.0);
    for (std::int64_t i = 0; i < 200; ++i) u0[i] = 1.0;              // the left half starts at u = 1
    auto u = vec(u0).unsqueeze(0);
    const auto front = [&](const pluss::Series &s) { const auto v = flat(s); std::int64_t i = 0; while (i < cells && v[i] > 0.5) ++i; return i * h; };
    const double x0 = front(u);
    u = spatial::integrate(u, vec({D}), grid, reaction, 0.0, 10.0, 1000);
    const double x1 = front(u);
    u = spatial::integrate(u, vec({D}), grid, reaction, 10.0, 20.0, 1000);
    out.writeln("a = ", a, ": front moved from ", x0, " to ", x1, " to ", front(u), ", speed ", (front(u) - x1) / 10.0, ", theory ", std::sqrt(r * D / 2.0) * (1.0 - 2.0 * a));
  }
}
```

Output

```text
a = 0.1000: front moved from 40.0000 to 44.6000 to 50.2000, speed 0.5600, theory 0.5657
a = 0.3000: front moved from 40.0000 to 42.4000 to 45.2000, speed 0.2800, theory 0.2828
a = 0.5000: front moved from 40.0000 to 40.0000 to 40.0000, speed 0.0000, theory 0.0000
a = 0.7000: front moved from 40.0000 to 37.6000 to 34.8000, speed -0.2800, theory -0.2828
```

The front moves right for {math}`a = 0.1` and {math}`a = 0.3` (measured speeds `0.5600` and `0.2800` against `0.5657` and `0.2828`), stands still for {math}`a = 0.5` (`0.0000`) and retreats for {math}`a = 0.7` (`-0.2800` against `-0.2828`). The measured speed is within one per cent of the exact value, and its sign tells you which of the two states wins the habitat.

## A stationary pattern

In the Brusselator two species react, {math}`\dot u = a - (b+1)u + u^2 v` and {math}`\dot v = bu - u^2 v`, and the second one diffuses ten times faster. The program starts from the uniform state {math}`(a, b/a)` with a ripple of amplitude `0.01` and follows it on a periodic line of length `40`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace biology = natsci::biology;
namespace spatial = biology::spatial;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // the Brusselator, two species (u, v) with the inhibitor v diffusing 10 times faster than u, on a line of length 40 with periodic ends
  const std::int64_t cells = 200;
  const double length = 40.0, a = 2.0, b = 3.5;
  const auto grid = spatial::box({cells}, {length}, spatial::Boundary::Periodic);
  const auto reaction = [&](const pluss::Series &s) {
    const auto u = s.select(0, 0), v = s.select(0, 1);
    const auto du = u.mul(0.0).add(a).sub(u.mul(b + 1.0)).add(u.square().mul(v));
    const auto dv = u.mul(b).sub(u.square().mul(v));
    return du.unsqueeze(0).unite(std::vector<pluss::Series>{dv.unsqueeze(0)}, 0);
  };
  // start at the uniform state (a, b/a) with a small ripple
  std::vector<double> ripple(cells), vbase(cells);
  for (std::int64_t i = 0; i < cells; ++i) { ripple[i] = a + 0.01 * std::sin(2 * M_PI * 7 * i / cells) + 0.005 * std::cos(2 * M_PI * 3 * i / cells); vbase[i] = b / a; }
  auto s = vec(ripple).unsqueeze(0).unite(std::vector<pluss::Series>{vec(vbase).unsqueeze(0)}, 0);
  const auto peaks = [&](const pluss::Series &state) { const auto u = flat(state.select(0, 0)); int n = 0; for (std::int64_t i = 0; i < cells; ++i) if (u[i] > u[(i + cells - 1) % cells] && u[i] >= u[(i + 1) % cells] && u[i] > a * 1.3) ++n; return n; };
  double t = 0.0;
  for (double target : {5.0, 50.0, 150.0}) {
    s = spatial::integrate(s, vec({1.0, 10.0}), grid, reaction, t, target, static_cast<std::int64_t>((target - t) / 0.002));
    t = target;
    const auto u = flat(s.select(0, 0));
    out.writeln("time ", t, ": u ranges over [", *std::min_element(u.begin(), u.end()), ", ", *std::max_element(u.begin(), u.end()), "], peaks of u: ", peaks(s));
  }
}
```

Output

```text
time 5.000: u ranges over [1.895, 2.110], peaks of u: 0
time 50.000: u ranges over [1.021, 3.301], peaks of u: 7
time 150.000: u ranges over [0.963, 3.463], peaks of u: 7
```

At time `5` the ripple is still small (`u` between `1.895` and `2.110`). By time `50` it has grown into a pattern with seven peaks of height `3.301` and troughs of `1.021`, and at time `150` it has hardly changed (`0.963` to `3.463`): the uniform state is unstable and the system settles into a stationary pattern, with the wavelength set by the diffusion coefficients and the reaction.

## Decisions

- Choose the boundary from the physics: `Neumann` for a closed habitat, `Dirichlet` for a hostile surround, `Periodic` for a ring or to avoid edge effects.
- Use enough steps that the step is below `max_stable_dt`, and check a result by halving the cell size; the front speeds above depend on the resolution of the front.
- Test whether a reaction system forms patterns with the linear analysis of [Biology 12: Pattern Formation](../bio_12_patterns.md) before you simulate; the simulation then shows what the analysis cannot, such as the final amplitude.

## Pitfalls

The step has to be small enough for the diffusion and the state needs a species axis. The program asks for a step above the limit, repeats the run with enough steps, and passes a state without a species axis.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace natsci = pluss::ma::natsci;
namespace biology = natsci::biology;
namespace spatial = biology::spatial;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  const std::int64_t cells = 200;
  const double length = 40.0, D = 10.0;
  const auto grid = spatial::box({cells}, {length});
  std::vector<double> u0(cells, 0.0); u0[100] = 1.0 / (length / cells);
  const auto start = vec(u0).unsqueeze(0);
  out.writeln("largest stable step for D = ", D, " on this grid: ", spatial::max_stable_dt(grid, D));
  try {
    spatial::integrate(start, vec({D}), grid, nullptr, 0.0, 1.0, 10);        // dt = 0.1
  } catch (const pluss::Error &e) { out.writeln("error: ", e.message()); }
  const auto ok = spatial::integrate(start, vec({D}), grid, nullptr, 0.0, 1.0, 400);
  out.writeln("with 400 steps: amount ", ok.sum().item<double>() * grid.axes[0].spacing());
  // a one-dimensional array has no species axis: [cells] instead of [1, cells]
  try {
    spatial::integrate(vec(u0), vec({D}), grid, nullptr, 0.0, 1.0, 400);
  } catch (const pluss::Error &e) { out.writeln("error: ", e.message()); }
}
```

Output

```text
largest stable step for D = 10.000 on this grid: 0.002
error: natsci: spatial::integrate: the step is too large for the diffusion (RK4 is unstable above about 1.39 times max_stable_dt); use more steps
with 400 steps: amount 1.000
error: natsci: spatial::rate: the state needs a species axis in front of the spatial axes, shape [species, cells...]
```

The stability limit for {math}`D = 10` on this grid is `0.002`, and a step of `0.1` is refused with an error that says why. With `400` steps the run is accepted and the amount stays `1.0000`. A one-dimensional Series `[cells]` is rejected, because the first axis is read as the species axis: write `[1, cells]` for one species.

## Where next

[Biology 12: Pattern Formation](../bio_12_patterns.md) predicts which systems form patterns, [Biology 19: Diffusion and Spread](../bio_19_diffusion.md) gives the front speed and the heat kernel in closed form, and [Biology 39: Axons](../bio_39_axons.md) carries a nerve impulse along a fibre, a reaction-diffusion system of the same kind.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
