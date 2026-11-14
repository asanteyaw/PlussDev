<!-- generated from forward_rate_models.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Forward-rate models

[Back to Interest Rates](../finance_07_interest_rates.md)

A short-rate model has one number as its state, the rate. A forward-rate model, in the framework of Heath, Jarrow and Morton (HJM), takes the whole curve of forward rates {math}`f(t,T)`, the rate agreed at time {math}`t` for borrowing instantaneously at {math}`T`, and lets it evolve. The curve at time 0 is an input rather than an output, so the model starts from the market. Zero-coupon bond prices are the exponential of the integrated forward curve,

```{math}
P(t,T)=\exp\Big(-\int_t^Tf(t,u)\,du\Big),\qquad r_t=f(t,t).
```

In the library the forward curve is held on a **grid of tenors** {math}`\tau_1<\dots<\tau_n` (the times to maturity of the forwards that are followed), with one volatility {math}`\sigma_j` per tenor and a single source of randomness shared by all of them, so the model has one factor. The curve is described by the time to maturity {math}`\tau`, the Musiela parametrisation {math}`f(t,\tau)=f(t,t+\tau)`, so that the grid is fixed while calendar time passes. Under the pricing measure the curve moves by

```{math}
df(t,\tau)=\Big[\frac{\partial f}{\partial\tau}+\sigma(\tau)\int_0^{\tau}\sigma(u)\,du\Big]dt+\sigma(\tau)\,dW_t .
```

Both terms of the drift matter. The first is the **transport**: as time passes the forward for a given calendar date comes nearer, so the curve slides along the tenor axis, and a sloped curve changes at every tenor even without noise. The second is the **no-arbitrage drift** of Heath, Jarrow and Morton, fixed by the volatility: it is what makes discounted zero-coupon bond prices martingales, so that a flat starting curve prices bonds at {math}`e^{-r_0T}` exactly. The library adds a node at {math}`\tau=0`, whose volatility is that of the first tenor and whose forward is the short rate {math}`r_t=f(t,0)`, takes {math}`\sigma` piecewise constant on each tenor interval (equal to the value at its right end), approximates the slope by a forward difference along the grid, and steps explicitly:

```{math}
f_j\;\leftarrow\;f_j+\Big[\frac{f_{j+1}-f_j}{\tau_{j+1}-\tau_j}+\sigma_j\sum_{i\le j}\sigma_i(\tau_i-\tau_{i-1})\Big]\Delta t+\sigma_j\sqrt{\Delta t}\;Z,\qquad Z\sim\mathcal N(0,1),
```

with the same {math}`Z` for every {math}`j`. The explicit transport is stable only if the time step is not larger than the smallest spacing of the tenor grid, and the call refuses a larger step.

```cpp
rates::HjmPaths rates::hjm_paths(std::int64_t steps, std::int64_t paths, double dt, const rates::HjmParams &params, const rates::SimOptions &options, Config cfg = {}, std::uint64_t seed = 0);
rates::HjmPaths rates::hjm_paths(std::int64_t steps, std::int64_t paths, double dt, const rates::HjmParams &params, Config cfg = {}, std::uint64_t seed = 0);
double rates::hjm_zcb_mc(double maturity, const rates::HjmParams &params, std::int64_t paths = 20000, std::int64_t steps = 252, rates::SimOptions options = {}, Config cfg = {});
```

| `HjmParams` | Meaning | Default |
|---|---|---|
| `r0` | the initial forward rate, the same at every tenor: a flat curve | 0.03 |
| `tenors` | the grid, in years, strictly increasing and positive | 0.25, 0.5, 1, 2, 3, 5 |
| `vol` | the volatility of the forward at each tenor, non-negative, one per tenor | 0.012, 0.011, 0.010, 0.009, 0.008, 0.007 |

| `HjmPaths` member | Shape | Contents |
|---|---|---|
| `rate` | `[steps + 1, paths]` | the short rate of each path: the forward at zero time to maturity |
| `forward` | `[steps + 1, paths, tenors]` | the forward curve of each path at each date |
| `innovation` | `[steps, paths]` | the standard normals that drove the steps |

With `paths_first` the first two axes of the first two swap, and `forward` becomes `[paths, steps + 1, tenors]`.

## Program 1: the state of the model

