<!-- generated from simulation.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Simulation

[Back to Series by example](../series.md)

When there is no formula, simulate. This page prices an option whose payoff depends on the whole price path, with fifty thousand paths generated at once, and reports the answer with its Monte Carlo error.

## The mental model

A simulation is an array of random numbers with one axis for the *scenario* and one for *time*, followed by a few array operations that turn shocks into prices and prices into payoffs. `cumsum` along the time axis turns a series of steps into a path; reductions along the scenario axis turn many payoffs into one estimate and its error. No loop over scenarios is ever written.

## An Asian option

A stock starts at 100 and follows geometric Brownian motion with 20 % volatility and 3 % interest. The option pays the average price over the year above 100. The program simulates weekly prices for 50 000 paths.

```cpp
#include <pluss/pluss.h>

#include <cmath>
#include <iostream>

int main() {
  using pluss::ScalarType;
  const double spot = 100.0, strike = 100.0, rate = 0.03, vol = 0.2, years = 1.0;
  const std::int64_t paths = 50000, steps = 52;
  const double dt = years / steps;

  // 1. All shocks in one array: one row per path, one column per week.
  auto z = pluss::series::randn({paths, steps}, ScalarType::Float64, 2024);

  // 2. Geometric Brownian motion: each step multiplies the price by exp((r - vol^2 / 2) dt + vol sqrt(dt) z).
  auto log_step = z.mul(vol * std::sqrt(dt)).add((rate - 0.5 * vol * vol) * dt);
  auto price = log_step.cumsum(1).exp().mul(spot);
  std::cout << "paths " << price.size(0) << ", steps " << price.size(1) << "\n";
  std::cout << "mean final price " << price.select(1, steps - 1).mean().item<double>() << " (forward " << spot * std::exp(rate * years) << ")\n";

  // 3. Payoff of an Asian call: the average price over the path against the strike.
  auto payoff = price.mean(1).sub(strike).clamp_min(0.0);
  auto discounted = payoff.mul(std::exp(-rate * years));

  // 4. The estimate and its Monte Carlo standard error.
  const double estimate = discounted.mean().item<double>();
  const double stderror = discounted.std().item<double>() / std::sqrt(static_cast<double>(paths));
  std::cout << "price " << estimate << " +/- " << stderror << " (one standard error)\n";
  std::cout << "paths ending in the money: " << discounted.gt(0.0).sum().item<double>() / paths << "\n";
  std::cout << "95th percentile of the payoff " << discounted.quantile(0.95).item<double>() << "\n";
}
```

Output

```text
paths 50000, steps 52
mean final price 102.99 (forward 103.045)
price 5.34536 +/- 0.0348491 (one standard error)
paths ending in the money: 0.52402
95th percentile of the payoff 21.7677
```
The `randn` call produces a `50000 x 52` table of shocks. Scaling and shifting turn them into log-returns, `cumsum(1)` accumulates them along each path, and `exp` returns to prices. The first sanity check is the mean final price, 102.99, against the forward price `100 e^0.03 = 103.05`: they agree to within sampling error, which tells you the dynamics are right before any payoff is computed. The payoff is the average of each path minus the strike, clamped at zero; the estimate is `5.35`, and its standard error `0.035` (the standard deviation of the discounted payoffs over the square root of the number of paths) says the price is `5.35` give or take `0.07` at two standard errors. About 52 % of paths end in the money, and the 95th percentile of the payoff is `21.8`, a reminder that an option is a bet on the tail.

## Integration by random sampling

Monte Carlo is also the way to integrate when a grid is hopeless. The integral of a function over a region is the region's size times the average of the function at random points, and the standard deviation of the function over the square root of the number of points is the error. The program integrates `4 / (1 + x^2)` over `[0, 1]`, whose value is pi, at three sample sizes; measures the volume of the unit ball in five dimensions by counting random points of a cube that fall inside it; and finishes with antithetic variates, a way to cut the error without drawing more points.

