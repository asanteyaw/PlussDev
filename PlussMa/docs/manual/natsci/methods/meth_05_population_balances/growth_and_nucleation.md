# Growth and nucleation

[Back to Methods 5: Population balances](../meth_05_population_balances.md)

A powder, a crystal slurry, a culture of cells and a school of fish all have a size distribution: not how many individuals there are but how many there are of each size. This page follows such a distribution as its members grow, as new ones appear at the smallest size, and as the largest ones leave. You describe the density on a grid of size bins, give a growth rate at every bin edge, and `population_balance::growth_rhs` returns the rate of change of the density, which `ssprk3` advances in time.

## How it works

The unknown is a number density `n(L, t)`, the number of particles per unit volume per unit size, stored as one value per bin. The balance law says particles are neither created nor destroyed except at the ends of the size range:

```{math}
\frac{\partial n}{\partial t} + \frac{\partial (G n)}{\partial L} = 0, \qquad G(0)\, n(0, t) = B_0 ,
```

where `G(L)` is the growth rate and `B_0` is the nucleation rate: the flux of new particles entering at size zero. The grid is a `Grid` with `edges` (`B + 1` of them) and `centers` (`B`); `uniform_grid(lo, hi, bins)` makes equal bins. The discretisation is a finite-volume scheme: the flux through each interior edge is the growth rate there times the density reconstructed from the upwind side, with a limiter so that fronts do not overshoot. The lower end receives the flux `B0`, and the upper end is a free outflow, so anything that grows past the top leaves the range.

Two details decide how you call it. The growth rate is given **at the bin edges**, so it has `B + 1` entries, and a growth rate that depends on size is just a different vector. And everything accepts leading axes: a density of shape `[lanes, B]` with growth of shape `[lanes, B + 1]` is `lanes` independent problems solved by one call. The time stepper `ssprk3(f, n0, t0, t1, steps)` takes `steps` equal steps; it is third order in time and stays stable while the step respects the Courant condition `G dt / dL <= 1`.

## A size distribution sliding to the right

With a constant growth rate every particle grows by the same amount, so a profile is carried to the right without changing shape: `n(L, t) = n0(L - G t)`. The program starts a Gaussian centred at size 3, grows it at `G = 1` for two time units, and compares with the exact profile centred at 5 on grids of 100, 200 and 400 bins. It also reads the total number, which is the zeroth moment, before and after.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace pb = pluss::ma::natsci::common::population_balance;

static pluss::Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(pluss::ScalarType::Float64); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6).with_sep(""));
  // crystals of size L with a Gaussian size distribution centred at 3 and every crystal growing at G = 1:
  // the distribution slides to the right, n(L, t) = n0(L - G t)
  double previous = 0.0;
  for (std::int64_t bins : {100, 200, 400}) {
    const auto grid = pb::uniform_grid(0.0, 10.0, bins);
    std::vector<double> n0, exact;
    for (double c : grid.centers) {
      n0.push_back(std::exp(-(c - 3.0) * (c - 3.0) / 0.5));
      exact.push_back(std::exp(-(c - 5.0) * (c - 5.0) / 0.5));  // after t = 2
    }
    const pluss::Series growth = vec(std::vector<double>(static_cast<std::size_t>(bins) + 1, 1.0));  // G at the bins + 1 cell edges
    const auto rhs = [&](const pluss::Series &n) { return pb::growth_rhs(n, grid, growth, 0.0); };
    const pluss::Series n = pb::ssprk3(rhs, vec(n0), 0.0, 2.0, bins * 2);
    double error = 0.0;
    const auto host = n.host<double>();
    for (std::size_t i = 0; i < host.size(); ++i) error += (host[i] - exact[i]) * (host[i] - exact[i]) * grid.width(0);
    error = std::sqrt(error);
    out.writeln(bins, " bins: L2 error = ", error, previous > 0.0 ? "   (previous / this = " : "", previous > 0.0 ? std::to_string(previous / error) + ")" : "");
    out.writeln("        number: ", pb::moment(n, grid, 0).item<double>(), " (start ", pb::moment(vec(n0), grid, 0).item<double>(), ")");
    previous = error;
  }
}
```

Output

```text
100 bins: L2 error = 0.032584
        number: 1.253314 (start 1.253314)
