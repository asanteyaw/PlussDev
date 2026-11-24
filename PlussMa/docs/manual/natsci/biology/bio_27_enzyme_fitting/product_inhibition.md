# Product inhibition

[Back to Biology 27: Fitting Enzyme Progress Curves](../bio_27_enzyme_fitting.md)

An enzyme that is slowed by its own product has progress curves that bend more than the plain law allows: the reaction that starts quickly dwindles faster than the substrate alone would explain. You have curves like that and you want three constants, the maximum rate, the Michaelis constant and the product-inhibition constant, with intervals, and you want to know whether the third constant is worth having. This page fits the competitive product-inhibition model, tests it against the plain law, and uses it to predict a held-out experiment.

## How it works

With a competitive inhibitor that is the product itself, the amount of product at time $t$ is $S_0 - S$, and the rate law is

```{math}
\frac{\mathrm{d}S}{\mathrm{d}t} = -\,\frac{V_{max}\,S}{K_m\left(1 + \dfrac{S_0 - S}{K_p}\right) + S}
```

where $K_p$ is the product-inhibition constant: a small $K_p$ means strong inhibition, and $K_p\to\infty$ returns the plain Michaelis-Menten law. This law has no closed form, so `fitting::fit_product_inhibition(data, guess, steps_per_interval)` integrates it with fixed-step RK4 between consecutive measurement times, `steps_per_interval` steps each (default `40`), for all curves and all trial parameters at once. The data, the intervals and the result are the same as in [Global fits of progress curves](integrated_fits.md), but the guess has three columns, $(V_{max}, K_m, K_p)$, and `initial_guess(data, true)` supplies all three. Two nested models can be compared by their sums of squares: with $\mathrm{SSE}_2$ for the plain law (two constants) and $\mathrm{SSE}_3$ for the extended one, the statistic

```{math}
F = \frac{\mathrm{SSE}_2 - \mathrm{SSE}_3}{\mathrm{SSE}_3/(n-3)}
```

is large when the extra constant earns its place. The data below are generated with $V_{max} = 1.1$, $K_m = 1.8$ and $K_p = 0.6$ and noise of standard deviation $0.01$.

## Fit the three constants

