<!-- generated from VarFit.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::VarFit`

The result of a vector autoregression: the coefficients and the methods that evaluate, forecast and analyse the system.

## Signature

```cpp
ts::VarFit fit = ts::Var::fit_mle(y, options);
```

## Construction

Obtained from [`Var::fit_mle`](Var.md).

## State

| Field | Meaning |
|---|---|
| `lag_order`, `include_intercept`, `include_trend` | how the model was fitted |
| `constant` | the intercepts, a vector of length `k` |
| `trend` | the trend coefficients, length `k` |
| `ar` | the lag matrices, `p × k × k`: `ar[l][j][i]` is the effect of series `j` at lag `l + 1` on series `i` |
| `beta` | the exogenous coefficients (empty without regressors) |
| `covariance` | the `k × k` maximum-likelihood covariance of the errors (the residual cross-products divided by `sample_size`) |
| `sample_size` | the number of fitted rows, `n − p` |

## Methods

### `infer`

```cpp
pluss::Series infer(const pluss::Series& y, pluss::codem::Config cfg = {}) const;
pluss::Series infer(const pluss::Series& y, const pluss::Series& x, pluss::codem::Config cfg = {}) const;
```

The one-step prediction errors (residuals) of the fitted equations on `y`, an `(n − p) × k` matrix: the rows that have a full set of lags.

`const pluss::Series& y`
: The series as an `n × k` matrix, one column per series (a vector is a single series).

`const pluss::Series& x`
: Exogenous regressors, an `n × m` matrix with one row per row of `y`.

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned series).

**Returns.** A `Series` of residuals.

### `loglik`

```cpp
double loglik(const pluss::Series& y) const;
double loglik(const pluss::Series& y, const pluss::Series& x) const;
```

The joint Gaussian log-likelihood of the fitted rows of `y`. Compare lag orders by Akaike's or Schwarz's criterion built from it.

`const pluss::Series& y`
: The series as an `n × k` matrix, one column per series (a vector is a single series).

`const pluss::Series& x`
: Exogenous regressors, an `n × m` matrix with one row per row of `y`.

**Returns.** The total as a `double`.

### `loglik_per_obs`

```cpp
pluss::Series loglik_per_obs(const pluss::Series& y, pluss::codem::Config cfg = {}) const;
pluss::Series loglik_per_obs(const pluss::Series& y, const pluss::Series& x, pluss::codem::Config cfg = {}) const;
```

The joint log-likelihood contribution of each fitted row; they sum to `loglik`.

`const pluss::Series& y`
: The series as an `n × k` matrix, one column per series (a vector is a single series).

`const pluss::Series& x`
: Exogenous regressors, an `n × m` matrix with one row per row of `y`.

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned series).

**Returns.** A `Series` of length `n − p`.

### `forecast`

```cpp
pluss::Series forecast(std::int64_t horizon, const pluss::Series& y, pluss::codem::Config cfg = {}) const;
pluss::Series forecast(std::int64_t horizon, const pluss::Series& y, const pluss::Series& x_future, pluss::codem::Config cfg = {}) const;
```

Point forecasts of every series `horizon` steps beyond the end of `y`, computed recursively. They decay to the mean of the process. A fit with regressors needs their future values `x_future`.

`std::int64_t horizon`
: Number of steps (greater than 0).

`const pluss::Series& y`
: The series as an `n × k` matrix, one column per series (a vector is a single series).

`const pluss::Series& x_future`
: The regressors over the forecast steps, `horizon` rows.

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned series).

**Returns.** A `horizon × k` `Series`.

**Errors.** `horizon` must be positive.

### `irf`

```cpp
pluss::Series irf(const ts::IrfOptions& options = {}, pluss::codem::Config cfg = {}) const;
```

The impulse responses: how the system moves in the steps after a one-standard-deviation shock to one series. Entry `[s][j][i]` is the response of series `i`, `s` steps after a shock to series `j` (step 0 is the impact). The orthogonalised responses use the Cholesky factor of the covariance, so the first series is treated as not reacting to the others in the same period; reorder the columns to change that. The generalised responses do not depend on the order. The covariance is the maximum-likelihood one.

`const ts::IrfOptions& options`
: Horizon and shock identification; see [`IrfOptions`](IrfOptions.md).

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned series).

**Returns.** A `horizon × k × k` `Series`, indexed `[step][shock][response]`.

**Errors.** `horizon` must be greater than 0.

### `fevd`

```cpp
pluss::Series fevd(const ts::FevdOptions& options = {}, pluss::codem::Config cfg = {}) const;
```

The forecast-error variance decomposition: the share of each series' forecast-error variance, `s + 1` steps ahead, that is due to each shock. Entry `[s][j][i]` is the share of series `i` due to shock `j`; the shares of each series add up to one (for the generalised method they are normalised to).

`const ts::FevdOptions& options`
: Horizon and shock identification; see [`FevdOptions`](FevdOptions.md).

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned series).

**Returns.** A `horizon × k × k` `Series`, indexed `[step][shock][series]`.

**Errors.** `horizon` must be greater than 0.

### `granger_test`

```cpp
ts::GrangerResult granger_test(const pluss::Series& y, const ts::GrangerOptions& options = {}, pluss::codem::Config cfg = {}) const;
ts::GrangerResult granger_test(const pluss::Series& y, const pluss::Series& x, const ts::GrangerOptions& options = {}, pluss::codem::Config cfg = {}) const;
```

Tests whether the lags of a candidate cause have zero coefficients in the equation of an effect: if so the series does not Granger-cause the effect. The Wald statistic (or its F form) is referred to a chi-squared (or F) distribution with one degree of freedom per restricted coefficient.

`const pluss::Series& y`
: The series as an `n × k` matrix, one column per series (a vector is a single series).

`const pluss::Series& x`
: Exogenous regressors, an `n × m` matrix with one row per row of `y`.

`const ts::GrangerOptions& options`
: Which hypotheses to test and the form of the statistic; see [`GrangerOptions`](GrangerOptions.md).

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned series).

**Returns.** A [`GrangerResult`](GrangerResult.md) with one entry per test.

**Errors.** Index lists must lie inside `0 … k − 1`.

### `simulate`

```cpp
pluss::Series simulate(std::int64_t num_observations, pluss::codem::Config cfg = {}, std::uint64_t seed = 0) const;
pluss::Series simulate(const pluss::Series& x, pluss::codem::Config cfg = {}, std::uint64_t seed = 0) const;
```

Draws a path of the fitted system with Gaussian errors of the estimated covariance. The same `seed` gives the same path.

`std::int64_t num_observations`
: Length of the path.

`const pluss::Series& x`
: Exogenous regressors, an `n × m` matrix with one row per row of `y`.

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned series).

`std::uint64_t seed`
: Seed (default 0).

**Returns.** A `num_observations × k` `Series`.

## Practical Examples

### Example 1

Impulse responses and variance shares for two linked series: the orthogonalised response at step 0 is the Cholesky factor, and the shares of each series add up to one.

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
  for (double v : s.to_vec<double>()) std::cout << std::setw(9) << v;
  std::cout << "\n";
}
std::vector<double> noise(int n, int offset) {
  std::vector<double> e(n);
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + offset + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[i] = u - 0.5;
  }
  return e;
}
// Two linked series: a_t = 0.5 a_{t-1} + 0.2 b_{t-1} + e1_t,  b_t = 0.1 a_{t-1} + 0.6 b_{t-1} + 0.5 e1_t + e2_t   (n x 2, row t = (a_t, b_t)).
Series linked(int n) {
  const auto e1 = noise(n, 0), e2 = noise(n, 5000);
  std::vector<double> y(2 * n);
  double a = 0.0, b = 0.0;
  for (int t = 0; t < n; ++t) {
    const double na = 0.5 * a + 0.2 * b + e1[t];
    const double nb = 0.1 * a + 0.6 * b + 0.5 * e1[t] + e2[t];
    a = na;
    b = nb;
    y[2 * t] = a;
    y[2 * t + 1] = b;
  }
  return pluss::make_series(y, {n, 2}, f64);
}

int main() {
  const Series y = linked(300);
  const auto fit = ts::Var::fit_mle(y);
  ts::IrfOptions io;
  io.horizon = 4;
  const Series irf = fit.irf(io);
  std::cout << std::fixed << std::setprecision(4);
  for (int s = 0; s < 4; ++s) {
    const auto v = irf.select(0, s).to_vec<double>();
    std::cout << "step " << s << "  shock 0 -> (" << v[0] << ", " << v[1] << ")   shock 1 -> (" << v[2] << ", " << v[3] << ")\n";
  }
  ts::FevdOptions fo;
  fo.horizon = 4;
  const auto f = fit.fevd(fo).select(0, 3).to_vec<double>();
  std::cout << "variance of series 0 at step 4 due to shock 0: " << f[0] << ", shock 1: " << f[2] << "\n";
}
```

