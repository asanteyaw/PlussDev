# Profile likelihood

[Back to Methods 2: Fitting and Identifiability](../meth_02_fitting.md)

The intervals from a fit assume the model is nearly linear in its parameters around the best fit. When it is not, or when you suspect two parameters trade off, you want the interval that follows the model's own curvature. This page computes the profile of a parameter, the best fit that is possible when that parameter is held at each value of a grid, and reads confidence intervals from it. It compares them with the intervals of the previous page and shows a case in which the data determine a parameter only weakly.

## How it works

With Gaussian errors of variance {math}`\sigma^2`, the statistic {math}`\chi^2(\theta) = S(\theta)/\sigma^2` is, up to a constant, minus twice the log-likelihood. For one parameter {math}`\theta_i` held fixed and all others re-fitted, the profile is

```{math}
\chi^2_{\mathrm{PL}}(\theta_i) = \min_{\theta_{j\ne i}} S(\theta)/\sigma^2
```

and the confidence set at level {math}`1-\alpha` is {math}`\{\theta_i : \chi^2_{\mathrm{PL}}(\theta_i) - \chi^2_{\min} \le \chi^2_{1-\alpha}(d)\}`, with {math}`d = 1` for an interval on one parameter and {math}`d = p` for a region that holds for all parameters at once. A profile that stays below the threshold over a long stretch means the data do not determine the parameter there.

`estimation::profile_likelihood(model, y, start, index, grid)` re-fits all other parameters for every value in `grid` (all grid points as lanes of one Levenberg-Marquardt call) and returns the values, the sum of squares `sse` and the fitted parameters `beta`. `estimation::profile_interval(profile, sse_min, sigma2, alpha, dof)` interpolates the crossing of the threshold and returns the pair `(lower, upper)`; a side that the grid never crosses is `nan`. `estimation::chi2_quantile(q, dof)` is the threshold. The model and the data are those of [the previous page](nonlinear_fits.md): {math}`v = V_{\max} S/(K_m + S)` for six substrate concentrations, with {math}`\sigma^2 = S_{\min}/(n - p)` taken from the best fit.

## The profile of a parameter

The program profiles {math}`K_m` over seven values from `1.5` to `4` and prints the re-fitted {math}`V_{\max}`, the sum of squares and the excess {math}`(S - S_{\min})/\sigma^2` at each.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace estimation = common::estimation;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// initial rates of an enzyme measured at six substrate concentrations (mM -> umol/min)
static pluss::Series substrate() { return vec({0.5, 1.0, 2.0, 4.0, 8.0, 16.0}); }
static pluss::Series rate() { return vec({0.3201, 0.4880, 0.8052, 1.1356, 1.4274, 1.6291}); }
// the model sees a batch of parameter vectors beta [lanes, 2] = (Vmax, Km) and returns the predicted rates [lanes, 6]
static pluss::Series michaelis_menten(const pluss::Series &beta) {
  const auto vmax = beta.select(-1, 0).unsqueeze(-1), km = beta.select(-1, 1).unsqueeze(-1);
  return vmax.mul(substrate()).div(km.add(substrate()));
}