The program generates four inhibited curves, takes the automatic three-constant guess, fits, and prints the estimates, the standard errors and the intervals.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace fitting = biology::systems::enzyme_fitting;
namespace enzymes = biology::systems::enzymes;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// Data with competitive product inhibition, generated with Vmax = 1.1, Km = 1.8, Kp = 0.6: dS/dt = -Vmax S / (Km (1 + (S0 - S)/Kp) + S)
static pluss::Series inhibited(const pluss::Series &times, const pluss::Series &s0, double noise) {
  const double vmax = 1.1, km = 1.8, kp = 0.6;
  const auto t = flat(times);
  std::vector<pluss::Series> columns;
  pluss::Series s = s0;
  double now = 0.0;
  for (double next : t) {
    const auto rate = [&](const pluss::Series &x) { return x.mul(-vmax).div(x.sub(s0).neg().div(kp).add(1.0).mul(km).add(x)); };
    s = dynamics::simulate(rate, s, now, next, 200);
    now = next;
    columns.push_back(s.unsqueeze(1));
  }
  const pluss::Series clean = columns[0].unite(std::vector<pluss::Series>(columns.begin() + 1, columns.end()), 1);
  return clean.add(pluss::series::randn({clean.size(0), clean.size(1)}, pluss::ScalarType::Float64, 11).mul(noise));
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  fitting::ProgressData data;
  data.times = vec({0.5, 1.0, 1.5, 2.0, 3.0, 4.0, 5.0, 6.5, 8.0, 10.0});
  data.initial = vec({0.4, 1.0, 2.5, 6.0});
  data.substrate = inhibited(data.times, data.initial, 0.01);

  const auto guess = fitting::initial_guess(data, true);
  out.writeln("starting guess (Vmax, Km, Kp) ", flat(guess));
  const auto result = fitting::fit_product_inhibition(data, guess);
  out.writeln("fitted (Vmax, Km, Kp)         ", flat(result.fit.beta));
  out.writeln("standard errors               ", flat(result.intervals.standard_error));
  out.writeln("95% interval, lower           ", flat(result.intervals.lower));
  out.writeln("95% interval, upper           ", flat(result.intervals.upper));
  out.writeln("converged                     ", flat(result.fit.converged));
  out.writeln("sum of squares                ", flat(result.fit.sse));
}
```

Output

```text
starting guess (Vmax, Km, Kp) [1.1602, 2.5000, 2.5000]
fitted (Vmax, Km, Kp)         [1.1003, 1.7809, 0.5865]
standard errors               [0.0119, 0.0450, 0.0151]
95% interval, lower           [1.0763, 1.6897, 0.5559]
95% interval, upper           [1.1243, 1.8721, 0.6171]
converged                     [1.0000]
sum of squares                [0.0033]
```

The fit converges to $(V_{max}, K_m, K_p) = (1.1003, 1.7809, 0.5865)$, within `0.03` of the truth `(1.1, 1.8, 0.6)`. The intervals `[1.0763, 1.1243]`, `[1.6897, 1.8721]` and `[0.5559, 0.6171]` all contain the generating values, so the model has recovered its own constants. The sum of squares `0.0033` is again the size of the noise.

## Is the third constant needed?

The program fits the same inhibited data with the plain two-constant law and with the three-constant model, and compares the sums of squares and the F statistic.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace fitting = biology::systems::enzyme_fitting;
namespace enzymes = biology::systems::enzymes;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static pluss::Series inhibited(const pluss::Series &times, const pluss::Series &s0, double noise) {
  const double vmax = 1.1, km = 1.8, kp = 0.6;
  const auto t = flat(times);
  std::vector<pluss::Series> columns;
  pluss::Series s = s0;
  double now = 0.0;
  for (double next : t) {
    const auto rate = [&](const pluss::Series &x) { return x.mul(-vmax).div(x.sub(s0).neg().div(kp).add(1.0).mul(km).add(x)); };
    s = dynamics::simulate(rate, s, now, next, 200);
    now = next;
    columns.push_back(s.unsqueeze(1));
  }
  const pluss::Series clean = columns[0].unite(std::vector<pluss::Series>(columns.begin() + 1, columns.end()), 1);
  return clean.add(pluss::series::randn({clean.size(0), clean.size(1)}, pluss::ScalarType::Float64, 11).mul(noise));
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  fitting::ProgressData data;
  data.times = vec({0.5, 1.0, 1.5, 2.0, 3.0, 4.0, 5.0, 6.5, 8.0, 10.0});
  data.initial = vec({0.4, 1.0, 2.5, 6.0});
  data.substrate = inhibited(data.times, data.initial, 0.01);

  // the same inhibited data fitted two ways: without product inhibition (two parameters) and with it (three)
  const auto plain = fitting::fit_integrated(data, fitting::initial_guess(data));
  const auto full = fitting::fit_product_inhibition(data, fitting::initial_guess(data, true));
  out.writeln("two-parameter fit (Vmax, Km)    ", flat(plain.fit.beta));
  out.writeln("three-parameter fit (Vmax, Km, Kp) ", flat(full.fit.beta));
  const double s2 = flat(plain.fit.sse)[0], s3 = flat(full.fit.sse)[0];
  out.writeln("sum of squares, two vs three  ", std::vector<double>{s2, s3});
  out.writeln("ratio                         ", std::vector<double>{s2 / s3});
  // the F statistic for one extra parameter: how much better per degree of freedom the larger model fits
  const double n = static_cast<double>(plain.fit.observations);
  out.writeln("F statistic (1, n - 3)        ", std::vector<double>{(s2 - s3) / (s3 / (n - 3.0))});
  out.writeln("rms residual, two vs three    ", std::vector<double>{std::sqrt(s2 / n), std::sqrt(s3 / n)});
}
```