Output

```text
step 0  shock 0 -> (0.2815, 0.1298)   shock 1 -> (0.0000, 0.2845)
step 1  shock 0 -> (0.1657, 0.1165)   shock 1 -> (0.0417, 0.1533)
step 2  shock 0 -> (0.1034, 0.0901)   shock 1 -> (0.0442, 0.0895)
step 3  shock 0 -> (0.0671, 0.0657)   shock 1 -> (0.0361, 0.0555)
variance of series 0 at step 4 due to shock 0: 0.9606, shock 1: 0.0394
```

### Example 2

Granger tests of every ordered pair, in the Wald and the F form.

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
  for (double v : s.to_vec<double>()) std::cout << std::setw(9) << v;
  std::cout << "\n";
}
std::vector<double> noise(int n, int offset) {
  std::vector<double> e(n);
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + offset + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[i] = u - 0.5;
  }
  return e;
}
// Two linked series: a_t = 0.5 a_{t-1} + 0.2 b_{t-1} + e1_t,  b_t = 0.1 a_{t-1} + 0.6 b_{t-1} + 0.5 e1_t + e2_t   (n x 2, row t = (a_t, b_t)).
Series linked(int n) {
  const auto e1 = noise(n, 0), e2 = noise(n, 5000);
  std::vector<double> y(2 * n);
  double a = 0.0, b = 0.0;
  for (int t = 0; t < n; ++t) {
    const double na = 0.5 * a + 0.2 * b + e1[t];
    const double nb = 0.1 * a + 0.6 * b + 0.5 * e1[t] + e2[t];
    a = na;
    b = nb;
    y[2 * t] = a;
    y[2 * t + 1] = b;
  }
  return pluss::make_series(y, {n, 2}, f64);
}