int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto fit = estimation::levenberg_marquardt(michaelis_menten, rate(), mat({{1.0, 1.0}}));
  const double sse_min = flat(fit.sse)[0], sigma2 = sse_min / 4.0;   // 6 readings - 2 parameters
  // the profile of Km: for every Km on the grid, Vmax is re-fitted
  const auto grid = vec({1.5, 2.0, 2.5, 2.75, 3.0, 3.5, 4.0});
  const auto profile = estimation::profile_likelihood(michaelis_menten, rate(), vec({1.9, 2.7}), 1, grid);
  out.writeln("Km grid            ", flat(profile.values));
  out.writeln("re-fitted Vmax     ", flat(profile.beta.select(-1, 0)));
  out.writeln("sum of squares     ", flat(profile.sse));
  out.writeln("(SSE - SSE_min) / sigma^2  ", flat(profile.sse.sub(sse_min).div(sigma2)));
}
```

Output

```text
Km grid            [1.5000, 2.0000, 2.5000, 2.7500, 3.0000, 3.5000, 4.0000]
re-fitted Vmax     [1.6213, 1.7473, 1.8598, 1.9125, 1.9633, 2.0603, 2.1525]
sum of squares     [0.0750, 0.0220, 0.0031, 0.0012, 0.0027, 0.0131, 0.0298]
(SSE - SSE_min) / sigma^2  [249.4745, 70.2029, 6.3166, 0.0000, 5.2836, 40.3138, 96.5400]
```

The excess is `0.0000` at the best value {math}`K_m = 2.75` and grows on both sides: `5.2836` at `3.0` but `6.3166` at `2.5`, and `249.4745` at `1.5` against `96.5400` at `4`. The curve is steeper below the optimum than above it, an asymmetry that a symmetric interval cannot express. The re-fitted {math}`V_{\max}` rises with {math}`K_m` (`1.6213` to `2.1525`): the two estimates are positively correlated.

## Intervals from the profile

The program profiles {math}`K_m` on a fine grid of `41` points and reads the pointwise interval (`dof = 1`) and the simultaneous interval (`dof = 2`), and compares them with the interval from the Jacobian.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace estimation = common::estimation;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// initial rates of an enzyme measured at six substrate concentrations (mM -> umol/min)
static pluss::Series substrate() { return vec({0.5, 1.0, 2.0, 4.0, 8.0, 16.0}); }
static pluss::Series rate() { return vec({0.3201, 0.4880, 0.8052, 1.1356, 1.4274, 1.6291}); }
// the model sees a batch of parameter vectors beta [lanes, 2] = (Vmax, Km) and returns the predicted rates [lanes, 6]
static pluss::Series michaelis_menten(const pluss::Series &beta) {
  const auto vmax = beta.select(-1, 0).unsqueeze(-1), km = beta.select(-1, 1).unsqueeze(-1);
  return vmax.mul(substrate()).div(km.add(substrate()));
}


int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto fit = estimation::levenberg_marquardt(michaelis_menten, rate(), mat({{1.0, 1.0}}));
  const double sse_min = flat(fit.sse)[0], sigma2 = sse_min / 4.0;
  std::vector<double> g;
  for (int i = 0; i <= 40; ++i)
    g.push_back(1.5 + 0.075 * i);
  const auto profile = estimation::profile_likelihood(michaelis_menten, rate(), vec({1.9, 2.7}), 1, vec(g));
  const auto pointwise = estimation::profile_interval(profile, sse_min, sigma2, 0.05, 1);
  const auto joint = estimation::profile_interval(profile, sse_min, sigma2, 0.05, 2);
  const auto wald = estimation::confidence_intervals(fit);
  out.writeln("threshold  chi2(0.95, 1) = ", estimation::chi2_quantile(0.95, 1), ",  chi2(0.95, 2) = ", estimation::chi2_quantile(0.95, 2));
  out.writeln("Km, profile interval (pointwise)   = [", pointwise.first, ", ", pointwise.second, "]");
  out.writeln("Km, profile interval (simultaneous) = [", joint.first, ", ", joint.second, "]");
  out.writeln("Km, Wald interval from the Jacobian = [", flat(wald.lower)[1], ", ", flat(wald.upper)[1], "]");
}
```

Output

```text
threshold  chi2(0.95, 1) = 3.8415,  chi2(0.95, 2) = 5.9915
Km, profile interval (pointwise)   = [2.5537, 2.9588]
Km, profile interval (simultaneous) = [2.5095, 3.0155]
Km, Wald interval from the Jacobian = [2.4606, 3.0388]
```

The thresholds are `3.8415` for one degree of freedom and `5.9915` for two. The pointwise profile interval is [`2.5537`, `2.9588`]; the simultaneous one is wider, [`2.5095`, `3.0155`]. The interval from the Jacobian, [`2.4606`, `3.0388`], is wider and exactly symmetric about the estimate (`0.2891` on each side). The profile interval is narrower and lopsided: its lower end is `0.196` below the estimate and its upper end `0.209` above, as the steeper rise below the optimum suggested.

## A parameter the data barely determine

