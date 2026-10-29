<!-- generated from simulated_paths.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Simulated paths

[Back to Monte Carlo](../finance_04_monte_carlo.md)

Monte Carlo pricing starts from simulated paths of the asset and its variance. Each model has a path generator with the same shape:

```cpp
vol::StochVolPaths vol::heston_paths(std::int64_t steps, std::int64_t paths, double spot0, double dt, const vol::HestonParams &params,
                                     const vol::SimOptions &options, Config cfg = {}, std::uint64_t seed = 0);
vol::StochVolPaths vol::heston_paths(steps, paths, spot0, dt, params, Config cfg = {}, std::uint64_t seed = 0);   // default options
// the same pair of overloads for gbm_paths, bates_paths, sv_paths, heston_nandi_paths, sabr_paths, slv_paths, rough_heston_paths, rough_bergomi_paths

struct vol::StochVolPaths { Series spot, variance, innovation_spot, innovation_var; };
```

`vol` is `pluss::subsp::math::finance::volatility`. `spot` and `variance` are `[steps + 1, paths]` arrays whose first row is the initial value; `innovation_spot` and `innovation_var` are the `[steps, paths]` normal shocks that drove the spot and the variance, kept so that estimators can reweight them. The time step `dt` is in years, so a horizon of one year in 252 steps has `dt = 1/252`. The drift of the spot is the `mu` of the parameter struct, as an expected growth rate, so for a risk-neutral simulation set it to `r − q`.

| `SimOptions` | Meaning | Default |
|---|---|---|
| `scheme` | the discretisation of the variance: `Euler`, `Milstein`, `Milstein2`, `QuadExp`, `Transition` | `Euler` |
| `method` | the source of random numbers: `Pseudo`, `Sobol`, `Halton`, `Torus` ([Quasi-random sequences](quasi_random_sequences.md)) | `Pseudo` |
| `randomized_quasi`, `sobol_scrambling` | randomise a quasi-random sequence, and the scrambling variant (0 to 3) | false, 0 |
| `antithetic` | pair each path with its mirror image ([Variance reduction](variance_reduction.md)) | false |
| `normal_shift` | a shift added to the spot shocks, for importance sampling | 0 |
| `paths_first` | return `[paths, steps + 1]` instead of `[steps + 1, paths]` | false |
| `seed`, `offset` | the random stream, and where a quasi-random sequence starts | 0, 0 |

## What a path set looks like

The program simulates a Heston path set and checks it against what the model implies: the shapes and first rows; the variance staying above its floor; the mean variance at the end, which for the mean-reverting square-root process is exactly θ + (v₀ − θ) e^(−κT); the mean spot at the end, which for `mu = r − q` must be S₀ e^((r−q)T); and the correlation between the two shocks, which must equal ρ.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace vol = pluss::subsp::math::finance::volatility;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const double spot0 = 100.0, mu = 0.02, maturity = 1.0;
  const std::int64_t steps = 100, paths = 50000;
  vol::HestonParams params;
  params.mu = mu;
  params.v0 = 0.09;  // away from theta, so the mean reversion of the variance is visible
  const auto sim = vol::heston_paths(steps, paths, spot0, maturity / steps, params, vol::SimOptions{}, as_f64(), 7);

  out.writeln("spot", sim.spot.sizes(), " variance", sim.variance.sizes(), " innovations", sim.innovation_spot.sizes(), sim.innovation_var.sizes());
  out.writeln("first row: spot", sim.spot.select(0, 0).mean().item<double>(), " variance", sim.variance.select(0, 0).mean().item<double>());
  out.writeln("smallest variance on any path", sim.variance.min().item<double>(), "(the floor is", params.variance_floor, ")");
  const double expected_variance = params.theta + (params.v0 - params.theta) * std::exp(-params.kappa * maturity);
  out.writeln("E[V_T]: simulated", sim.variance.select(0, steps).mean().item<double>(), " exact", expected_variance);
  const auto terminal = sim.spot.select(0, steps);
  const double mean = terminal.mean().item<double>();
  const double standard_error = std::sqrt(terminal.sub(mean).pow(2.0).mean().item<double>() / paths);
  out.writeln("E[S_T]: simulated", mean, " exact", spot0 * std::exp(mu * maturity), " (standard error", standard_error, ")");
  const auto zs = sim.innovation_spot.reshape({steps * paths});
  const auto zv = sim.innovation_var.reshape({steps * paths});
  out.writeln("correlation of the shocks", zs.mul(zv).mean().item<double>() / std::sqrt(zs.pow(2.0).mean().item<double>() * zv.pow(2.0).mean().item<double>()), " (rho =", params.rho, ")");
}
```

Output

```text
spot [101, 50000]  variance [101, 50000]  innovations [100, 50000] [100, 50000]
first row: spot 100.00000  variance 0.09000
smallest variance on any path 0.00000 (the floor is 0.00000 )
E[V_T]: simulated 0.05117  exact 0.05116
E[S_T]: simulated 101.90327  exact 102.02013  (standard error 0.11075 )
correlation of the shocks -0.70024  (rho = -0.70000 )
```

Both arrays are `[101, 50000]` and the shocks `[100, 50000]`, with the first row equal to the starting values. The variance reaches its floor on some path (the smallest value is at the floor of 10⁻¹²) and never goes below it. The mean variance at the end is 0.05117 against the exact 0.05116. The mean terminal spot is 101.90 against 102.02, a difference of one standard error (0.11), and the correlation between the shocks is −0.7002 against ρ = −0.7.

## Every generator is a martingale

A risk-neutral generator must reproduce the forward: E[S_T] = S₀ e^(μT). This is the first check to run on any generator, and a failure of it was behind a bias in an earlier version of the Heston generator. The program checks every generator that has a spot price, with its default parameters except for the drift, and prints the mean, the exact value and the number of standard errors between them.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace vol = pluss::subsp::math::finance::volatility;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(3));
  const double spot0 = 100.0, mu = 0.02, maturity = 1.0;
  const std::int64_t steps = 100, paths = 30000;
  const double dt = maturity / steps, target = spot0 * std::exp(mu * maturity);
  auto report = [&](const char *name, const vol::StochVolPaths &p) {
    const auto terminal = p.spot.select(0, steps);
    const double mean = terminal.mean().item<double>();
    const double se = std::sqrt(terminal.sub(mean).pow(2.0).mean().item<double>() / paths);
    out.writeln(name, " E[S_T]", mean, " exact", target, " z =", (mean - target) / se);
  };
  vol::GbmParams gbm;          gbm.mu = mu;          report("GBM          ", vol::gbm_paths(steps, paths, spot0, dt, gbm));
  vol::HestonParams heston;    heston.mu = mu;       report("Heston       ", vol::heston_paths(steps, paths, spot0, dt, heston));
  vol::BatesParams bates;      bates.mu = mu;        report("Bates        ", vol::bates_paths(steps, paths, spot0, dt, bates));
  vol::SvParams sv;            sv.mu = mu;           report("log-variance ", vol::sv_paths(steps, paths, spot0, dt, sv));
  vol::SabrParams sabr;        sabr.mu = mu;         report("SABR         ", vol::sabr_paths(steps, paths, spot0, dt, sabr));
  vol::SlvParams slv;          slv.mu = mu;          report("local-stoch vol", vol::slv_paths(steps, paths, spot0, dt, slv));
  vol::RoughHestonParams rough; rough.mu = mu;       report("rough Heston ", vol::rough_heston_paths(steps, 10000, spot0, dt, rough));
  vol::RoughBergomiParams bergomi; bergomi.mu = mu;  report("rough Bergomi", vol::rough_bergomi_paths(steps, 10000, spot0, dt, bergomi));
}
```

