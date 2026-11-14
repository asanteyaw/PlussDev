<!-- generated from short_rate_models.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Short-rate models

[Back to Interest Rates](../finance_07_interest_rates.md)

A short-rate model describes the instantaneous interest rate {math}`r_t`, the rate at which money earns interest over the next instant. From it come the price of a zero-coupon bond, {math}`P(t,T)=\mathbb{E}^{\mathbb{Q}}\big[e^{-\int_t^Tr_s\,ds}\mid r_t\big]`, and from the bonds everything else in fixed income. This page simulates the three short-rate models of the library; the next pages price bonds and build curves from them.

## The three models

All three revert to a level at speed {math}`\kappa` and differ in the noise:

```{math}
\begin{aligned}
\textbf{Vasicek:}\quad & dr_t=\kappa(\theta-r_t)\,dt+\sigma\,dW_t,\\[2pt]
\textbf{CIR:}\quad & dr_t=\kappa(\theta-r_t)\,dt+\sigma\sqrt{r_t}\,dW_t,\\[2pt]
\textbf{Hull-White:}\quad & dr_t=\kappa\big(\theta(t)-r_t\big)\,dt+\sigma\,dW_t,\qquad \theta(t)=\theta_0+\theta_{\text{slope}}\,t.
\end{aligned}
```

In the Vasicek model the noise has constant size, so the rate is Gaussian and can be negative; given {math}`r_s`, the rate at {math}`t=s+\Delta` is normal with

```{math}
\mathbb{E}[r_t\mid r_s]=\theta+(r_s-\theta)e^{-\kappa\Delta},\qquad
\operatorname{Var}[r_t\mid r_s]=\frac{\sigma^2}{2\kappa}\big(1-e^{-2\kappa\Delta}\big).
```

In the CIR model the noise shrinks with the rate, so the rate cannot become negative; it stays strictly positive when the Feller condition {math}`2\kappa\theta\ge\sigma^2` holds. Its transition law is a scaled non-central chi-squared, with mean {math}`\theta+(r_s-\theta)e^{-\kappa\Delta}` and variance

```{math}
\operatorname{Var}[r_t\mid r_s]=r_s\frac{\sigma^2}{\kappa}\big(e^{-\kappa\Delta}-e^{-2\kappa\Delta}\big)+\frac{\theta\sigma^2}{2\kappa}\big(1-e^{-\kappa\Delta}\big)^2.
```

Hull-White is Vasicek with a time-dependent level {math}`\theta(t)`, which is what allows a model to be fitted to an initial curve: here the level is linear in time, {math}`\theta_0+\theta_{\text{slope}}\,t`, and the mean rate follows it,

```{math}
\mathbb{E}[r_t]=r_0e^{-\kappa t}+\theta_0\big(1-e^{-\kappa t}\big)+\theta_{\text{slope}}\Big(t-\frac{1-e^{-\kappa t}}{\kappa}\Big).
```

| Parameters | `VasicekParams` | `CirParams` | `HullWhiteParams` |
|---|---|---|---|
| speed of reversion {math}`\kappa` | `kappa` (0.7) | `kappa` (0.7) | `kappa` (0.7) |
| level {math}`\theta` | `theta` (0.03) | `theta` (0.03) | `theta0` (0.03) and `theta_slope` (0) |
| volatility {math}`\sigma` | `sigma` (0.01) | `sigma` (0.12) | `sigma` (0.01) |
| starting rate {math}`r_0` | `r0` (0.03) | `r0` (0.03) | `r0` (0.03) |
| floor | | `rate_floor` (10⁻¹²) | |

Rates are annual and continuously compounded, and time is in years: a step of `dt = 0.25` is a quarter.

## The calls