200 bins: L2 error = 0.010211   (previous / this = 3.190984)
        number: 1.253314 (start 1.253314)
400 bins: L2 error = 0.003057   (previous / this = 3.340350)
        number: 1.253314 (start 1.253314)
```

Each doubling of the bin count cuts the error by a factor of about three, which is the second-order accuracy of the reconstruction (the factor tends to four on a smooth profile; a Gaussian this narrow is still being resolved on the coarsest grid). The total number `1.253314` is the same to every printed digit before and after, because the scheme moves particles between bins through edge fluxes and never creates one.

## A continuous crystallizer reaching steady state

Now crystals are born at size zero at the rate `B0 = 2` per unit time, grow at `G = 1`, and leave at the top of the range `L = 10`. The program starts from an empty vessel and reports the number of crystals, their total length (the first moment) and the density at `L = 5` at three times. Before anything leaves, the totals are known exactly: `B0 t` crystals and `B0 G t^2 / 2` of length.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

using namespace pluss::tils;
namespace pb = pluss::ma::natsci::common::population_balance;

static pluss::Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(pluss::ScalarType::Float64); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // a continuous crystallizer: new crystals of size 0 are born at rate B0 = 2 per unit time, all grow at G = 1,
  // and crystals leave through the upper end of the size range L = 10
  const double b0 = 2.0, g = 1.0;
  const std::int64_t bins = 200;
  const auto grid = pb::uniform_grid(0.0, 10.0, bins);
  const pluss::Series growth = vec(std::vector<double>(bins + 1, g));
  const auto rhs = [&](const pluss::Series &n) { return pb::growth_rhs(n, grid, growth, b0); };
  const pluss::Series empty = vec(std::vector<double>(bins, 0.0));
  for (double t : {3.0, 6.0, 15.0}) {
    const pluss::Series n = pb::ssprk3(rhs, empty, 0.0, t, static_cast<std::int64_t>(t * 100));
    out.writeln("t = ", t, ": crystals ", pb::moment(n, grid, 0).item<double>(), ", total length ", pb::moment(n, grid, 1).item<double>(),
                ", density at L = 5: ", n.select(0, bins / 2).item<double>());
  }
  out.writeln("before outflow: B0 t = ", b0 * 3.0, " crystals and B0 G t^2 / 2 = ", b0 * g * 9.0 / 2.0, " total length at t = 3");
  out.writeln("steady state: density B0 / G = ", b0 / g, " and ", b0 / g * 10.0, " crystals in the range");
}
```

Output

```text
t = 3.0000: crystals 6.0000, total length 9.0141, density at L = 5: 0.0000
t = 6.0000: crystals 12.0000, total length 36.0211, density at L = 5: 2.0000
t = 15.0000: crystals 20.0000, total length 100.0000, density at L = 5: 2.0000
before outflow: B0 t = 6.0000 crystals and B0 G t^2 / 2 = 9.0000 total length at t = 3
steady state: density B0 / G = 2.0000 and 20.0000 crystals in the range
```

At `t = 3` the vessel holds `6` crystals and `9.01` of length, in agreement with `B0 t = 6` and `B0 G t^2 / 2 = 9`; the density at `L = 5` is still zero because the first crystal has only reached size 3. By `t = 15` every crystal born more than 10 time units ago has left, the density is `B0 / G = 2` everywhere, and the vessel holds `20` crystals with `100` total length: the steady state, reached by the same call that started the transient.

## Three crystallizers in one call