Output

```text
GBM            E[S_T] 101.898  exact 102.020  z = -1.023
Heston         E[S_T] 102.152  exact 102.020  z = 1.166
Bates          E[S_T] 101.971  exact 102.020  z = -0.405
log-variance   E[S_T] 101.970  exact 102.020  z = -0.375
SABR           E[S_T] 101.870  exact 102.020  z = -1.244
local-stoch vol  E[S_T] 102.147  exact 102.020  z = 1.105
rough Heston   E[S_T] 101.900  exact 102.020  z = -1.022
rough Bergomi  E[S_T] 102.026  exact 102.020  z = 0.077
```

Every generator reproduces the forward within about one standard error: the z-values are between −1.2 and 1.2.

(The two rough generators use 10000 paths, so their standard errors are larger.)

## Options and shapes

`seed` fixes the random stream, so the same seed gives the same paths and a different seed different ones. `antithetic` makes each path's second half the mirror of its first, so the shocks average to exactly zero. `paths_first` transposes the outputs. `normal_shift` shifts the spot shocks, which is the mechanism of importance sampling. The program shows each.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/aux/output.h>

namespace vol = pluss::subsp::math::finance::volatility;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const vol::HestonParams params;
  auto terminal_mean = [&](const vol::SimOptions &options, std::uint64_t seed) {
    return vol::heston_paths(20, 2000, 100.0, 0.05, params, options, as_f64(), seed).spot.select(0, 20).mean().item<double>();
  };
  vol::SimOptions plain;
  out.writeln("seed 5 twice:", terminal_mean(plain, 5), terminal_mean(plain, 5), " seed 6:", terminal_mean(plain, 6));

  vol::SimOptions antithetic;
  antithetic.antithetic = true;
  const auto anti = vol::heston_paths(20, 2000, 100.0, 0.05, params, antithetic, as_f64(), 5);
  const auto plain_paths = vol::heston_paths(20, 2000, 100.0, 0.05, params, plain, as_f64(), 5);
  out.writeln("mean of the spot shocks over all paths: plain", plain_paths.innovation_spot.mean().item<double>(), " antithetic", anti.innovation_spot.mean().item<double>());
  out.writeln("antithetic: the first path's shocks and the mirror path's shocks add to", anti.innovation_spot.select(1, 0).add(anti.innovation_spot.select(1, 1000)).abs().max().item<double>());

  vol::SimOptions transposed;
  transposed.paths_first = true;
  out.writeln("paths_first: spot", vol::heston_paths(20, 2000, 100.0, 0.05, params, transposed, as_f64(), 5).spot.sizes(), " default", plain_paths.spot.sizes());

  vol::SimOptions shifted;
  shifted.normal_shift = 0.1;
  out.writeln("normal_shift 0.1: mean spot shock", vol::heston_paths(20, 2000, 100.0, 0.05, params, shifted, as_f64(), 5).innovation_spot.mean().item<double>());
}
```

Output

```text
seed 5 twice: 100.094106 100.094106  seed 6: 99.753035
mean of the spot shocks over all paths: plain -0.002040  antithetic 0.000000
antithetic: the first path's shocks and the mirror path's shocks add to 0.000000
paths_first: spot [2000, 21]  default [21, 2000]
normal_shift 0.1: mean spot shock 0.097960
```

The same seed gives the same paths (100.094106 twice) and another seed different ones (99.753035). With `antithetic` the spot shocks average to exactly zero, against −0.0020 for the plain set, and a path's shocks and its mirror's add to zero. `paths_first` returns `[2000, 21]` instead of `[21, 2000]`, and a `normal_shift` of 0.1 moves the mean shock to 0.0980.