```cpp
#include <pluss/pluss.h>

#include <cmath>
#include <iostream>

int main() {
  using pluss::ScalarType;
  const double pi = 3.14159265358979323846;

  // 1. The integral of 4 / (1 + x^2) over [0, 1] is pi. Average the integrand at uniform points: the mean times the interval
  //    length estimates the integral, and the standard deviation of the integrand over sqrt(N) is the error.
  for (const std::int64_t n : {100, 10000, 1000000}) {
    auto x = pluss::series::rand({n}, ScalarType::Float64, 17);
    auto f = x.square().add(1.0).reciprocal().mul(4.0);
    std::cout << "N = " << n << ": " << f.mean().item<double>() << " +/- " << f.std().item<double>() / std::sqrt(static_cast<double>(n)) << "   (pi = " << pi << ")\n";
  }

  // 2. Volume of the unit ball in five dimensions: sample the cube [-1, 1]^5 and count the points inside the ball.
  //    The cube has volume 32; the exact ball volume is 8 pi^2 / 15.
  const std::int64_t n = 2000000;
  auto points = pluss::series::rand({n, 5}, ScalarType::Float64, 99).mul(2.0).sub(1.0);
  auto inside = points.square().sum(1).le(1.0).to(ScalarType::Float64);
  const double fraction = inside.mean().item<double>();
  const double volume = 32.0 * fraction;
  const double error = 32.0 * std::sqrt(fraction * (1.0 - fraction) / static_cast<double>(n));
  std::cout << "5-ball volume " << volume << " +/- " << error << "   (exact " << 8.0 * pi * pi / 15.0 << ")\n";

  // 3. Antithetic variates: pair every point x with 1 - x. For a monotone integrand the two values are negatively correlated,
  //    so their average varies less than a single value. Integrate exp(x) over [0, 1]; the exact answer is e - 1.
  const std::int64_t m = 20000;
  auto u = pluss::series::rand({m}, ScalarType::Float64, 5);
  auto plain = u.exp();
  auto paired = u.exp().add(u.neg().add(1.0).exp()).mul(0.5);
  std::cout << "plain      " << plain.mean().item<double>() << " +/- " << plain.std().item<double>() / std::sqrt(static_cast<double>(m)) << "\n";
  std::cout << "antithetic " << paired.mean().item<double>() << " +/- " << paired.std().item<double>() / std::sqrt(static_cast<double>(m)) << "   (exact " << std::exp(1.0) - 1.0 << ")\n";
}
```

Output

```text
N = 100: 3.19238 +/- 0.0682382   (pi = 3.14159)
N = 10000: 3.15534 +/- 0.00644179   (pi = 3.14159)
N = 1000000: 3.14116 +/- 0.000643055   (pi = 3.14159)
5-ball volume 5.26701 +/- 0.00839056   (exact 5.26379)
plain      1.72307 +/- 0.00348902
antithetic 1.71851 +/- 0.000442965   (exact 1.71828)
```
Each hundredfold increase in `N` shrinks the error tenfold, from `0.068` to `0.0064` to `0.00064`, and the estimates stay within about one error of pi. The five-dimensional volume, `5.267 +/- 0.008`, agrees with the exact `8 pi^2 / 15 = 5.264` to within half an error; a grid with only ten points per axis would already need a hundred thousand evaluations, and the cost of sampling does not grow with the dimension. The antithetic pairing, where each `x` is used together with `1 - x`, estimates the integral of `exp(x)` with an error of `0.00044` against `0.0035` for plain sampling: eight times smaller, the same as drawing about sixty times as many points. It works because `exp(x)` and `exp(1 - x)` move in opposite directions, so their average varies far less than either alone.

## Decisions

- **Axes:** scenarios on one axis, time on the next; reductions along time build the payoff and along scenarios the estimate.
- **Check the dynamics first:** compare a quantity with a known answer (the forward, a variance) before trusting a payoff.
- **Report the error:** the standard error shrinks with the square root of the path count, so ten times the accuracy costs a hundred times the paths.
- **Seeds:** fix the seed for a result you must reproduce, change it to see how much the answer moves.
- **Variance reduction:** before buying more paths, ask whether a pairing or a control variate with a known mean removes the variation you do not care about.

## Where next

Reference pages: [`randn`](../../sphinx/api/series_factory/randn.md), [`rand`](../../sphinx/api/series_factory/rand.md), [`cumsum`](../../sphinx/api/series/cumsum.md), [`exp`](../../sphinx/api/series/exp.md), [`mean`](../../sphinx/api/series/mean.md), [`std`](../../sphinx/api/series/std.md), [`clamp_min`](../../sphinx/api/series/clamp_min.md), [`quantile`](../../sphinx/api/series/quantile.md), [`reciprocal`](../../sphinx/api/series/reciprocal.md). Next: [Signals](signals.md).
