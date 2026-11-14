<!-- generated from VecmFit.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::VecmFit`

The result of a vector error-correction fit.

## Signature

```cpp
ts::VecmFit fit = ts::Vecm::fit_mle(y, options);
```

## Construction

Obtained from [`Vecm::fit_mle`](Vecm.md).

## State

| Field | Meaning |
|---|---|
| `lag_order`, `rank`, `include_intercept`, `include_trend` | how the model was fitted |
| `beta` | the cointegrating vectors, `k × r`, normalised so the leading `r × r` block is the identity (`y_{t−1} β` is stationary) |
| `alpha` | the adjustment loadings, `k × r` |
| `impact` | `β α'`: in `Δy_t = y_{t−1} impact + …` (row convention) |
| `gamma` | the short-run matrices, `(p − 1) × k × k` |
| `constant`, `trend` | the deterministic terms |
| `covariance` | the maximum-likelihood covariance of the errors |
| `sample_size` | the number of fitted rows |

## Methods

### `infer`

```cpp
pluss::Series infer(const pluss::Series& y, pluss::codem::Config cfg = {}) const;
```

The one-step errors of the differenced equations, `(n − p) × k`.

`const pluss::Series& y`
: The series as an `n × k` matrix, one column per series (a vector is a single series).

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned series).

**Returns.** A `Series` of residuals.

### `loglik`

```cpp
double loglik(const pluss::Series& y) const;
```

The joint Gaussian log-likelihood of the fitted rows.

`const pluss::Series& y`
: The series as an `n × k` matrix, one column per series (a vector is a single series).

**Returns.** A `double`.

### `loglik_per_obs`

```cpp
pluss::Series loglik_per_obs(const pluss::Series& y, pluss::codem::Config cfg = {}) const;
```

The log-likelihood contribution of each fitted row.

`const pluss::Series& y`
: The series as an `n × k` matrix, one column per series (a vector is a single series).

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned series).

**Returns.** A `Series` of length `n − p`.

### `forecast`

```cpp
pluss::Series forecast(std::int64_t horizon, const pluss::Series& y, pluss::codem::Config cfg = {}) const;
```

Forecasts of the levels `horizon` steps ahead: the series stay tied by the long-run relations while wandering at the estimated drift.

`std::int64_t horizon`
: Number of steps (greater than 0).

`const pluss::Series& y`
: The series as an `n × k` matrix, one column per series (a vector is a single series).

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned series).

**Returns.** A `horizon × k` `Series`.

**Errors.** `horizon` must be positive.

### `simulate`

```cpp
pluss::Series simulate(std::int64_t num_observations, pluss::codem::Config cfg = {}, std::uint64_t seed = 0) const;
```

Draws a path of the levels from the fitted model, starting from zero.

`std::int64_t num_observations`
: Length of the path.

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned series).

`std::uint64_t seed`
: Seed (default 0).

**Returns.** A `num_observations × k` `Series`.

## Practical Examples

### Example 1

Forecast the levels from a fitted rank-1 model: the first two series stay tied.

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
std::vector<double> noise(int n, int offset) {
  std::vector<double> e(n);
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + offset + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[i] = u - 0.5;
  }
  return e;
}
// Three series: x is a random walk, y = x + a stationary AR(1) (so y is tied to x), w is an unrelated random walk.
Series three(int n) {
  const auto e1 = noise(n, 0), e2 = noise(n, 5000), e3 = noise(n, 9000);
  std::vector<double> y(3 * n);
  double x = 0.0, u = 0.0, w = 0.0;
  for (int t = 0; t < n; ++t) {
    x += e1[t];
    u = 0.6 * u + e2[t];
    w += e3[t];
    y[3 * t] = x;
    y[3 * t + 1] = x + u;
    y[3 * t + 2] = w;
  }
  return pluss::make_series(y, {n, 3}, f64);
}

int main() {
  const Series y = three(400);
  ts::VecmOptions options;
  options.rank = 1;
  const auto fit = ts::Vecm::fit_mle(y, options);
  const Series f = fit.forecast(3, y);
  for (int h = 0; h < 3; ++h) show("forecast", f.select(0, h));
  show("last observed", y.select(0, 399));
}
```

Output

```text
forecast         4.9015   4.9188   2.5817
forecast         4.9139   4.9437   2.5866
forecast         4.9259   4.9628   2.5920
last observed    4.8885   4.8833   2.5779
```


## Changelog

- **0.1** — Initial documented surface.

## See Also

- [`Vecm`](Vecm.md)

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="vecm_to_var.html">
    <span class="label">Previous</span>
    <span class="title">vecm_to_var</span>
  </a>
  <a class="next-link" href="VecmOptions.html">
    <span class="label">Next</span>
    <span class="title">VecmOptions</span>
  </a>
</div>
<!-- apiref:nav:end -->