The leading axis of the density and of the growth rates is a lane. The program stacks three crystallizers with growth rates `0.5`, `1` and `2` into a single `[3, 200]` density and a `[3, 201]` growth array, advances them together, and computes each lane's mean size as the first moment divided by the zeroth. With a constant rate the mean size is `3 + G t`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace pb = pluss::ma::natsci::common::population_balance;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // three crystallizers in ONE call: the leading axis is a lane, each lane has its own growth rate G = 0.5, 1, 2
  const std::int64_t bins = 200, lanes = 3;
  const auto grid = pb::uniform_grid(0.0, 20.0, bins);
  const std::vector<double> rates{0.5, 1.0, 2.0};
  std::vector<double> n0, growth;
  for (double rate : rates) {
    for (double c : grid.centers) n0.push_back(std::exp(-(c - 3.0) * (c - 3.0) / 0.5));
    for (std::int64_t i = 0; i <= bins; ++i) growth.push_back(rate);
  }
  const pluss::Series start = pluss::make_series(n0, {lanes, bins}).to(pluss::ScalarType::Float64);
  const pluss::Series g_edges = pluss::make_series(growth, {lanes, bins + 1}).to(pluss::ScalarType::Float64);
  const auto rhs = [&](const pluss::Series &n) { return pb::growth_rhs(n, grid, g_edges, 0.0); };
  const pluss::Series n = pb::ssprk3(rhs, start, 0.0, 4.0, 800);
  const pluss::Series number = pb::moment(n, grid, 0), length = pb::moment(n, grid, 1);
  const pluss::Series mean_size = length.div(number);
  for (std::int64_t k = 0; k < lanes; ++k)
    out.writeln("G = ", rates[static_cast<std::size_t>(k)], ": mean size ", mean_size.select(0, k).item<double>(), " (3 + G t = ", 3.0 + rates[static_cast<std::size_t>(k)] * 4.0, "), crystals ", number.select(0, k).item<double>());
}
```

Output

```text
G = 0.5000: mean size 4.9992 (3 + G t = 5.0000), crystals 1.2533
G = 1.0000: mean size 6.9981 (3 + G t = 7.0000), crystals 1.2533
G = 2.0000: mean size 10.9961 (3 + G t = 11.0000), crystals 1.2533
```

The three lanes finish with mean sizes `4.9992`, `6.9981` and `10.9961`, within `0.004` of `3 + G t = 5`, `7` and `11`, and every lane still holds `1.2533` crystals. A parameter sweep is therefore one call with a longer leading axis, not a loop over calls.

## Decisions

- Pick the grid to resolve the features you care about: at least 20 bins across the width of the narrowest peak. `uniform_grid` is the right default for growth; the geometric grids on the next page are for aggregation, where sizes span decades.
- Choose `steps` from the Courant number. With `steps` equal steps over `[t0, t1]` and bin width `dL`, the Courant number is `G_max (t1 - t0) / (steps dL)`; keep it at or below one. The cost is linear in `steps`.
- Put size-dependent growth into the vector of edge values: `G(L) = a (1 + b L)` is `a (1 + b e_i)` evaluated at each edge `e_i`. A growth rate that changes sign is supported: the upwind side follows the sign at each edge.
- Give the nucleation rate as a number when every lane has the same `B0`, and as a Series of shape `[lanes]` when each lane has its own.
- Read results through `moment(n, grid, k)`: the zeroth is the number of particles, the first the total length, and the ratio of consecutive moments gives mean sizes.

## Pitfalls

`growth_rhs` expects the growth rate at the `B + 1` bin edges. Passing one value per bin centre, a negative nucleation rate, a reversed size range or a NaN in the starting density each stop with an error that says what to change.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace pb = pluss::ma::natsci::common::population_balance;

static pluss::Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(pluss::ScalarType::Float64); }

int main() {
  OutputEngine out;
  const std::int64_t bins = 100;
  const auto grid = pb::uniform_grid(0.0, 10.0, bins);
  std::vector<double> n0;
  for (double c : grid.centers) n0.push_back(std::exp(-(c - 3.0) * (c - 3.0) / 0.5));
  try {  // growth rates given at the 100 bin centres instead of the 101 bin edges
    pb::growth_rhs(vec(n0), grid, vec(std::vector<double>(bins, 1.0)), 0.0);
  } catch (const pluss::Error &e) { out.writeln("growth at centres: ", e.message()); }
  try {  // a negative nucleation rate
    pb::growth_rhs(vec(n0), grid, vec(std::vector<double>(bins + 1, 1.0)), -1.0);
  } catch (const pluss::Error &e) { out.writeln("negative nucleation: ", e.message()); }
  try {  // an upper size below the lower one
    pb::uniform_grid(5.0, 1.0, 10);
  } catch (const pluss::Error &e) { out.writeln("reversed grid: ", e.message()); }
  try {  // a NaN in the starting density travels through every step and is reported when the run ends
    std::vector<double> bad = n0;
    bad[10] = std::nan("");
    const pluss::Series g = vec(std::vector<double>(bins + 1, 1.0));
    pb::ssprk3([&](const pluss::Series &n) { return pb::growth_rhs(n, grid, g, 0.0); }, vec(bad), 0.0, 1.0, 100);
  } catch (const pluss::Error &e) { out.writeln("NaN density: ", e.message()); }
}
```

