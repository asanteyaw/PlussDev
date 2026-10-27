<!-- generated from Gjr.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::Gjr`

Fits a GJR-GARCH(p, q) model by maximum likelihood.

## Signature

```cpp
ts::GjrFit ts::Gjr::fit_mle(const pluss::Series& y, const ts::GjrOptions& options = {}, pluss::codem::Config cfg = {});
```

## Construction

`Gjr` has no instances: it is a namespace for the static `fit_mle`, which returns a [`GjrFit`](GjrFit.md).

## Methods

### `fit_mle`

```cpp
ts::GjrFit ts::Gjr::fit_mle(const pluss::Series& y, const ts::GjrOptions& options = {}, pluss::codem::Config cfg = {});
```

Fits the model to the return series `y` by maximising the Gaussian log-likelihood with a restarted Nelder–Mead search over an unconstrained parametrisation of the parameters. The recursion is `h_t = ω + Σ (α_i + γ_i 1[e_{t−i} < 0]) e²_{t−i} + Σ β_j h_{t−j}` with `γ_i ≥ 0` (a negative shock raises the variance more than a positive one) and `Σα + Σγ/2 + Σβ < 1`. The sample mean is removed first (unless `demean` is off) and held fixed. The recursion starts from the mean squared residual.

`const pluss::Series& y`
: The return series in time order (at least 8 observations; a vector or an `n × 1` matrix).

`const ts::GjrOptions& options`
: Orders, starting values and search settings; see [`GjrOptions`](GjrOptions.md).

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned series); computation is in double precision.

**Returns.** A [`GjrFit`](GjrFit.md).

**Errors.** At least 8 observations are required. `variance_floor`, `stationarity_margin`, the iteration limits and `nm_restarts` must be positive and in range.

## Practical Examples

### Example 1

Fit the model to a series with volatility clustering and print the estimates.

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
  const auto fit = ts::Gjr::fit_mle(y);
  std::cout << std::fixed << std::setprecision(4) << "log-likelihood " << fit.loglik(y) << ", persistence " << fit.persistence() << "\n";
}
```

Output

```text
log-likelihood -1012.7488, persistence 0.8869
```

### Example 2

A series that is too short is refused.

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
  try {
    ts::Gjr::fit_mle(pluss::make_series({1.0, 2.0, 3.0}, f64));
  } catch (const std::exception&) {
    std::cout << "fewer than 8 observations are rejected\n";
  }
}
```

Output

```text
fewer than 8 observations are rejected
```


## Changelog

- **0.1** — Initial documented surface.

## See Also

- [`GjrOptions`](GjrOptions.md)
- [`GjrFit`](GjrFit.md)

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
