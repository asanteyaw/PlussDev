# Spreading and fronts

[Back to Biology 19: Diffusion and Fronts](../bio_19_diffusion.md)

You want to know how fast a released puff of molecules thins out, and how fast a growing population that also disperses takes over new ground. Molecules spread by random motion, and a population that grows as it disperses advances as a front of constant speed. This page computes the Gaussian spreading of a puff in one to three dimensions, the speed of a Fisher-KPP invasion front, and then follows a simulated front as it approaches that speed.

## How it works

Everything here lives in `pluss::ma::natsci::biology::biophysics::diffusion` (written `diffusion::` below) and `pluss::ma::natsci::biology::spatial`. A puff released at a point with diffusion coefficient {math}`D` has, in {math}`d` dimensions, the concentration

```{math}
c(r, t) = (4\pi D t)^{-d/2}\,\exp\left(-\frac{r^2}{4Dt}\right),
```

which `diffusion::heat_kernel(r2, D, t, dims)` evaluates at squared distances `r2`. Add logistic growth at rate {math}`r` and you get the Fisher-KPP equation {math}`u_t = D u_{xx} + r u(1 - u)`. Its fronts advance at a speed of at least {math}`2\sqrt{Dr}`, returned by `diffusion::fisher_kpp_min_speed(D, r)`; `fisher_kpp_exact_speed(D, r)` is the speed of one exact solution, and `fisher_kpp_exact(x, t, D, r)` is that solution. To watch a front form rather than assume it, `spatial::box(cells, lengths)` makes a grid, `diffusion::fisher_kpp_reaction(u, r)` is the growth term, and `spatial::integrate(u0, D, grid, reaction, t0, t1, steps)` advances a field laid out with one row per species and one column per cell.

## A puff of molecules

Start with the Gaussian. The program prints the concentration at four distances for two times, in one, two and three dimensions, with {math}`D = 1`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace diffusion = biology::biophysics::diffusion;
namespace spatial = biology::spatial;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  // a puff of molecules released at one point spreads as a Gaussian. Concentration (per unit volume, one molecule released) at distances from the release point, D = 1, in 1, 2 and 3 dimensions
  const auto r2 = vec({0.0, 1.0, 4.0, 9.0});
  for (double t : {0.5, 2.0}) {
    out.writeln("time ", t, ": distance^2 ", flat(r2));
    for (int dims = 1; dims <= 3; ++dims) out.writeln("    ", dims, " dimension(s): ", flat(diffusion::heat_kernel(r2, 1.0, t, dims)));
  }
}
```

Output

```text
time 0.50000: distance^2 [0.00000, 1.00000, 4.00000, 9.00000]
    1 dimension(s): [0.39894, 0.24197, 0.05399, 0.00443]
    2 dimension(s): [0.15915, 0.09653, 0.02154, 0.00177]
    3 dimension(s): [0.06349, 0.03851, 0.00859, 0.00071]
time 2.00000: distance^2 [0.00000, 1.00000, 4.00000, 9.00000]
    1 dimension(s): [0.19947, 0.17603, 0.12099, 0.06476]
    2 dimension(s): [0.03979, 0.03511, 0.02413, 0.01292]
    3 dimension(s): [0.00794, 0.00700, 0.00481, 0.00258]
```

The puff gets lower and wider. In one dimension the concentration at the release point falls from `0.39894` at time `0.5` to `0.19947` at time `2.0`, as {math}`t^{-1/2}`; in three dimensions it falls from `0.06349` to `0.00794`, as {math}`t^{-3/2}`. More dimensions give the molecules more room, so the puff thins faster.

## The speed of an invasion

Next, the front speed. The program prints the minimum speed and the speed of the exact solution for six pairs of {math}`D` and {math}`r`, and the exact front at three times.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace diffusion = biology::biophysics::diffusion;
namespace spatial = biology::spatial;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // an advantageous gene or a growing population spreads as a front: Fisher-KPP, u_t = D u_xx + r u (1 - u). Minimum and exact-solution speeds, for a few D and r
  for (double d : {0.5, 1.0, 4.0}) for (double r : {0.25, 1.0})
    out.writeln("D = ", d, ", r = ", r, ": minimum speed 2 sqrt(D r) = ", diffusion::fisher_kpp_min_speed(d, r), ", speed of the exact front = ", diffusion::fisher_kpp_exact_speed(d, r));
  // the exact front (Ablowitz-Zeppetella) for D = 1, r = 1 at times 0, 5 and 10: positions where u = 0.5
  const auto x = vec({-10.0, 0.0, 5.0, 10.0, 15.0, 20.0, 30.0});
  out.writeln("x                        ", flat(x));
  for (double t : {0.0, 5.0, 10.0}) out.writeln("front at t = ", t, "        ", flat(diffusion::fisher_kpp_exact(x, t, 1.0, 1.0)));
}
```