Output

```text
growth at centres:  natsci: growth_rhs: growth_at_edges must have one entry per bin edge (bins + 1)
negative nucleation:  natsci: growth_rhs: nucleation_flux must be finite and non-negative
reversed grid:  natsci: population_balance: need hi > lo and at least two bins
NaN density:  natsci: ssprk3: the solution became non-finite (a NaN or infinite start, or a step too large for the growth rate)
```

The first message is the one you will meet most often: the grid has `bins` cells and `bins + 1` edges, and the growth array follows the edges. The NaN is reported when the run ends, by `ssprk3`, because checking inside every right-hand side would cost a read of the device on each call.

An explicit time step must respect the Courant condition. The program advances the same profile over two time units with 4, 20 and 200 steps and prints the Courant number and the range of the density.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace pb = pluss::ma::natsci::common::population_balance;

static pluss::Series vec(const std::vector<double> &v) { return pluss::make_series(v, {static_cast<std::int64_t>(v.size())}).to(pluss::ScalarType::Float64); }

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  // growth G = 1 on 100 bins of width 0.1 over a time span of 2: the Courant number is G dt / dh
  const std::int64_t bins = 100;
  const auto grid = pb::uniform_grid(0.0, 10.0, bins);
  std::vector<double> n0;
  for (double c : grid.centers) n0.push_back(std::exp(-(c - 3.0) * (c - 3.0) / 0.5));
  const pluss::Series g = vec(std::vector<double>(bins + 1, 1.0));
  const auto rhs = [&](const pluss::Series &n) { return pb::growth_rhs(n, grid, g, 0.0); };
  for (std::int64_t steps : {4, 20, 200}) {
    const double dt = 2.0 / static_cast<double>(steps);
    const pluss::Series n = pb::ssprk3(rhs, vec(n0), 0.0, 2.0, steps);
    out.writeln(steps, " steps: Courant number ", dt / grid.width(0), ", density range [", n.min().item<double>(), ", ", n.max().item<double>(), "]");
  }
}
```

Output

```text
4 steps: Courant number 5.0000, density range [-478187.1243, 640381.8449]
20 steps: Courant number 1.0000, density range [-0.0000, 0.9426]
200 steps: Courant number 0.1000, density range [0.0000, 0.9459]
```

With 4 steps the Courant number is `5` and the density swings between `-478187` and `640382`: the scheme has blown up without producing a NaN, so no error is raised. At 20 steps the number is `1` and the density stays between zero and `0.94`; at 200 steps it is a comfortable `0.1`. A negative density, or a maximum far above the initial one, is the sign to raise `steps`.

## Where next

[Aggregation and breakage](aggregation_and_breakage.md) adds the two processes that change the number of particles. The nucleation rate `B0` used here is the quantity that [nucleation and growth](../../chemistry/chem_03_crystallization/nucleation_and_growth.md) computes from supersaturation, so the two pages connect directly. The reference pages for `natsci::common::population_balance::Grid`, `uniform_grid`, `growth_rhs`, `ssprk3` and `moment` list every argument.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="aggregation_and_breakage.html">
    <span class="label">Next</span>
    <span class="title">Aggregation and breakage</span>
  </a>
</div>
<!-- apiref:nav:end -->