```cpp
rates::RatePaths rates::vasicek_paths(std::int64_t steps, std::int64_t paths, double dt, const rates::VasicekParams &params, const rates::SimOptions &options, Config cfg = {}, std::uint64_t seed = 0);
rates::RatePaths rates::vasicek_paths(std::int64_t steps, std::int64_t paths, double dt, const rates::VasicekParams &params, Config cfg = {}, std::uint64_t seed = 0);
rates::RatePaths rates::cir_paths(std::int64_t steps, std::int64_t paths, double dt, const rates::CirParams &params, const rates::SimOptions &options, Config cfg = {}, std::uint64_t seed = 0);
rates::RatePaths rates::cir_paths(std::int64_t steps, std::int64_t paths, double dt, const rates::CirParams &params, Config cfg = {}, std::uint64_t seed = 0);
rates::RatePaths rates::hull_white_paths(std::int64_t steps, std::int64_t paths, double dt, const rates::HullWhiteParams &params, const rates::SimOptions &options, Config cfg = {}, std::uint64_t seed = 0);
rates::RatePaths rates::hull_white_paths(std::int64_t steps, std::int64_t paths, double dt, const rates::HullWhiteParams &params, Config cfg = {}, std::uint64_t seed = 0);
```

The overloads without `options` use the defaults of `SimOptions`. The result is a `RatePaths` with two `Series`: `rate`, of shape `[steps + 1, paths]` (row 0 is {math}`r_0`), and `innovation`, the standard normals that drove the steps, of shape `[steps, paths]`. Simulation is in `Float64`.

| `SimOptions` field | Meaning | Default |
|---|---|---|
| `scheme` | `Euler`, `Milstein` (CIR only) or `Transition`, the exact step | `Euler` |
| `method` | `Pseudo`, `Sobol`, `Halton` or `Torus` random numbers | `Pseudo` |
| `randomized_quasi`, `sobol_scrambling` | randomise a quasi-random sequence; scrambling 0 to 3 for Sobol | `false`, 0 |
| `paths_first` | return `[paths, steps + 1]` and `[paths, steps]` instead | `false` |
| `seed`, `offset` | seed of the random numbers and starting point of a quasi-random sequence | 0, 0 |

## Program 1: shapes

The program simulates four steps of a quarter year for three Vasicek paths, and prints the shapes and one path. It then asks for `paths_first`, which transposes everything so that row {math}`i` is a path, the layout of the equity `Field` of Chapter 6.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace rt = pluss::subsp::math::finance::rates;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rt::VasicekParams params{.kappa = 0.7, .theta = 0.03, .sigma = 0.01, .r0 = 0.05};
  const auto sample = rt::vasicek_paths(4, 3, 0.25, params, as_f64());
  out.writeln("rate", sample.rate.sizes(), " innovation", sample.innovation.sizes());
  out.writeln("path 0", sample.rate.select(1, 0).items<double>());
  out.writeln("first row is r0:", sample.rate.select(0, 0).items<double>());

  rt::SimOptions options;
  options.paths_first = true;
  const auto by_path = rt::vasicek_paths(4, 3, 0.25, params, options, as_f64());
  out.writeln("paths_first: rate", by_path.rate.sizes(), " innovation", by_path.innovation.sizes());
}
```

Output

```text
rate [5, 3]  innovation [4, 3]
path 0 [0.05000, 0.04844, 0.04877, 0.05231, 0.04728]
first row is r0: [0.05000, 0.05000, 0.05000]
paths_first: rate [3, 5]  innovation [3, 4]
```

Row 0 of `rate` is {math}`r_0=0.05` for every path, there are `steps + 1 = 5` rows and `paths = 3` columns, and `innovation` has one row per step. With `paths_first` every array is transposed, `[3, 5]` and `[3, 4]`. The choice affects only the layout: the numbers are the same.

## Program 2: how the schemes differ

The three schemes of `SimOptions::scheme` are:

- **`Euler`** takes the first-order step {math}`r_{t+\Delta}=r_t+\kappa(\theta-r_t)\Delta+\sigma\sqrt{\Delta}\,Z`. It is exact only as {math}`\Delta\to0`.
- **`Transition`** samples the exact law of {math}`r_{t+\Delta}` given {math}`r_t`: the normal above for Vasicek and Hull-White, the scaled non-central chi-squared for CIR. It has no discretisation error, whatever the step.
- **`Milstein`** adds the second-order term {math}`\tfrac14\sigma^2(\Delta W^2-\Delta)` to the CIR step. For Vasicek and Hull-White the noise is additive, the correction is zero, and `Milstein` is the same as `Euler`.

The program simulates a Vasicek rate to two years with 2, 4, 8 and 32 steps, 400,000 paths each, and compares the mean and the variance of the terminal rate with the exact values.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace rt = pluss::subsp::math::finance::rates;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const rt::VasicekParams p{.kappa = 0.7, .theta = 0.03, .sigma = 0.01, .r0 = 0.05};
  const double horizon = 2.0;
  const double mean = p.theta + (p.r0 - p.theta) * std::exp(-p.kappa * horizon);
  const double variance = p.sigma * p.sigma * (1.0 - std::exp(-2.0 * p.kappa * horizon)) / (2.0 * p.kappa);
  out.writeln("exact mean", mean, " variance", variance);
  out.writeln("steps  step    Euler mean  Euler var   Transition mean  Transition var");
  for (const std::int64_t steps : {2, 4, 8, 32}) {
    std::string row = std::to_string(steps) + "  " + std::to_string(horizon / static_cast<double>(steps)).substr(0, 5) + "   ";
    for (const auto scheme : {rt::Scheme::Euler, rt::Scheme::Transition}) {
      rt::SimOptions options;
      options.scheme = scheme;
      options.seed = 5;
      const auto r = rt::vasicek_paths(steps, 400000, horizon / static_cast<double>(steps), p, options, as_f64()).rate.select(0, steps);
      row += std::to_string(r.mean().item<double>()).substr(0, 8) + "    " + std::to_string(r.var().item<double>()).substr(0, 8) + "    ";
    }
    out.writeln(row);
  }
}
```