Output

```text
two-parameter fit (Vmax, Km)    [0.6882, 2.3538]
three-parameter fit (Vmax, Km, Kp) [1.1003, 1.7809, 0.5865]
sum of squares, two vs three  [1.1262, 0.0033]
ratio                         [346.5197]
F statistic (1, n - 3)        [12784.2272]
rms residual, two vs three    [0.1678, 0.0090]
```

The plain law fits badly, with `V_max = 0.6882` and `K_m = 2.3538` and a root mean square residual of `0.1678`, about seventeen times the noise; the three-constant model brings it to `0.0090`. The sums of squares differ by a factor of `346.52` and the F statistic is `12784`, far beyond any critical value for one extra parameter with `37` degrees of freedom. The extra constant is clearly needed; and notice that ignoring the inhibition did not only add error, it also changed the estimates of the other two constants.

## Predicting a new experiment

The strongest test of a model is an experiment it has not seen. The program fits the four curves, then generates a fifth curve at starting concentration `4` with fresh noise and compares it with the prediction made from the fit alone.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace fitting = biology::systems::enzyme_fitting;
namespace enzymes = biology::systems::enzymes;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static pluss::Series inhibited(const pluss::Series &times, const pluss::Series &s0, double noise, std::uint64_t seed) {
  const double vmax = 1.1, km = 1.8, kp = 0.6;
  const auto t = flat(times);
  std::vector<pluss::Series> columns;
  pluss::Series s = s0;
  double now = 0.0;
  for (double next : t) {
    const auto rate = [&](const pluss::Series &x) { return x.mul(-vmax).div(x.sub(s0).neg().div(kp).add(1.0).mul(km).add(x)); };
    s = dynamics::simulate(rate, s, now, next, 200);
    now = next;
    columns.push_back(s.unsqueeze(1));
  }
  const pluss::Series clean = columns[0].unite(std::vector<pluss::Series>(columns.begin() + 1, columns.end()), 1);
  return clean.add(pluss::series::randn({clean.size(0), clean.size(1)}, pluss::ScalarType::Float64, seed).mul(noise));
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  fitting::ProgressData data;
  data.times = vec({0.5, 1.0, 1.5, 2.0, 3.0, 4.0, 5.0, 6.5, 8.0, 10.0});
  data.initial = vec({0.4, 1.0, 2.5, 6.0});
  data.substrate = inhibited(data.times, data.initial, 0.01, 11);
  const auto result = fitting::fit_product_inhibition(data, fitting::initial_guess(data, true));
  const auto beta = flat(result.fit.beta);