The program fits only the three lowest substrate concentrations, where the curve has not bent yet, takes the stated measurement error {math}`\sigma = 0.05` and profiles {math}`V_{\max}` from `1` to `31`.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace estimation = common::estimation;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// initial rates of an enzyme measured at six substrate concentrations (mM -> umol/min)
static pluss::Series substrate() { return vec({0.5, 1.0, 2.0, 4.0, 8.0, 16.0}); }
static pluss::Series rate() { return vec({0.3201, 0.4880, 0.8052, 1.1356, 1.4274, 1.6291}); }
// the model sees a batch of parameter vectors beta [lanes, 2] = (Vmax, Km) and returns the predicted rates [lanes, 6]
static pluss::Series michaelis_menten(const pluss::Series &beta) {
  const auto vmax = beta.select(-1, 0).unsqueeze(-1), km = beta.select(-1, 1).unsqueeze(-1);
  return vmax.mul(substrate()).div(km.add(substrate()));
}


// only the three lowest substrate concentrations: the curve has not bent over yet, so Vmax and Km trade off against each other
static pluss::Series low_model(const pluss::Series &beta) {
  const auto s = vec({0.5, 1.0, 2.0});
  return beta.select(-1, 0).unsqueeze(-1).mul(s).div(beta.select(-1, 1).unsqueeze(-1).add(s));
}
int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto y = vec({0.3201, 0.4880, 0.8052});
  const auto fit = estimation::levenberg_marquardt(low_model, y, mat({{1.0, 1.0}}));
  out.writeln("fit on the low end: (Vmax, Km) = ", flat(fit.beta), ", sum of squares = ", flat(fit.sse));
  const double sse_min = flat(fit.sse)[0], sigma2 = 0.05 * 0.05;   // the instrument's stated standard deviation, 0.05
  std::vector<double> g;
  for (int i = 0; i <= 60; ++i)
    g.push_back(1.0 + 0.5 * i);
  const auto profile = estimation::profile_likelihood(low_model, y, vec({2.0, 3.0}), 0, vec(g));
  const auto interval = estimation::profile_interval(profile, sse_min, sigma2, 0.05, 1);
  out.writeln("Vmax grid 1 .. 31, profile interval = [", interval.first, ", ", interval.second, "]");
  const auto full = estimation::levenberg_marquardt(michaelis_menten, vec({0.3201, 0.4880, 0.8052, 1.1356, 1.4274, 1.6291}), mat({{1.0, 1.0}}));
  const auto wald = estimation::confidence_intervals(full);
  out.writeln("for comparison, the Wald interval for Vmax from all six readings = [", flat(wald.lower)[0], ", ", flat(wald.upper)[0], "]");
  out.writeln("excess chi2 at Vmax = 10, 20, 31    = ", flat(profile.sse.sub(sse_min).div(sigma2).select(0, 18)), " ", flat(profile.sse.sub(sse_min).div(sigma2).select(0, 38)), " ", flat(profile.sse.sub(sse_min).div(sigma2).select(0, 60)));
}
```

Output

```text
fit on the low end: (Vmax, Km) = [1.8266, 2.5731], sum of squares = [0.0011]
Vmax grid 1 .. 31, profile interval = [1.1890, 6.9898]
for comparison, the Wald interval for Vmax from all six readings = [1.8434, 1.9815]
excess chi2 at Vmax = 10, 20, 31    = [4.5999] [5.5359] [5.8809]
```

The low-end fit gives {math}`(V_{\max}, K_m) = (1.8266, 2.5731)`, close to the full fit, but the profile interval for {math}`V_{\max}` is [`1.1890`, `6.9898`], against [`1.8434`, `1.9815`] from all six readings. The excess grows from `4.5999` at `10` to only `5.8809` at `31`: a large {math}`V_{\max}` can be compensated by a large {math}`K_m`, since at low concentration only the ratio {math}`V_{\max}/K_m` is measured.

## Decisions

- Use the Jacobian interval of [the previous page](nonlinear_fits.md) as a first look and the profile when it matters: it costs one batched fit for the whole grid.
- Choose `dof` by what you claim: `1` for one parameter alone, `p` for a statement about all of them together.
- Cover the profile with a grid that extends past both crossings and is fine near them, as the interval is interpolated linearly between grid points.
- Give `sigma2` from the instrument, or estimate it as the minimum sum of squares divided by the degrees of freedom.

## Pitfalls

The interval is read off the grid, so the grid decides how good it is. The program computes the same interval from a fine grid, from seven points half a unit apart and from a grid that stops at the best value on one side.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace estimation = common::estimation;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// initial rates of an enzyme measured at six substrate concentrations (mM -> umol/min)
static pluss::Series substrate() { return vec({0.5, 1.0, 2.0, 4.0, 8.0, 16.0}); }
static pluss::Series rate() { return vec({0.3201, 0.4880, 0.8052, 1.1356, 1.4274, 1.6291}); }
// the model sees a batch of parameter vectors beta [lanes, 2] = (Vmax, Km) and returns the predicted rates [lanes, 6]
static pluss::Series michaelis_menten(const pluss::Series &beta) {
  const auto vmax = beta.select(-1, 0).unsqueeze(-1), km = beta.select(-1, 1).unsqueeze(-1);
  return vmax.mul(substrate()).div(km.add(substrate()));
}


int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto fit = estimation::levenberg_marquardt(michaelis_menten, rate(), mat({{1.0, 1.0}}));
  const double sse_min = flat(fit.sse)[0], sigma2 = sse_min / 4.0;
  std::vector<double> fine;
  for (int i = 0; i <= 40; ++i)
    fine.push_back(1.5 + 0.075 * i);
  const auto interval = [&](const pluss::Series &grid) {
    return estimation::profile_interval(estimation::profile_likelihood(michaelis_menten, rate(), vec({1.9, 2.7}), 1, grid), sse_min, sigma2, 0.05, 1);
  };
  const auto a = interval(vec(fine));
  const auto b = interval(vec({1.5, 2.0, 2.5, 2.7, 3.0, 3.5, 4.0}));   // seven points: the crossing is interpolated across wide gaps
  const auto c = interval(vec({2.7, 2.8, 2.9, 3.0, 3.1, 3.2, 3.3, 3.5}));   // never goes below the estimate on the left
  out.writeln("41 grid points, step 0.075:  [", a.first, ", ", a.second, "]");
  out.writeln("7 grid points, steps 0.5:    [", b.first, ", ", b.second, "]");
  out.writeln("grid from 2.7 to 3.5:        [", c.first, ", ", c.second, "]");
}
```

