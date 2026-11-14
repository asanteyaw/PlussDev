<!-- generated from EgarchFit.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::EgarchFit`

The result of a `Egarch` fit: the estimated parameters and the methods that evaluate the fitted model.

## Signature

```cpp
ts::EgarchFit fit = ts::Egarch::fit_mle(y, options);
```

## Construction

Obtained from [`Egarch::fit_mle`](Egarch.md).

## State

| Field | Meaning |
|---|---|
| `mean` | the sample mean removed before fitting |
| `omega` | the constant of the log-variance equation |
| `arch`, `asym`, `garch` | the magnitude, sign and persistence coefficient vectors |
| `variance_floor`, `stationarity_margin`, `finite_diff_eps` | settings the methods use |

## Methods

### `persistence`

```cpp
double persistence() const;
```

The persistence `Σβ` of the log variance.

**Returns.** The persistence as a `double`.

### `infer`

```cpp
pluss::Series infer(const pluss::Series& y, pluss::codem::Config cfg = {}) const;
```

The fitted conditional variances `h_t` of `y`, one per observation: the model's estimate of the variance of each return given the past. Dividing `(y − mean)` by their square roots gives the standardised residuals, which should show no remaining clustering (test them with [`arch_lm`](../diagnostics/arch_lm.md)).

`const pluss::Series& y`
: The return series in time order (at least 8 observations; a vector or an `n × 1` matrix).

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned series); computation is in double precision.

**Returns.** A `Series` of variances, the length of `y`.

### `loglik`

```cpp
double loglik(const pluss::Series& y) const;
```

The Gaussian log-likelihood of `y` under the fitted parameters. Compare models by it and by Akaike's criterion.

`const pluss::Series& y`
: The return series in time order (at least 8 observations; a vector or an `n × 1` matrix).

**Returns.** The total log-likelihood as a `double`.

### `loglik_per_obs`

```cpp
pluss::Series loglik_per_obs(const pluss::Series& y, pluss::codem::Config cfg = {}) const;
```

The contribution of each observation to the log-likelihood; they sum to `loglik`.

`const pluss::Series& y`
: The return series in time order (at least 8 observations; a vector or an `n × 1` matrix).

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned series); computation is in double precision.

**Returns.** A `Series`, one value per observation.

### `score`

```cpp
pluss::Series score(const pluss::Series& y, pluss::codem::Config cfg = {}) const;
```

The gradient of the log-likelihood with respect to the parameters (`ω`, the magnitude coefficients, the sign coefficients, then the persistence coefficients), by central differences. It is close to zero at a maximum.

`const pluss::Series& y`
: The return series in time order (at least 8 observations; a vector or an `n × 1` matrix).

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned series); computation is in double precision.

**Returns.** A `Series` with one entry per parameter.

### `observed_inf`

```cpp
pluss::Series observed_inf(const pluss::Series& y, pluss::codem::Config cfg = {}) const;
```

The observed information matrix (minus the numerical Hessian of the log-likelihood) in the parameter order of `score`. Its inverse estimates the covariance of the estimates.

`const pluss::Series& y`
: The return series in time order (at least 8 observations; a vector or an `n × 1` matrix).

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned series); computation is in double precision.

**Returns.** A square `Series`, one row and column per parameter.

### `forecast`

```cpp
pluss::Series forecast(std::int64_t horizon, const pluss::Series& y, pluss::codem::Config cfg = {}) const;
```

Forecasts the conditional variance `horizon` steps beyond `y`. Each forecast mean-reverts geometrically to the unconditional variance at the rate of the persistence; the one-step forecast uses the last observed shock and variance. (For `EgarchFit` the forecast is the exact expectation of the variance given the data: the one-step value is known and each later step multiplies in the expectation of the exponential of the unknown shocks, which is available in closed form for Gaussian shocks.)

`std::int64_t horizon`
: Number of steps ahead (greater than 0).

`const pluss::Series& y`
: The return series in time order (at least 8 observations; a vector or an `n × 1` matrix).

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned series); computation is in double precision.

**Returns.** A `Series` of `horizon` variance forecasts.

**Errors.** `horizon` must be positive.

### `simulate`

```cpp
pluss::Series simulate(std::int64_t num_observations, pluss::codem::Config cfg = {}, std::uint64_t seed = 0) const;
```

Draws a return series from the fitted model with Gaussian shocks. The same `seed` gives the same series.

`std::int64_t num_observations`
: Length of the simulated series.

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned series); computation is in double precision.

`std::uint64_t seed`
: Seed of the random stream (default 0, a fixed stream).

**Returns.** A `Series` of simulated returns.

## Practical Examples

### Example 1

Fit the model, check the first-order condition (the score is zero at the maximum) and compare the fitted variances with their long-run level.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
void show(const char* label, const Series& s) {
  std::cout << std::left << std::setw(14) << label << std::right << std::fixed << std::setprecision(4);
  for (double v : s.host<double>()) std::cout << std::setw(9) << v;
  std::cout << "\n";
}
// Returns whose variance responds more to negative shocks: h = 0.05 + (0.05 + 0.20 * 1[r < 0]) r_prev^2 + 0.75 h_prev.
Series returns(int n) {
  std::vector<double> r(n);
  double h = 0.5, prev = 0.0;
  for (int t = 0; t < n; ++t) {
    double u = std::sin(12.9898 * (t + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    const double z = (u - 0.5) * std::sqrt(12.0);
    h = 0.05 + (0.05 + (prev < 0 ? 0.20 : 0.0)) * prev * prev + 0.75 * h;
    r[t] = std::sqrt(h) * z;
    prev = r[t];
  }
  return pluss::make_series(r, {n}, f64);
}

int main() {
  const Series y = returns(1000);
  const auto fit = ts::Egarch::fit_mle(y);
  show("score", fit.score(y));
  const Series h = fit.infer(y);
  std::cout << std::fixed << std::setprecision(4) << "mean of fitted variances " << h.mean().item<double>() << ", log-likelihood " << fit.loglik(y) << "\n";
}
```

