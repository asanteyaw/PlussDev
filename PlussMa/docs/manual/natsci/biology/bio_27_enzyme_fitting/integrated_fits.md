# Global fits of progress curves

[Back to Biology 27: Fitting Enzyme Progress Curves](../bio_27_enzyme_fitting.md)

You have measured how the substrate of an enzyme disappears with time, at several starting concentrations, and you want the maximum rate and the Michaelis constant, with a statement of how well the data pins them down. This page fits all the curves at once, with the closed-form law of [Biology 5](../bio_05_enzymes.md), starting from an automatic guess or from your own, and reports estimates, standard errors and 95% confidence intervals.

## How it works

Each curve $k$ starts at a known concentration $S_{0,k}$ and is measured at the same times $t_1,\dots,t_M$. When the enzyme is much scarcer than the substrate the substrate follows the integrated Michaelis-Menten law

```{math}
S(t) = K_m\,W\!\left(\frac{S_0}{K_m}\exp\frac{S_0 - V_{max}t}{K_m}\right)
```

where $W$ is the Lambert W function. A global fit chooses one $V_{max}$ and one $K_m$ for all $K\times M$ measurements by minimising the sum of squared residuals

```{math}
\mathrm{SSE}(V_{max}, K_m) = \sum_{k=1}^{K}\sum_{i=1}^{M}\bigl(y_{ki} - S(t_i; S_{0,k}, V_{max}, K_m)\bigr)^2
```

with Levenberg-Marquardt iterations. The data go in a `ProgressData` with three fields: `times` of shape `[M]`, `initial` of shape `[K]` and `substrate` of shape `[K, M]`. `fitting::fit_integrated(data, guess)` takes a guess of shape `[fits, 2]` holding $(V_{max}, K_m)$, one row for every fit you want, and returns an `EnzymeFit` with `fit` (the `beta` estimates, `sse`, `residuals`, `converged`, `iterations`) and `intervals` (`standard_error`, `lower`, `upper`). The intervals are the usual ones for nonlinear least squares, $\hat\beta_j \pm t_{n-p}\,\hat\sigma\sqrt{[(J^\top J)^{-1}]_{jj}}$ with $J$ the Jacobian at the optimum, $n = KM$ the number of measurements and $p = 2$. `fitting::initial_guess(data)` gives a starting point from the data alone: $K_m$ at the median starting concentration and $V_{max}$ from the steepest early decline. The programs generate their own data from $V_{max} = 0.9$ and $K_m = 1.4$ with seeded noise of standard deviation $0.01$, so you can see how close the fit comes to the truth.

## Fit four curves

The program generates four progress curves, starting at `0.4`, `1`, `2.5` and `6`, each measured at ten times, asks for the automatic starting guess, fits, and prints the estimates with their errors and intervals.

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

// four progress curves measured at ten times, generated from Vmax = 0.9 and Km = 1.4 with a little noise (seeded, so the program is repeatable)
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
  out.writeln("curves and times            ", std::vector<double>{static_cast<double>(data.substrate.size(0)), static_cast<double>(data.substrate.size(1))});
  out.writeln("starting guess (Vmax, Km) ", flat(fitting::initial_guess(data)));

  const auto result = fitting::fit_integrated(data, fitting::initial_guess(data));
  out.writeln("fitted (Vmax, Km)         ", flat(result.fit.beta));
  out.writeln("standard errors           ", flat(result.intervals.standard_error));
  out.writeln("95% lower                 ", flat(result.intervals.lower));
  out.writeln("95% upper                 ", flat(result.intervals.upper));
  out.writeln("converged, iterations     ", flat(result.fit.converged), flat(result.fit.iterations));
  out.writeln("sum of squares            ", flat(result.fit.sse));
}
```

Output

```text
curves and times            [4.0000, 10.0000]
starting guess (Vmax, Km) [1.0813, 2.5000]
fitted (Vmax, Km)         [0.8987, 1.3989]
standard errors           [0.0038, 0.0180]
95% lower                 [0.8910, 1.3625]
95% upper                 [0.9063, 1.4353]
converged, iterations     [1.0000][4.0000]
sum of squares            [0.0034]
```

The automatic guess `(1.0813, 2.5000)` is rough but good enough: Levenberg-Marquardt converges (`converged = 1`) in `4` iterations to `V_max = 0.8987` and `K_m = 1.3989`, within one standard error or two of the truth `(0.9, 1.4)`. The standard errors `0.0038` and `0.0180` give 95% intervals `[0.8910, 0.9063]` and `[1.3625, 1.4353]`, and both contain the generating values. The sum of squares `0.0034` over `40` measurements corresponds to a residual standard deviation of about `0.0095`, close to the noise that was added.

## Many starting points in one call

A guess with several rows is several fits, run together. The program starts from five very different guesses, reads the fitted constants, the number of iterations, and the residuals of the first fit curve by curve.

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

  // five starting guesses, from far too small to far too large, in one call: one row of the guess per fit
  const auto starts = pluss::series::make({{0.2, 0.3}, {1.5, 3.0}, {0.5, 0.7}, {3.0, 0.2}, {0.1, 8.0}}).to(pluss::ScalarType::Float64);
  const auto many = fitting::fit_integrated(data, starts);
  out.writeln("fitted Vmax from five starts ", flat(many.fit.beta.select(1, 0)));
  out.writeln("fitted Km from five starts   ", flat(many.fit.beta.select(1, 1)));
  out.writeln("converged                    ", flat(many.fit.converged));
  out.writeln("iterations                   ", flat(many.fit.iterations));

  // the residuals of one fit, curve by curve: root mean square of what the model leaves unexplained
  const auto resid = many.fit.residuals.select(0, 0).reshape({4, 10});
  out.writeln("rms residual per curve       ", flat(resid.square().mean(1).sqrt()));
  out.writeln("largest residual (any curve) ", resid.abs().max().item<double>());
}
```