Output

```text
41 grid points, step 0.075:  [2.5537, 2.9588]
7 grid points, steps 0.5:    [2.5814, 2.9144]
grid from 2.7 to 3.5:        [nan, 2.9564]
```

The fine grid gives [`2.5537`, `2.9588`]; the coarse grid gives [`2.5814`, `2.9144`], too narrow by a few hundredths because the profile is curved between its points; the third grid never crosses the threshold on the left, and the lower end is `nan`. A `nan` end means "not found on this grid", not "unbounded". Extend the grid before reading it as open.

{math}`\sigma^2` is the variance of one reading, not the sum of squares. The program uses both.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace estimation = common::estimation;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// initial rates of an enzyme measured at six substrate concentrations (mM -> umol/min)
static pluss::Series substrate() { return vec({0.5, 1.0, 2.0, 4.0, 8.0, 16.0}); }
static pluss::Series rate() { return vec({0.3201, 0.4880, 0.8052, 1.1356, 1.4274, 1.6291}); }
// the model sees a batch of parameter vectors beta [lanes, 2] = (Vmax, Km) and returns the predicted rates [lanes, 6]
static pluss::Series michaelis_menten(const pluss::Series &beta) {
  const auto vmax = beta.select(-1, 0).unsqueeze(-1), km = beta.select(-1, 1).unsqueeze(-1);
  return vmax.mul(substrate()).div(km.add(substrate()));
}