int main() {
  const Series y = linked(300);
  ts::VarOptions options;
  options.lag_order = 2;
  const auto fit = ts::Var::fit_mle(y, options);
  for (auto kind : {ts::GrangerStat::ChiSquare, ts::GrangerStat::F}) {
    ts::GrangerOptions g;
    g.stat = kind;
    const auto r = fit.granger_test(y, g);
    const auto cause = r.cause.to_vec<std::int64_t>(), effect = r.effect.to_vec<std::int64_t>();
    const auto stat = r.stat.to_vec<double>(), p = r.pvalue.to_vec<double>();
    for (std::size_t i = 0; i < cause.size(); ++i)
      std::cout << (kind == ts::GrangerStat::F ? "F    " : "Wald ") << "series " << cause[i] << " -> " << effect[i] << "  statistic " << std::fixed << std::setprecision(3) << stat[i] << "  p " << p[i] << "\n";
  }
}
```

Output

```text
Wald series 1 -> 0  statistic 9.698  p 0.008
Wald series 0 -> 1  statistic 6.501  p 0.039
F    series 1 -> 0  statistic 4.849  p 0.008
F    series 0 -> 1  statistic 3.251  p 0.040
```

### Example 3

Forecasts, the residuals, and a simulated path of the fitted system.

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
  for (double v : s.to_vec<double>()) std::cout << std::setw(9) << v;
  std::cout << "\n";
}
std::vector<double> noise(int n, int offset) {
  std::vector<double> e(n);
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + offset + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[i] = u - 0.5;
  }
  return e;
}
// Two linked series: a_t = 0.5 a_{t-1} + 0.2 b_{t-1} + e1_t,  b_t = 0.1 a_{t-1} + 0.6 b_{t-1} + 0.5 e1_t + e2_t   (n x 2, row t = (a_t, b_t)).
Series linked(int n) {
  const auto e1 = noise(n, 0), e2 = noise(n, 5000);
  std::vector<double> y(2 * n);
  double a = 0.0, b = 0.0;
  for (int t = 0; t < n; ++t) {
    const double na = 0.5 * a + 0.2 * b + e1[t];
    const double nb = 0.1 * a + 0.6 * b + 0.5 * e1[t] + e2[t];
    a = na;
    b = nb;
    y[2 * t] = a;
    y[2 * t + 1] = b;
  }
  return pluss::make_series(y, {n, 2}, f64);
}

int main() {
  const Series y = linked(300);
  const auto fit = ts::Var::fit_mle(y);
  const Series f = fit.forecast(3, y);
  for (int h = 0; h < 3; ++h) show("forecast", f.select(0, h));
  std::cout << "residuals: " << fit.infer(y).sizes()[0] << " rows of " << fit.infer(y).sizes()[1] << "\n";
  show("simulated 0", fit.simulate(3, f64, 5ULL).select(0, 0));
}
```

Output

```text
forecast         0.0149   0.0190
forecast         0.0131   0.0138
forecast         0.0113   0.0107
residuals: 299 rows of 2
simulated 0      0.0081  -0.3884
```


## Changelog

- **0.1** — Initial documented surface.

## See Also

- [`Var`](Var.md)
- [`IrfOptions`](IrfOptions.md)
- [`GrangerOptions`](GrangerOptions.md)
- [`var_to_vecm`](var_to_vecm.md)

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
