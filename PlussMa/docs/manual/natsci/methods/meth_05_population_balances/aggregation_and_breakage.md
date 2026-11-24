# Aggregation and breakage

[Back to Methods 5: Population balances](../meth_05_population_balances.md)

Particles in a slurry stick together, droplets in a spray merge, polymer chains join, and big grains crumble under stress. These processes change the number of particles, not just their sizes, and the equations are more demanding than pure growth because every pair of sizes interacts. This page solves them on a fixed grid of representative sizes (pivots) in a way that conserves the number and the mass of the particles you track, and it shows how the grid you choose controls the accuracy of the moments you care about.

## How it works

The unknown here is the count `N_i` of particles in pivot bin `i`, not a density. For aggregation of particles of sizes `x_i` and `x_j` with a rate kernel `K_ij` the balance is

```{math}
\frac{dN_k}{dt} = \frac12 \sum_{i,j} \eta_{ijk}\, K_{ij} N_i N_j \;-\; N_k \sum_j K_{kj} N_j ,
```

where `eta_ijk` is the share of a new particle of size `x_i + x_j` assigned to pivot `k`. A new particle seldom falls exactly on a pivot, so its count is split between the two pivots on either side in proportions chosen to keep the number and the mass. `build_aggregation_operator(grid, like)` builds those shares once as an array of shape `[B*B, B]`; `aggregation_rhs(counts, op, kernel)` then reads the kernel as a `[B, B]` array (or `[..., B, B]` for one kernel per lane) and returns `dN/dt` with a few matrix products and no loop over bins.

Breakage is the mirror image. `build_breakage_operator(grid, beta, like)` takes a callback `beta(x, y)`, the number of fragments of size `x` produced when a particle of size `y` breaks, integrates it over every bin by Gauss-Legendre quadrature, and shares the fragments between the enclosing pivots. `breakage_rhs(counts, op, selection)` takes a selection rate per bin (the rate at which particles of that size break). Three grids are provided: `integer_grid(K)` with pivots `1, 2, ..., K` (the assignment is then exact, as in the discrete Smoluchowski equation), `geometric_grid(x0, ratio, bins)` with `x_k = x0 ratio^k`, and `uniform_grid`. Use `pivot_moment(counts, grid, k)` for `sum_i x_i^k N_i` on these count variables; the plain `moment` belongs to densities.

## The Smoluchowski equation on integer sizes

When particles are clusters of `k` identical monomers, the pivots are the integers and every merger of a `k`-cluster with a `j`-cluster lands exactly on the pivot `k + j`. For the constant kernel `K = 1` and a start of unit monomer concentration, the solution is known in closed form: `N_k(t) = (t/2)^(k-1) / (1 + t/2)^(k+1)`, and the total number of clusters is `1 / (1 + t/2)`. The program integrates to `t = 4` on 200 pivots and prints the first five sizes, the total number and the total mass.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace pb = pluss::ma::natsci::common::population_balance;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  // clusters of k monomers on the integer grid 1, 2, ..., 200; any two clusters merge at the same rate (kernel K = 1)
  const std::int64_t bins = 200;
  const auto grid = pb::integer_grid(bins);
  const pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const pluss::Series kernel = pluss::series::ones({bins, bins}, cfg);
  const auto op = pb::build_aggregation_operator(grid, kernel);
  std::vector<double> start(bins, 0.0);
  start[0] = 1.0;  // one unit of monomers per volume
  const pluss::Series n0 = pluss::make_series(start, {bins}).to(pluss::ScalarType::Float64);
  const auto rhs = [&](const pluss::Series &n) { return pb::aggregation_rhs(n, op, kernel); };
  const double t = 4.0;
  const pluss::Series n = pb::ssprk3(rhs, n0, 0.0, t, 400);
  const double tau = t / 2.0;
  out.writeln("cluster size k : simulated  exact (t/2)^(k-1) / (1 + t/2)^(k+1)");
  for (std::int64_t k = 1; k <= 5; ++k)
    out.writeln("  k = ", k, " : ", n.select(0, k - 1).item<double>(), "    ", std::pow(tau, k - 1) / std::pow(1.0 + tau, k + 1));
  out.writeln("clusters in total: ", pb::pivot_moment(n, grid, 0).item<double>(), "   exact 1 / (1 + t/2) = ", 1.0 / (1.0 + tau));
  out.writeln("monomers in total: ", pb::pivot_moment(n, grid, 1).item<double>(), "   (start 1)");
}
```

Output

```text
cluster size k : simulated  exact (t/2)^(k-1) / (1 + t/2)^(k+1)
  k = 1 : 0.11111    0.11111
  k = 2 : 0.07407    0.07407
  k = 3 : 0.04938    0.04938
  k = 4 : 0.03292    0.03292
  k = 5 : 0.02195    0.02195