Output

```text
fitted Vmax from five starts [0.8987, 0.8987, 0.8987, 0.8987, 0.8987]
fitted Km from five starts   [1.3989, 1.3989, 1.3989, 1.3989, 1.3989]
converged                    [1.0000, 1.0000, 1.0000, 1.0000, 1.0000]
iterations                   [6.0000, 5.0000, 5.0000, 7.0000, 9.0000]
rms residual per curve       [0.0096, 0.0079, 0.0093, 0.0102]
largest residual (any curve) 0.0205
```

All five starts reach the same optimum, `V_max = 0.8987` and `K_m = 1.3989`, in `5` to `9` iterations. The surface that the fit minimises has one basin here, so the answer does not depend on the guess; if five starts had disagreed you would have found several local optima instead. The residual of every curve has a root mean square of about `0.009` to `0.010`, which is the size of the noise, and no curve is fitted worse than another; a curve whose residual is much larger than the rest would point at a measurement problem or a wrong model.

## Predicting with the uncertainty

The program uses the fit to predict a curve that was never measured, starting at `10` (higher than any measured curve), and repeats the prediction at the two extreme corners of the confidence box to see how far the prediction can move.

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
  const auto result = fitting::fit_integrated(data, fitting::initial_guess(data));
  const auto beta = flat(result.fit.beta), lo = flat(result.intervals.lower), hi = flat(result.intervals.upper);

  // use the fit to predict a curve that was never measured: starting concentration 10, at times 2, 5, 10
  const auto t = vec({2.0, 5.0, 10.0});
  const auto s0 = vec({10.0}).select(0, 0);
  const auto predict = [&](double vmax, double km) { return flat(enzymes::integrated_michaelis_menten(t, s0, vec({vmax}), vec({km}))); };
  out.writeln("times                          ", flat(t));
  out.writeln("prediction at the best fit     ", predict(beta[0], beta[1]));
  out.writeln("prediction, Vmax low, Km high  ", predict(lo[0], hi[1]));
  out.writeln("prediction, Vmax high, Km low  ", predict(hi[0], lo[1]));
  out.writeln("generating law (0.9, 1.4)      ", predict(0.9, 1.4));
}
```

Output

```text
times                          [2.0000, 5.0000, 10.0000]
prediction at the best fit     [8.4399, 6.1800, 2.7961]
prediction, Vmax low, Km high  [8.4583, 6.2252, 2.8777]
prediction, Vmax high, Km low  [8.4215, 6.1343, 2.7139]
generating law (0.9, 1.4)      [8.4378, 6.1749, 2.7881]
```

At the best fit the substrate falls from `10` to `8.4399`, `6.1800` and `2.7961` at times `2`, `5` and `10`, within `0.01` of what the generating law gives (`8.4378`, `6.1749`, `2.7881`). The two corners, a slow enzyme (low $V_{max}$, high $K_m$) and a fast one, bracket the prediction between `2.7139` and `2.8777` at time `10`. Even the extreme corners differ from the best fit by less than `0.09`, so the uncertainty of the constants moves the prediction of an unmeasured curve by only about `0.08` at the end of the experiment. The corners are deliberately extreme combinations of the two constants, so the real spread is smaller still.

## Decisions

- Use the closed form of `fit_integrated` when the enzyme is much scarcer than the substrate, $E_0 \ll S_0 + K_m$. When product inhibition is present, use `fit_product_inhibition` on the [next page](product_inhibition.md).
- Measure several starting concentrations, including some well above $K_m$: a single curve constrains the two constants much less, as the pitfall below shows.
- Take the starting guess from `initial_guess` unless you have a better one; pass a table of guesses to compare starts in one call.
- Read `converged`, the residuals curve by curve, and the intervals before you trust the numbers.

## Pitfalls

A single curve is cheaper to measure, but it carries much less information about the two constants. The program fits the same data twice, once with all four curves and once with only the curve that starts at `6`.

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
  const auto all = fitting::fit_integrated(data, fitting::initial_guess(data));
  // keep only the curve that starts at 6.0
  fitting::ProgressData single;
  single.times = data.times;
  single.initial = data.initial.slice(0, 3, 4, 1);
  single.substrate = data.substrate.slice(0, 3, 4, 1);
  const auto one = fitting::fit_integrated(single, fitting::initial_guess(single));
  out.writeln("all four curves: (Vmax, Km)  ", flat(all.fit.beta));
  out.writeln("                 standard errors ", flat(all.intervals.standard_error));
  out.writeln("one curve:       (Vmax, Km)  ", flat(one.fit.beta));
  out.writeln("                 standard errors ", flat(one.intervals.standard_error));
  out.writeln("ratio of the Km standard errors ", std::vector<double>{flat(one.intervals.standard_error)[1] / flat(all.intervals.standard_error)[1]});
}
```