  // a held-out experiment: starting concentration 4, new noise. Predict it from the fit and compare with the measurements
  const auto s0 = vec({4.0});
  const auto held = inhibited(data.times, s0, 0.01, 99);
  const auto rate = [&](const pluss::Series &x) { return x.mul(-beta[0]).div(x.sub(4.0).neg().div(beta[2]).add(1.0).mul(beta[1]).add(x)); };
  std::vector<double> predicted;
  double now = 0.0;
  pluss::Series s = s0;
  for (double next : flat(data.times)) {
    s = dynamics::simulate(rate, s, now, next, 200);
    now = next;
    predicted.push_back(flat(s)[0]);
  }
  out.writeln("held-out measurements  ", flat(held));
  out.writeln("predicted by the fit   ", predicted);
  const auto measured = flat(held);
  double sum = 0.0;
  for (std::size_t i = 0; i < measured.size(); ++i) sum += (measured[i] - predicted[i]) * (measured[i] - predicted[i]);
  out.writeln("rms prediction error   ", std::vector<double>{std::sqrt(sum / static_cast<double>(measured.size()))});
  out.writeln("size of the noise      ", std::vector<double>{0.01});
}
```

Output

```text
held-out measurements  [3.6737, 3.3757, 3.1326, 2.9174, 2.5556, 2.2785, 2.0122, 1.7007, 1.4530, 1.1979]
predicted by the fit   [3.6569, 3.3717, 3.1273, 2.9138, 2.5547, 2.2616, 2.0161, 1.7132, 1.4682, 1.2066]
rms prediction error   [0.0105]
size of the noise      [0.0100]
```

The ten measured values follow the prediction to within the noise: the root mean square difference is `0.0105` against a noise standard deviation of `0.01`. A model that predicts a new curve to the size of the measurement noise has captured the system, and no part of the new curve was used in the fit.

## Decisions

- Start with the two-constant fit and add $K_p$ only when the F statistic, or a curve-by-curve look at the residuals, says the plain law is not enough.
- Leave `steps_per_interval` at its default unless your measurement times are very far apart; raise it and refit to check that the estimates do not move.
- Report the intervals as well as the estimates: $K_p$ is usually the least well determined of the three constants.
- Fit from several guesses and keep the best sum of squares among the converged fits: this model has valleys in which the optimiser can wander, and a runaway fit is reported as not converged.

## Pitfalls

If the data have no product inhibition, $K_p$ has nothing to measure. The program fits the three-constant model to the plain curves of the previous page.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace fitting = biology::systems::enzyme_fitting;
namespace enzymes = biology::systems::enzymes;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// data with NO product inhibition (the plain integrated law, Vmax = 0.9, Km = 1.4), fitted with the three-parameter model
static pluss::Series curves(const pluss::Series &t, const pluss::Series &s0, double vmax, double km, double noise) {
  std::vector<pluss::Series> rows;
  for (std::int64_t i = 0; i < s0.numel(); ++i)
    rows.push_back(enzymes::integrated_michaelis_menten(t, s0.select(0, i), vec({vmax}), vec({km})).unsqueeze(0));
  const pluss::Series clean = rows[0].unite(std::vector<pluss::Series>(rows.begin() + 1, rows.end()), 0);
  return clean.add(pluss::series::randn({clean.size(0), clean.size(1)}, pluss::ScalarType::Float64, 7).mul(noise));
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  fitting::ProgressData data;
  data.times = vec({0.5, 1.0, 1.5, 2.0, 3.0, 4.0, 5.0, 6.5, 8.0, 10.0});
  data.initial = vec({0.4, 1.0, 2.5, 6.0});
  data.substrate = curves(data.times, data.initial, 0.9, 1.4, 0.01);
  const auto result = fitting::fit_product_inhibition(data, fitting::initial_guess(data, true));
  out.writeln("fitted (Vmax, Km, Kp)    ", flat(result.fit.beta));
  out.writeln("standard errors          ", flat(result.intervals.standard_error));
  out.writeln("95% interval, lower      ", flat(result.intervals.lower));
  out.writeln("95% interval, upper      ", flat(result.intervals.upper));
  out.writeln("converged                ", flat(result.fit.converged));
}
```

Output

```text
fitted (Vmax, Km, Kp)    [0.8995, 1.3915, 252.6111]
standard errors          [0.0039, 0.0191, 230.8012]
95% interval, lower      [0.8917, 1.3527, -215.0364]
95% interval, upper      [0.9073, 1.4302, 720.2587]
converged                [1.0000]
```

The fit reports `converged = 1` and good estimates for $V_{max}$ and $K_m$ (`0.8995`, `1.3915`), but $K_p = 252.6$ with a standard error of `230.8`, and a 95% interval from `-215` to `720`. The interval includes zero and negative values: the data say only that $K_p$ is large, not what it is. Read a huge standard error on one constant as 'this constant is not identified', and drop the term.