clusters in total: 0.33333   exact 1 / (1 + t/2) = 0.33333
monomers in total: 1.00000   (start 1)
```

The five simulated sizes agree with the closed form to the five printed digits, and the number of clusters is `0.33333 = 1 / (1 + 2)`. The mass, the sum of `k N_k`, stays at `1.00000`: every merger moves monomers between clusters and none disappear.

## Coarse grids keep the mass but spoil the spread

Real particle sizes span decades, and 200 integer pivots would be wasteful. The program repeats the problem on geometric grids with ratio 2, `sqrt 2` and `2^(1/4)` that reach `2^15`, and prints three moments: the number, the mass, and the second moment, which measures how wide the distribution is. For the constant kernel the second moment grows linearly, `1 + t`, so at `t = 4` it should be `5`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace pb = pluss::ma::natsci::common::population_balance;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  // the same constant-kernel problem on geometric grids x_k = ratio^k: coarser grids need far fewer bins
  const pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  const double t = 4.0;
  out.writeln("exact at t = 4: clusters ", 1.0 / (1.0 + t / 2.0), ", mass 1, second moment 1 + t = ", 1.0 + t);
  for (double ratio : {2.0, std::sqrt(2.0), std::pow(2.0, 0.25)}) {
    const std::int64_t bins = static_cast<std::int64_t>(std::round(15.0 * std::log(2.0) / std::log(ratio))) + 1;  // sizes 1 ... 2^15
    const auto grid = pb::geometric_grid(1.0, ratio, bins);
    const pluss::Series kernel = pluss::series::ones({bins, bins}, cfg);
    const auto op = pb::build_aggregation_operator(grid, kernel);
    std::vector<double> start(static_cast<std::size_t>(bins), 0.0);
    start[0] = 1.0;
    const auto rhs = [&](const pluss::Series &n) { return pb::aggregation_rhs(n, op, kernel); };
    const pluss::Series n = pb::ssprk3(rhs, pluss::make_series(start, {bins}).to(pluss::ScalarType::Float64), 0.0, t, 400);
    out.writeln("ratio ", ratio, " (", bins, " bins): clusters ", pb::pivot_moment(n, grid, 0).item<double>(), ", mass ", pb::pivot_moment(n, grid, 1).item<double>(), ", second moment ", pb::pivot_moment(n, grid, 2).item<double>());
  }
}
```

Output

```text
exact at t = 4: clusters 0.33333, mass 1, second moment 1 + t = 5.00000
ratio 2.00000 (16 bins): clusters 0.33333, mass 1.00000, second moment 5.82235
ratio 1.41421 (31 bins): clusters 0.33333, mass 1.00000, second moment 5.17311
ratio 1.18921 (61 bins): clusters 0.33333, mass 1.00000, second moment 5.04262
```

Number and mass come out exactly right on every grid, because that is what the assignment preserves. The second moment is where the grid matters: `5.82` with ratio 2, `5.17` with `sqrt 2` and `5.04` with `2^(1/4)`, against the exact `5`. A finer ratio costs more pivots (16, 31 and 61) and a four-times larger operator, but it buys accuracy in the moments that the conserved ones do not control.

## Breaking particles down