Output

```text
exact mean 0.034932  variance 0.000067
steps  step    Euler mean  Euler var   Transition mean  Transition var
2  1.000   0.031786    0.000109    0.034919    0.000067    
4  0.500   0.033587    0.000084    0.034945    0.000067    
8  0.250   0.034303    0.000075    0.034943    0.000067    
32  0.062   0.034776    0.000069    0.034929    0.000067    
```

The exact terminal mean is 0.034932 and the variance 0.000067. `Transition` reproduces both at every step size (0.03492 to 0.03494 for the mean, and the variance to the digits shown), even with a single step of a year; the sampling error of the mean is {math}`0.0082/\sqrt{400000}=1.3\times10^{-5}`. `Euler` is biased in the mean by −0.0031, −0.0013, −0.0006 and −0.0002 as the step falls from 1 to 0.06, an error proportional to the step; and its variance is 63% too large at a step of 1 and 3% too large at 0.06. The rule: use `Transition` for the Gaussian models, where it costs the same as `Euler`, and use `Euler` only when you want the discretisation of a larger simulation to be the same as the one you compare with.

## Program 3: CIR and the Feller condition

For the CIR model the quantity {math}`2\kappa\theta/\sigma^2` decides the behaviour at zero. Above 1 the rate never reaches 0. Below 1 the exact process reaches zero and reflects, so a share of paths sits near 0. A discretisation can stay positive only by clamping: `rate_floor` is the clamp. The program simulates two cases to two years in 80 steps, one with the condition satisfied (ratio 6.6) and one violated (ratio 0.13), under all three schemes, and prints the mean, the variance, and the share of paths within {math}`10^{-9}` of zero.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace rt = pluss::subsp::math::finance::rates;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const double horizon = 2.0;
  const auto theory = [&](const rt::CirParams &p) {
    const double e = std::exp(-p.kappa * horizon);
    return std::pair{p.theta + (p.r0 - p.theta) * e,
                     p.r0 * p.sigma * p.sigma / p.kappa * (e - e * e) + p.theta * p.sigma * p.sigma / (2.0 * p.kappa) * (1.0 - e) * (1.0 - e)};
  };
  out.writeln("case               2 kappa theta / sigma^2   scheme      mean       variance   share of paths at the floor");
  const std::vector<std::pair<const char *, rt::CirParams>> cases{{"Feller holds       ", {.kappa = 0.7, .theta = 0.03, .sigma = 0.08, .r0 = 0.03}},
                                                                    {"Feller violated   ", {.kappa = 0.2, .theta = 0.03, .sigma = 0.30, .r0 = 0.03}}};
  for (const auto &[name, p] : cases) {
    const auto [mean, variance] = theory(p);
    out.writeln(name, 2.0 * p.kappa * p.theta / (p.sigma * p.sigma), "   exact       ", mean, variance);
    for (const auto scheme : {rt::Scheme::Euler, rt::Scheme::Milstein, rt::Scheme::Transition}) {
      rt::SimOptions options;
      options.scheme = scheme;
      options.seed = 7;
      const auto r = rt::cir_paths(80, 300000, horizon / 80.0, p, options, as_f64()).rate.select(0, 80);
      out.writeln("                                       ", scheme == rt::Scheme::Euler ? "Euler      " : scheme == rt::Scheme::Milstein ? "Milstein   " : "Transition ",
                  r.mean().item<double>(), r.var().item<double>(), (r <= 1.0e-9).to(pluss::ScalarType::Float64).mean().item<double>());
    }
  }
}
```

Output

```text
case               2 kappa theta / sigma^2   scheme      mean       variance   share of paths at the floor
Feller holds        6.562500    exact        0.030000 0.000129
                                        Euler       0.029978 0.000129 0.000000
                                        Milstein    0.029978 0.000129 0.000000
                                        Transition  0.029976 0.000128 0.000000