Output

```text
D = 0.5000, r = 0.2500: minimum speed 2 sqrt(D r) = 0.7071, speed of the exact front = 0.7217
D = 0.5000, r = 1.0000: minimum speed 2 sqrt(D r) = 1.4142, speed of the exact front = 1.4434
D = 1.0000, r = 0.2500: minimum speed 2 sqrt(D r) = 1.0000, speed of the exact front = 1.0206
D = 1.0000, r = 1.0000: minimum speed 2 sqrt(D r) = 2.0000, speed of the exact front = 2.0412
D = 4.0000, r = 0.2500: minimum speed 2 sqrt(D r) = 2.0000, speed of the exact front = 2.0412
D = 4.0000, r = 1.0000: minimum speed 2 sqrt(D r) = 4.0000, speed of the exact front = 4.0825
x                        [-10.0000, 0.0000, 5.0000, 10.0000, 15.0000, 20.0000, 30.0000]
front at t = 0.0000        [0.9671, 0.2500, 0.0132, 0.0003, 0.0000, 0.0000, 0.0000]
front at t = 5.0000        [0.9995, 0.9697, 0.7981, 0.2715, 0.0153, 0.0003, 0.0000]
front at t = 10.0000        [1.0000, 0.9995, 0.9963, 0.9721, 0.8120, 0.2938, 0.0004]
```

The minimum speed is `2.0000` for {math}`D = r = 1` and grows with both parameters, to `4.0000` for {math}`D = 4`, {math}`r = 1`; the exact front is a little faster, `2.0412`. The exact front keeps its shape while it moves: the density at `x = 0` is `0.2500` at time `0`, and the same shape reappears at `x = 10` at time `5` (`0.2715`) and at `x = 20` at time `10` (`0.2938`), ten length units per five time units, a speed of about `2`.

## Simulating a front