The program simulates four paths for a year in weekly steps on a grid of five tenors and prints the shapes, the initial curve, the curve of one path after a year, and the short rate of the same path at the start and the end.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace rt = pluss::subsp::math::finance::rates;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(5));
  const rt::HjmParams params{.r0 = 0.03, .tenors = {0.25, 0.5, 1.0, 2.0, 5.0}, .vol = {0.012, 0.011, 0.010, 0.009, 0.007}};
  const auto sim = rt::hjm_paths(52, 4, 1.0 / 52.0, params, as_f64());
  out.writeln("rate", sim.rate.sizes(), " forward", sim.forward.sizes(), " innovation", sim.innovation.sizes());
  out.writeln("forward curve of path 0 at t = 0   ", sim.forward.select(0, 0).select(0, 0).items<double>());
  out.writeln("forward curve of path 0 after a year", sim.forward.select(0, 52).select(0, 0).items<double>());
  out.writeln("rate of path 0 at the same times    ", sim.rate.select(1, 0).select(0, 0).item<double>(), sim.rate.select(1, 0).select(0, 52).item<double>(), " (the first tenor)");
}
```

Output

```text
rate [53, 4]  forward [53, 4, 5]  innovation [52, 4]
forward curve of path 0 at t = 0    [0.03000, 0.03000, 0.03000, 0.03000, 0.03000]
forward curve of path 0 after a year [0.04711, 0.04645, 0.04549, 0.04418, 0.04102]
rate of path 0 at the same times     0.03000 0.04779  (the first tenor)
```

The forward array has shape `[53, 4, 5]`: 53 dates (a year of 52 weekly steps, plus the start), four paths, five tenors. The curve at time 0 is flat at 0.03 for every tenor, as `r0` says. After a year the path's curve has moved to 0.0471 at the first tenor and 0.0410 at the fifth: the shocks are largest at short tenors, where the volatility is highest, so the curve has tilted. The short rate of the path is the forward at zero time to maturity, the node the library adds in front of the first tenor: 0.03 at the start and 0.0478 a year later, a little above the 0.25-year forward because the curve is rising towards the short end.

## Program 2: one factor, and what the grid costs

All tenors share one shock, so the model is one-dimensional: the forwards of all maturities move together. After a year, the forward at time to maturity {math}`\tau` has absorbed the shocks of the whole year, each carried along the tenor axis to its present position, so its standard deviation is {math}`\sqrt{\int_0^1\sigma(\tau+u)^2du}`, which averages the volatility curve over {math}`(\tau,\tau+1)` and is not simply {math}`\sigma(\tau)`. The program simulates 100,000 paths for a year on the coarse grid of five tenors and on a fine grid of twenty (every quarter-year to five years, with the volatility interpolated), and prints for each of a few tenors the simulated standard deviation of the forward next to that exact value, and the correlation between the first and last tenor on the coarse grid. The scheme moves the curve along the grid by linear interpolation, which smooths a little (a numerical diffusion of order half the tenor spacing), so the coarse grid overstates the spread; the fine grid should be close to exact.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

namespace rt = pluss::subsp::math::finance::rates;
using namespace pluss::tils;

// the exact standard deviation of f(1, tau) for the piecewise-constant volatility used by the library (the value at the right end of
// each tenor interval, and the last value beyond the grid)
double exact_sd(const rt::HjmParams &p, double tau) {
  const auto sigma_at = [&](double x) {
    for (std::size_t j = 0; j < p.tenors.size(); ++j) {
      if (x <= p.tenors[j]) return p.vol[j];
    }
    return p.vol.back();
  };
  double sum = 0.0;
  const int m = 4000;
  for (int i = 0; i < m; ++i) {
    const double s = sigma_at(tau + (i + 0.5) / m);
    sum += s * s / m;
  }
  return std::sqrt(sum);
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  const rt::HjmParams coarse{.r0 = 0.03, .tenors = {0.25, 0.5, 1.0, 2.0, 5.0}, .vol = {0.012, 0.011, 0.010, 0.009, 0.007}};
  rt::HjmParams fine{.r0 = 0.03};
  fine.tenors.clear();
  fine.vol.clear();
  for (int i = 1; i <= 20; ++i) {
    const double tau = 0.25 * i;
    fine.tenors.push_back(tau);
    fine.vol.push_back(tau <= 0.25 ? 0.012 : tau <= 0.5 ? 0.011 : tau <= 1.0 ? 0.010 : tau <= 2.0 ? 0.009 : 0.007);
  }
  const std::int64_t steps = 52;
  const double dt = 1.0 / steps;
  const auto coarse_sim = rt::hjm_paths(steps, 100000, dt, coarse, as_f64());
  const auto fine_sim = rt::hjm_paths(steps, 100000, dt, fine, as_f64());
  const auto coarse_curve = coarse_sim.forward.select(0, steps);  // [paths, tenors] after one year
  const auto fine_curve = fine_sim.forward.select(0, steps);
  const auto coarse_sd = coarse_curve.std(0, false).items<double>(), fine_sd = fine_curve.std(0, false).items<double>();
  out.writeln("tenor   exact sd   coarse grid   fine grid");
  const std::vector<std::pair<std::size_t, std::size_t>> pick = {{0, 0}, {2, 3}, {3, 7}};  // coarse index, fine index of the same tenor
  for (const auto &[c, f] : pick) {
    out.writeln(coarse.tenors[c], exact_sd(coarse, coarse.tenors[c]), coarse_sd[c], fine_sd[f]);
  }
  const auto first = coarse_curve.select(1, 0), last = coarse_curve.select(1, 4);
  const double corr = ((first - first.mean()) * (last - last.mean())).mean().item<double>() / (first.std().item<double>() * last.std().item<double>());
  out.writeln("correlation of the 0.25 and 5 year forwards:", corr);
}
```