Feller violated    0.133333    exact        0.030000 0.003717
                                        Euler       0.033028 0.003687 0.088807
                                        Milstein    0.032577 0.003707 0.204397
                                        Transition  0.029982 0.003722 0.072363
```

With the condition satisfied (ratio 6.56) the three schemes agree with the exact mean, 0.0300, and variance, 0.000129, and no path comes near zero. With it violated (ratio 0.13) they separate. `Transition` reproduces the exact mean (0.029982 for 0.030000) and variance (0.003722 for 0.003717), and 7.2% of its paths are at zero: that is the true law, which has probability mass near zero. `Euler` has a mean of 0.0330, 10% too high, because the clamp at the floor adds the amount that was cut off, and `Milstein` 0.0326; `Milstein` puts the most paths, 20%, at the floor. So with a CIR rate that is not far from zero, `Transition` is the only scheme that is both exact and positive; it is the default to reach for, and it is the one to use in a calibration or a bond price.

## Program 4: Hull-White and a moving level

With `theta_slope` the level of reversion moves, and the mean rate follows the formula above. The program compares the terminal mean and standard deviation of 300,000 Hull-White paths over three years with that formula, for a slope of 0 and of 0.01 (a level that rises by one percentage point a year), under `Euler` and `Transition`. The `Transition` step uses the level {math}`\theta(t)` at the middle of the step, which is exact for a level that is linear in time up to second order in the step.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace rt = pluss::subsp::math::finance::rates;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const double kappa = 0.7, theta0 = 0.03, sigma = 0.01, r0 = 0.02;
  out.writeln("slope   scheme      mean r_T    exact       std r_T     exact");
  for (const double slope : {0.0, 0.01}) {
    const double T = 3.0, e = std::exp(-kappa * T);
    const double mean = r0 * e + theta0 * (1.0 - e) + slope * (T - (1.0 - e) / kappa);
    const double sd = sigma * std::sqrt((1.0 - e * e) / (2.0 * kappa));
    for (const auto scheme : {rt::Scheme::Euler, rt::Scheme::Transition}) {
      rt::SimOptions options;
      options.scheme = scheme;
      options.seed = 3;
      const auto r = rt::hull_white_paths(120, 300000, T / 120.0, rt::HullWhiteParams{.kappa = kappa, .sigma = sigma, .r0 = r0, .theta0 = theta0, .theta_slope = slope}, options, as_f64()).rate.select(0, 120);
      out.writeln(slope, scheme == rt::Scheme::Euler ? "Euler      " : "Transition ", r.mean().item<double>(), mean, r.std().item<double>(), sd);
    }
  }
}
```

Output

```text
slope   scheme      mean r_T    exact       std r_T     exact
0.000000 Euler       0.028783 0.028775 0.008426 0.008388
0.000000 Transition  0.028761 0.028775 0.008387 0.008388
0.010000 Euler       0.046215 0.046239 0.008426 0.008388
0.010000 Transition  0.046224 0.046239 0.008387 0.008388
```

The mean rate three years ahead is 0.028775 for no slope, and 0.046239 for a slope of 0.01: the level rises to 0.06 by year three, and the rate, pulled towards it with speed 0.7, follows to within 1.4 points of it. Both schemes reproduce these (0.04622 for both against 0.046239, within about 1.5 sampling errors of 1.5×10⁻⁵ each) and the exact standard deviation 0.008388, of which `Transition` gives 0.008387 and `Euler` 0.008426 (0.5% too large). Given that a moving level is the point of a Hull-White model, it is worth knowing that the mean reproduces it in both schemes.