Output

```text
all four curves: (Vmax, Km)  [0.8987, 1.3989]
                 standard errors [0.0038, 0.0180]
one curve:       (Vmax, Km)  [0.9056, 1.4388]
                 standard errors [0.0077, 0.0420]
ratio of the Km standard errors [2.3381]
```

The estimates from the single curve are close (`0.9056` and `1.4388`), but their standard errors are about twice as large: the $K_m$ error grows from `0.0180` to `0.0420`, a ratio of `2.34`. A curve at one concentration sees only part of the rate law, so the two constants trade off against each other along it.

Malformed inputs are rejected before any fitting starts. The program tries two observations for two parameters, a guess of the wrong width and a table whose number of curves does not match the list of starting concentrations.

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

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  fitting::ProgressData data;
  data.times = vec({0.5, 1.0, 2.0});
  data.initial = vec({1.0, 3.0});
  data.substrate = pluss::series::make({{0.8, 0.7, 0.55}, {2.6, 2.3, 1.9}}).to(pluss::ScalarType::Float64);
  // one observation per parameter is not enough for an interval: two curves of one time each, two parameters
  fitting::ProgressData thin;
  thin.times = vec({1.0});
  thin.initial = data.initial;
  thin.substrate = vec({0.7, 2.3}).reshape({2, 1});
  try {
    fitting::fit_integrated(thin, vec({1.0, 1.0}).reshape({1, 2}));
  } catch (const pluss::Error &e) {
    out.writeln("two observations, two parameters: ", e.message());
  }
  // a guess with one value too few for the product-inhibition model
  try {
    fitting::fit_product_inhibition(data, vec({1.0, 1.0}).reshape({1, 2}));
  } catch (const pluss::Error &e) {
    out.writeln("guess of the wrong width: ", e.message());
  }
  // a measured substrate table whose rows do not match the list of starting concentrations
  fitting::ProgressData bad = data;
  bad.initial = vec({1.0, 3.0, 5.0});
  try {
    fitting::fit_integrated(bad, vec({1.0, 1.0}).reshape({1, 2}));
  } catch (const pluss::Error &e) {
    out.writeln("three starting levels, two curves: ", e.message());
  }
}
```

Output

```text
two observations, two parameters: natsci: estimation: need more observations than parameters
guess of the wrong width: natsci: fit_product_inhibition: the starting guess must be [fits, 3]
three starting levels, two curves: natsci: fit_integrated: substrate must be [curves, times] with one row per starting concentration and one column per time
```

Each message names the function and the rule that was broken. Two measurements cannot give an interval for two parameters, because no degrees of freedom remain; the guess for `fit_product_inhibition` must have three columns; and `substrate` must have one row for every entry of `initial` and one column for every time.

## Where next

Continue with [Product inhibition](product_inhibition.md) to fit a three-constant model, or go back to [Biology 5: Enzymes](../bio_05_enzymes.md) for the rate laws behind the closed form and to [Biology 20: Metabolic Control](../bio_20_control.md) for how such constants decide a pathway's behaviour.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="product_inhibition.html">
    <span class="label">Next</span>
    <span class="title">Product inhibition</span>
  </a>
</div>
<!-- apiref:nav:end -->