Output

```text
tenor   exact sd   coarse grid   fine grid
0.250000 0.010025 0.010791 0.010558
1.000000 0.009000 0.009566 0.009077
2.000000 0.007000 0.008685 0.007519
correlation of the 0.25 and 5 year forwards: 0.999641
```

The exact spread of the one-year-ahead forward at the first tenor is 0.0100, below its own volatility of 0.012 because the shocks it has absorbed came from longer tenors, where the volatility is lower. On the coarse grid the simulated spreads are larger than the exact ones, by 8% at the first tenor and 24% at two years; on the grid of twenty tenors they are within 5% of the exact value everywhere and within 1% at one year, which shows that the gap is the interpolation of the curve along a coarse grid and shrinks with the tenor spacing. The correlation between the 0.25-year and 5-year forwards is 0.9996, essentially one: one factor means that the curve can only move in the one direction that the volatilities define. A model that needs the curve to twist independently of its level needs more factors than this one has.

## Program 3: zero-coupon prices from the model

`hjm_zcb_mc` simulates the short rate on a grid of `steps` steps up to the maturity and averages the discount factor, {math}`\frac1N\sum_i\exp(-\sum_t r^{(i)}_t\Delta t)`. The program prices four maturities and prints the yield of each. It then sets every volatility to zero, so that nothing is random and nothing drifts, and prices a two-year bond: the result must be {math}`e^{-r_0T}`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

namespace rt = pluss::subsp::math::finance::rates;
using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(6));
  rt::HjmParams params{.r0 = 0.03, .tenors = {0.25, 0.5, 1.0, 2.0, 5.0}, .vol = {0.012, 0.011, 0.010, 0.009, 0.007}};
  out.writeln("maturity  zero price   yield");
  for (const double T : {0.5, 1.0, 2.0, 5.0}) {
    const double price = rt::hjm_zcb_mc(T, params, 200000, 252, {}, as_f64());
    out.writeln(T, price, -std::log(price) / T);
  }
  params.vol = {0.0, 0.0, 0.0, 0.0, 0.0};
  out.writeln("with zero volatility, 2 years:", rt::hjm_zcb_mc(2.0, params, 1000, 100, {}, as_f64()), " exp(-r0 T) =", std::exp(-0.03 * 2.0));
}
```

Output

```text
maturity  zero price   yield
0.500000 0.985115 0.029994
1.000000 0.970455 0.029990
2.000000 0.941802 0.029980
5.000000 0.861020 0.029928
with zero volatility, 2 years: 0.941765  exp(-r0 T) = 0.941765
```

The zero prices are 0.985115, 0.970455, 0.941802 and 0.861020 for half a year, one, two and five years, with yields of 2.9994%, 2.9990%, 2.9980% and 2.9928%, within a basis point of the initial 3% up to two years and within 0.7 basis points at five: the curve starts flat, and because the drift is the no-arbitrage drift, bond prices implied by the simulation reproduce it, up to sampling and discretisation error. (With a drift that lacked the no-arbitrage term or the transport along the tenor axis, the same program gave yields of 3.01% to 3.06%.) With zero volatility the price of the two-year bond is 0.941765, exactly {math}`e^{-0.03\times2}`: with nothing random the discount factor is that of the flat curve, as it must be.

## Checklist

1. Give `tenors` as a strictly increasing grid of positive times and one non-negative `vol` for each.
2. Remember that all the tenors share one shock: use the model where one factor is enough.
3. Read the short rate from `rate` and the curve from `forward`; with `paths_first` the axes swap.
4. Check a new set of volatilities against its own moments (Program 2) before pricing with it.
5. Use enough steps in `hjm_zcb_mc` that the discount factor's quadrature error is small against the sampling error.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="zero_coupon_bonds.html">
    <span class="label">Previous</span>
    <span class="title">Zero-coupon bonds and yield curves</span>
  </a>
  <a class="next-link" href="rate_objects.html">
    <span class="label">Next</span>
    <span class="title">Rate model objects</span>
  </a>
</div>
<!-- apiref:nav:end -->