Output

```text
score            0.0001   0.0000   0.0000  -0.0001
mean of fitted variances 0.4693, log-likelihood -1013.4849
```

### Example 2

Forecast the variance and simulate a series; the same seed reproduces it.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

namespace ts = pluss::subsp::stats::ts;
using pluss::Series;

const auto f64 = pluss::Config{}.with_dtype(pluss::ScalarType::Float64);
void show(const char* label, const Series& s) {
  std::cout << std::left << std::setw(14) << label << std::right << std::fixed << std::setprecision(4);
  for (double v : s.host<double>()) std::cout << std::setw(9) << v;
  std::cout << "\n";
}
// Returns whose variance responds more to negative shocks: h = 0.05 + (0.05 + 0.20 * 1[r < 0]) r_prev^2 + 0.75 h_prev.
Series returns(int n) {
  std::vector<double> r(n);
  double h = 0.5, prev = 0.0;
  for (int t = 0; t < n; ++t) {
    double u = std::sin(12.9898 * (t + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    const double z = (u - 0.5) * std::sqrt(12.0);
    h = 0.05 + (0.05 + (prev < 0 ? 0.20 : 0.0)) * prev * prev + 0.75 * h;
    r[t] = std::sqrt(h) * z;
    prev = r[t];
  }
  return pluss::make_series(r, {n}, f64);
}

int main() {
  const Series y = returns(1000);
  const auto fit = ts::Egarch::fit_mle(y);
  show("forecast", fit.forecast(5, y));
  const Series a = fit.simulate(6, f64, 3ULL), b = fit.simulate(6, f64, 3ULL);
  show("simulated", a);
  std::cout << "same seed, same draws: " << std::boolalpha << ((a - b).abs().max().item<double>() == 0.0) << "\n";
}
```

Output

```text
forecast         0.4043   0.4100   0.4143   0.4176   0.4201
simulated       -0.3839  -1.2628   0.7436   0.3521  -0.7759  -0.9828
same seed, same draws: true
```


## Changelog

- **0.1** — Initial documented surface.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="Egarch.html">
    <span class="label">Previous</span>
    <span class="title">Egarch</span>
  </a>
  <a class="next-link" href="EgarchOptions.html">
    <span class="label">Next</span>
    <span class="title">EgarchOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