## Program 5: quasi-random numbers

A bond price is an integral over the rate paths, so it can be computed with low-discrepancy sequences, in which one point of the sequence is one path and the dimensions are the steps. The program prices a three-year zero-coupon bond under Vasicek with 12 steps and 4096 paths, repeated for 6 seeds, with pseudo-random numbers, an unscrambled Sobol sequence, and a randomised Sobol sequence, and prints the root-mean-square error against the closed form. The path integral {math}`\int_0^Tr\,dt` uses the trapezoid rule along each path.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace rt = pluss::subsp::math::finance::rates;
namespace vol = pluss::subsp::math::finance::volatility;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(7));
  const rt::VasicekParams p{.kappa = 0.7, .theta = 0.03, .sigma = 0.01, .r0 = 0.05};
  const std::int64_t steps = 12, paths = 4096;
  const double T = 3.0, dt = T / steps, exact = rt::vasicek_zcb(T, p.r0, p);
  out.writeln("exact discount factor", exact);
  out.writeln("method                    error over 6 seeds (root mean square)");
  const std::vector<std::tuple<const char *, vol::RandMethod, bool>> methods{{"pseudo-random             ", vol::RandMethod::Pseudo, false},
                                                                              {"Sobol, unscrambled       ", vol::RandMethod::Sobol, false},
                                                                              {"Sobol, randomised        ", vol::RandMethod::Sobol, true}};
  for (const auto &[name, method, randomised] : methods) {
    double sum_sq = 0.0;
    int runs = 0;
    for (std::uint64_t seed = 1; seed <= 6; ++seed) {
      rt::SimOptions options;
      options.scheme = rt::Scheme::Transition;
      options.method = method;
      options.randomized_quasi = randomised;
      options.sobol_scrambling = randomised ? 3 : 0;
      options.seed = seed;
      const auto rate = rt::vasicek_paths(steps, paths, dt, p, options, as_f64()).rate;
      const auto integral = (rate.sum(0, false) - (rate.select(0, 0) + rate.select(0, steps)) * 0.5) * dt;  // trapezoid rule along each path
      const double estimate = (integral * -1.0).exp().mean().item<double>();
      sum_sq += (estimate - exact) * (estimate - exact);
      ++runs;
    }
    out.writeln(name, std::sqrt(sum_sq / runs));
  }
}
```

Output

```text
exact discount factor 0.8914102
method                    error over 6 seeds (root mean square)
pseudo-random              0.0002160
Sobol, unscrambled        0.0000480
Sobol, randomised         0.0002045
```

The unscrambled Sobol sequence has an error of 4.8×10⁻⁵, a fifth of the pseudo-random 2.2×10⁻⁴. This is the benefit of a low-discrepancy set on a smooth integrand with moderate dimension (12 here). The randomised sequence has the error of the pseudo-random one, 2.0×10⁻⁴, with the advantage that repeated runs with different seeds are independent, so an error bar is available; the deterministic sequence has none. The trapezoid rule matters: with the rectangle rule, summing the rates at the left end of each step, the quadrature bias, {math}`\tfrac12\Delta t\,(r_0-r_T)\approx0.002` in the discount factor, is ten times the sampling error and hides the comparison.

## Checklist

1. Use `Transition` for Vasicek and Hull-White, and for CIR above all when {math}`2\kappa\theta<\sigma^2`.
2. Check the Feller ratio {math}`2\kappa\theta/\sigma^2` of a CIR model before reading anything into paths that reach zero.
3. Check a new generator against the exact mean and variance of the terminal rate, as in Programs 2 to 4.
4. Integrate along a path with the trapezoid rule, not the left-endpoint sum, when the step is large.
5. Use `paths_first` when the paths are the rows of the next step of your workflow.
6. Time is in years and rates are annual and continuously compounded; convert quoted rates before using them as `r0`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="zero_coupon_bonds.html">
    <span class="label">Next</span>
    <span class="title">Zero-coupon bonds and yield curves</span>
  </a>
</div>
<!-- apiref:nav:end -->