For breakage, take particles of size `y` that split at a rate of one into two fragments with sizes uniform on `(0, y)`, so `beta(x, y) = 2 / y`. Each break turns one particle into two, so the particle count should grow like `e^t` while the mass stays fixed. The program starts one particle of size 1 on three geometric grids that reach down to sizes `1/16`, `1/256` and `1/4096`, advances to `t = 1`, and compares the count with `e = 2.718`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace pb = pluss::ma::natsci::common::population_balance;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5).with_sep(""));
  // every particle of size y splits at rate S = 1 into two fragments whose sizes are uniform on (0, y): beta(x, y) = 2 / y
  // one particle of size 1 starts the run; the grid reaches down to the "floor" size
  const pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
  out.writeln("exact at t = 1: particles e = ", std::exp(1.0), ", mass 1");
  for (double floor_size : {1.0 / 16.0, 1.0 / 256.0, 1.0 / 4096.0}) {
    const std::int64_t bins = static_cast<std::int64_t>(std::round(-std::log2(floor_size) * 2.0)) + 1;  // ratio sqrt 2, pivots floor ... 1
    const auto grid = pb::geometric_grid(floor_size, std::sqrt(2.0), bins);
    const auto op = pb::build_breakage_operator(grid, [](double, double y) { return 2.0 / y; }, pluss::series::zeros({1}, cfg));
    const pluss::Series selection = pluss::series::ones({bins}, cfg);
    std::vector<double> start(static_cast<std::size_t>(bins), 0.0);
    start.back() = 1.0;
    const auto rhs = [&](const pluss::Series &n) { return pb::breakage_rhs(n, op, selection); };
    const pluss::Series n = pb::ssprk3(rhs, pluss::make_series(start, {bins}).to(pluss::ScalarType::Float64), 0.0, 1.0, 400);
    out.writeln("floor 1/", std::llround(1.0 / floor_size), " (", bins, " bins): particles ", pb::pivot_moment(n, grid, 0).item<double>(), ", mass ", pb::pivot_moment(n, grid, 1).item<double>());
  }
}
```

Output

```text
exact at t = 1: particles e = 2.71828, mass 1
floor 1/16 (9 bins): particles 2.22532, mass 1.00000
floor 1/256 (17 bins): particles 2.61101, mass 1.00000
floor 1/4096 (25 bins): particles 2.69847, mass 1.00000
```

The mass is `1.00000` on every grid, as the fragment assignment guarantees. The count depends on how far down the grid reaches: `2.23`, `2.61` and `2.70` as the floor drops from `1/16` to `1/4096`. Fragments smaller than the lowest pivot are folded into that pivot with their mass kept, so they are fewer in number than they should be; a grid that reaches further down loses fewer of them.

## Decisions

- Use `integer_grid` when the particles really are `k`-mers: the result is then exact for the sizes it holds. Use `geometric_grid` for continuous sizes, with a ratio of `2^(1/2)` as a starting point and `2^(1/4)` when moments beyond the mass matter.
- Reach down to the smallest size that is physically present, and up to a size beyond which almost no mass sits. A merger that would exceed the largest pivot is placed in the largest pivot with its number kept, so a grid that is too short silently loses mass there.
- Pass a constant kernel as an array of ones or any symmetric `[B, B]` array you build from a formula in the sizes; give a `[lanes, B, B]` array when each lane has its own kernel.
- Pick the time step from the fastest loss rate, which is about `K N_total` for aggregation and the largest selection rate for breakage. A step of a hundredth of that time is a safe start.
- Track the moment that matters for the question: number for how many, `pivot_moment(1)` as a conservation check, and the second moment for the width.

## Pitfalls

Aggregation and breakage work on counts per pivot, and the moments of counts are `pivot_moment`. Calling the density function `moment` on the same array treats each count as a density and multiplies by the bin width, which gives a different number on any grid whose bins are not one unit wide.

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
  // a monodisperse start on the geometric grid 1, 2, 4, 8, 16: one particle of size 1, stored as a COUNT per pivot bin
  const auto grid = pb::geometric_grid(1.0, 2.0, 5);
  std::vector<double> counts(5, 0.0);
  counts[0] = 1.0;
  const pluss::Series n = vec(counts);
  out.writeln("pivot_moment(counts, 1) = ", pb::pivot_moment(n, grid, 1).item<double>(), "   <- the mass: one particle of size 1");
  out.writeln("moment(counts, 1)       = ", pb::moment(n, grid, 1).item<double>(), "   <- reads the count as a density and multiplies by the bin width");
}
```

Output

```text
pivot_moment(counts, 1) = 1.0000   <- the mass: one particle of size 1
moment(counts, 1)       = 0.7500   <- reads the count as a density and multiplies by the bin width
```

One particle of size 1 has mass `1`, and `pivot_moment` reports `1.0000`. `moment` reports `0.7500`, the bin width times the value, which is the mass of a density of height one in that bin. The two functions are not interchangeable: pick by what the array holds.

## Where next

[Growth and nucleation](growth_and_nucleation.md) covers the case where the density is a smooth function of size. The aggregation here is the physical process behind the cluster-size distributions of [step-growth polymers](../../chemistry/chem_02_polymers.md), where the count of `k`-mers follows a closely related equation. The reference pages for `natsci::common::population_balance::build_aggregation_operator`, `aggregation_rhs`, `build_breakage_operator`, `breakage_rhs`, `pivot_moment` and the three grid constructors list every argument.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="growth_and_nucleation.html">
    <span class="label">Previous</span>
    <span class="title">Growth and nucleation</span>
  </a>
</div>
<!-- apiref:nav:end -->