int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(4).with_sep(""));
  const auto fit = estimation::levenberg_marquardt(michaelis_menten, rate(), mat({{1.0, 1.0}}));
  const double sse_min = flat(fit.sse)[0];
  std::vector<double> g;
  for (int i = 0; i <= 60; ++i)
    g.push_back(1.0 + 0.1 * i);
  const auto profile = estimation::profile_likelihood(michaelis_menten, rate(), vec({1.9, 2.7}), 1, vec(g));
  // sigma^2 is the residual variance SSE / (n - p), not the raw sum of squares
  const auto right = estimation::profile_interval(profile, sse_min, sse_min / 4.0, 0.05, 1);
  const auto wrong = estimation::profile_interval(profile, sse_min, sse_min, 0.05, 1);
  out.writeln("sigma^2 = SSE / (n - p): [", right.first, ", ", right.second, "]");
  out.writeln("sigma^2 = SSE:           [", wrong.first, ", ", wrong.second, "]   (a four times larger sigma^2 hides the curvature)");
}
```

Output

```text
sigma^2 = SSE / (n - p): [2.5599, 2.9564]
sigma^2 = SSE:           [2.3734, 3.1897]   (a four times larger sigma^2 hides the curvature)
```

With {math}`\sigma^2 = S_{\min}/(n-p)` the interval is [`2.5599`, `2.9564`]; with the whole sum of squares, a value four times larger, it is [`2.3734`, `3.1897`]. The wrong variance makes the data look noisier and the parameter less certain than it is.

A profile of an index that does not exist, of a grid that is not a vector, and a critical value with zero degrees of freedom are all rejected.

```cpp
#include <pluss/ma.h>
#include <pluss/tils/kit/output.h>

#include <cmath>

using namespace pluss::tils;
namespace common = pluss::ma::natsci::common;
namespace estimation = common::estimation;
static pluss::Series vec(std::vector<double> v) { return pluss::series::make(v).to(pluss::ScalarType::Float64); }
static pluss::Series mat(std::initializer_list<std::initializer_list<double>> m) { return pluss::series::make(m).to(pluss::ScalarType::Float64); }
static std::vector<double> flat(const pluss::Series &s) { return s.reshape({s.numel()}).items<double>(); }

// initial rates of an enzyme measured at six substrate concentrations (mM -> umol/min)
static pluss::Series substrate() { return vec({0.5, 1.0, 2.0, 4.0, 8.0, 16.0}); }
static pluss::Series rate() { return vec({0.3201, 0.4880, 0.8052, 1.1356, 1.4274, 1.6291}); }
// the model sees a batch of parameter vectors beta [lanes, 2] = (Vmax, Km) and returns the predicted rates [lanes, 6]
static pluss::Series michaelis_menten(const pluss::Series &beta) {
  const auto vmax = beta.select(-1, 0).unsqueeze(-1), km = beta.select(-1, 1).unsqueeze(-1);
  return vmax.mul(substrate()).div(km.add(substrate()));
}


int main() {
  OutputEngine out;
  try {
    (void)estimation::profile_likelihood(michaelis_menten, rate(), vec({1.9, 2.7}), 2, vec({2.0, 3.0}));
  } catch (const pluss::Error &e) {
    out.writeln("index 2 of 2 parameters: ", e.message());
  }
  try {
    (void)estimation::profile_likelihood(michaelis_menten, rate(), vec({1.9, 2.7}), 1, mat({{2.0, 3.0}}));
  } catch (const pluss::Error &e) {
    out.writeln("grid as a matrix:        ", e.message());
  }
  try {
    (void)estimation::chi2_quantile(0.95, 0);
  } catch (const pluss::Error &e) {
    out.writeln("zero degrees of freedom: ", e.message());
  }
}
```

Output

```text
index 2 of 2 parameters:  natsci: profile_likelihood: start must be a parameter vector and index one of its entries
grid as a matrix:         natsci: profile_likelihood: grid must be a non-empty vector
zero degrees of freedom:  natsci: chi2_quantile: need 0 < q < 1 and dof > 0
```

Each message names the argument at fault. A profile needs at least two parameters because the others are what is re-fitted.

## Where next

[Sensitivities and identifiability](sensitivities.md) predicts, from the model alone, how well the parameters could be determined by readings at given times. Models with parameters to profile are in [Biology 5: Enzymes](../../biology/bio_05_enzymes.md), [Biology 15: Ligand Binding](../../biology/bio_15_binding.md) and [Chemistry 4: Reaction Rates](../../chemistry/chem_04_rates.md).

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
</div>
<!-- apiref:nav:end -->