A poor starting guess can send the optimiser away from the valley. The program fits the same inhibited data from three starting guesses, one reasonable, one very far off and one with a zero constant.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace biology = pluss::ma::natsci::biology;
namespace fitting = biology::systems::enzyme_fitting;
namespace enzymes = biology::systems::enzymes;
namespace dynamics = biology::dynamics;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

static pluss::Series inhibited(const pluss::Series &times, const pluss::Series &s0, double noise, std::uint64_t seed) {
  const double vmax = 1.1, km = 1.8, kp = 0.6;
  const auto t = flat(times);
  std::vector<pluss::Series> columns;
  pluss::Series s = s0;
  double now = 0.0;
  for (double next : t) {
    const auto rate = [&](const pluss::Series &x) { return x.mul(-vmax).div(x.sub(s0).neg().div(kp).add(1.0).mul(km).add(x)); };
    s = dynamics::simulate(rate, s, now, next, 200);
    now = next;
    columns.push_back(s.unsqueeze(1));
  }
  const pluss::Series clean = columns[0].unite(std::vector<pluss::Series>(columns.begin() + 1, columns.end()), 1);
  return clean.add(pluss::series::randn({clean.size(0), clean.size(1)}, pluss::ScalarType::Float64, seed).mul(noise));
}

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  fitting::ProgressData data;
  data.times = vec({0.5, 1.0, 1.5, 2.0, 3.0, 4.0, 5.0, 6.5, 8.0, 10.0});
  data.initial = vec({0.4, 1.0, 2.5, 6.0});
  data.substrate = inhibited(data.times, data.initial, 0.01, 11);
  // the same data fitted from three different starting guesses, one call each
  const std::vector<std::vector<double>> guesses = {{1.0, 2.0, 1.0}, {5.0, 0.2, 20.0}, {1.0, 2.0, 0.0}};
  for (const auto &g : guesses) {
    try {
      const auto r = fitting::fit_product_inhibition(data, vec(g).reshape({1, 3}));
      out.writeln("guess ", g, " -> ", flat(r.fit.beta.slice(1, 0, 2, 1)), " (Vmax, Km) converged ", flat(r.fit.converged), " sum of squares ", flat(r.fit.sse), " standard errors ", flat(r.intervals.standard_error));
    } catch (const pluss::Error &e) {
      out.writeln("guess ", g, " -> ", e.message());
    }
  }
}
```

Output

```text
guess [1.0000, 2.0000, 1.0000] -> [1.1003, 1.7809] (Vmax, Km) converged [1.0000] sum of squares [0.0033] standard errors [0.0119, 0.0450, 0.0151]
guess [5.0000, 0.2000, 20.0000] -> [86.4093, 745.5110] (Vmax, Km) converged [0.0000] sum of squares [2.6195] standard errors [nan, nan, nan]
guess [1.0000, 2.0000, 0.0000] -> natsci: fit_product_inhibition: every starting value must be non-zero and finite (a zero constant makes the rate law undefined)
```

The reasonable guess finds the right fit, with sum of squares `0.0033`. The far-off guess ends with a sum of squares of `2.6195`, about eight hundred times larger, and a Michaelis constant of `745`: the optimiser has run off along a flat direction where the rate law is almost a straight line and the inhibition constant grows without bound. The library reports this honestly: `converged` is `0.0000` and the standard errors are `nan`, because a constant that grows without limit has no interval. A zero constant makes the rate law undefined and is rejected at once with a message that says so. Treat `converged = 0` as a failed fit and start again from a better guess; do not use its estimates.

## Where next

Return to [Global fits of progress curves](integrated_fits.md) for the plain law, to [Biology 5: Enzymes](../bio_05_enzymes.md) for the rate laws, and to [Biology 25: Reaction Networks](../bio_25_networks.md) for the mass-action model that all of these progress-curve laws approximate.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="integrated_fits.html">
    <span class="label">Previous</span>
    <span class="title">Global fits of progress curves</span>
  </a>
</div>
<!-- apiref:nav:end -->