Now let the equation produce the front itself. The program starts a step of density one on the left of a line of length `100` with `400` cells and records where the density crosses one half.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace diffusion = biology::biophysics::diffusion;
namespace spatial = biology::spatial;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3).with_sep(""));
  // simulate the front on a line of 400 cells, length 100, started from a step of u = 1 on the left; D = 1, r = 1. The front is where u crosses one half
  const auto grid = spatial::box({400}, {100.0});
  std::vector<double> u0(400, 0.0); for (int i = 0; i < 40; ++i) u0[i] = 1.0;
  auto u = vec(u0).reshape({1, 400});    // one species, 400 cells
  const auto reaction = [](const pluss::Series &v) { return diffusion::fisher_kpp_reaction(v, 1.0); };
  const auto position = [&](const pluss::Series &field) {
    const auto f = flat(field);
    for (std::size_t i = 1; i < f.size(); ++i) if (f[i] < 0.5) return 0.25 * (i - 1 + (f[i - 1] - 0.5) / (f[i - 1] - f[i]));
    return 100.0;
  };
  double previous_time = 0.0, previous_x = position(u);
  for (double t : {10.0, 20.0, 30.0, 40.0}) {
    u = spatial::integrate(u, vec({1.0}), grid, reaction, previous_time, t, 4000);
    const double x = position(u);
    out.writeln("t = ", t, ": front at ", x, ", speed over the last 10 time units ", (x - previous_x) / (t - previous_time));
    previous_time = t; previous_x = x;
  }
  out.writeln("predicted speed 2 sqrt(D r) = ", diffusion::fisher_kpp_min_speed(1.0, 1.0));
}
```

Output

```text
t = 10.000: front at 24.383, speed over the last 10 time units 1.451
t = 20.000: front at 43.246, speed over the last 10 time units 1.886
t = 30.000: front at 62.660, speed over the last 10 time units 1.941
t = 40.000: front at 82.278, speed over the last 10 time units 1.962
predicted speed 2 sqrt(D r) = 2.000
```

The front moves at `1.451` over the first `10` time units, then `1.886`, `1.941` and `1.962`, approaching the predicted `2.000` from below. That slow approach is the known behaviour of a front started from a step: the speed converges only like {math}`1/t`.

## Decisions

- Use `heat_kernel` when the question is how far a puff has spread; use `fisher_kpp_min_speed` when you only need the speed of an invasion, and the simulation when you need the front's shape or its approach to that speed.
- Choose `steps` in `spatial::integrate` from the grid, not from the time span. The step must satisfy the RK4 diffusion limit (about {math}`1.39\,h^2/(2D)`), and `spatial::max_stable_dt(grid, D)` returns the bound to compare against.
- Measure the simulated front where the density crosses one half and interpolate between cells, as the program does; the cell index alone is only accurate to a cell width.

## Pitfalls

The functions refuse inputs that have no physical meaning. A puff needs a positive time and a positive diffusion coefficient, a growth rate must be positive, and an integration step must be small enough for the grid.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <algorithm>
#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace dynamics = biology::dynamics;
namespace diffusion = biology::biophysics::diffusion;
namespace spatial = biology::spatial;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a puff needs a positive time and a positive diffusion coefficient; a growth rate must be positive too
  try { (void)diffusion::heat_kernel(vec({1.0}), 1.0, 0.0, 2); } catch (const pluss::Error &e) { out.writeln("time 0:              ", e.message()); }
  try { (void)diffusion::heat_kernel(vec({1.0}), -1.0, 1.0, 2); } catch (const pluss::Error &e) { out.writeln("negative D:          ", e.message()); }
  try { (void)diffusion::fisher_kpp_min_speed(1.0, -0.5); } catch (const pluss::Error &e) { out.writeln("negative growth rate: ", e.message()); }
  // the explicit stepper needs steps small enough for the grid: 50 cells of width 0.2, D = 1
  const auto grid = spatial::box({50}, {10.0});
  out.writeln("largest stable step for D = 1: ", spatial::max_stable_dt(grid, 1.0), " (RK4 tolerates about 1.39 times that)");
  std::vector<double> bump(50, 0.0); bump[25] = 1.0;
  const auto no_growth = [](const pluss::Series &v) { return diffusion::fisher_kpp_reaction(v, 0.0); };
  for (std::int64_t steps : {5, 500}) {
    try {
      const auto u = spatial::integrate(vec(bump).reshape({1, 50}), vec({1.0}), grid, no_growth, 0.0, 5.0, steps);
      out.writeln(steps, " steps over 5 time units: largest density ", flat(u.max())[0]);
    } catch (const pluss::Error &e) { out.writeln(steps, " steps over 5 time units: ", e.message()); }
  }
}
```

Output

```text
time 0:              natsci: heat_kernel: D and t must be positive
negative D:          natsci: heat_kernel: D and t must be positive
negative growth rate: natsci: fisher_kpp: D and r must be positive
largest stable step for D = 1: 0.0200 (RK4 tolerates about 1.39 times that)
5 steps over 5 time units: natsci: spatial::integrate: the step is too large for the diffusion (RK4 is unstable above about 1.39 times max_stable_dt); use more steps
500 steps over 5 time units: largest density 0.0256
```

The first three are errors that name the rule. The fourth is the common slip with the stepper: `5` steps over `5` time units is far above `max_stable_dt` (`0.0200`), so it is refused rather than returning numbers that have blown up; `500` steps is stable and the bump spreads out to a largest density of `0.0256`.

## Where next

[Biology 12: Patterns](../bio_12_patterns.md) shows what diffusion does when two species react as well, and [Biology 21: Delay Models](../bio_21_delays.md) adds a delay to the growth that drives a front.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
