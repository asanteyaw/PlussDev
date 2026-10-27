<!-- generated from acf.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::acf`

The sample autocorrelation function of a series: its correlation with itself at lags 0 through `max_lag`.

## Signature

```cpp
pluss::Series ts::acf(const pluss::Series& x, const ts::AcfOptions& options = {}, pluss::codem::Config cfg = {});
```

## What It Does

`acf` returns `r[k]` for `k = 0 … max_lag`, where `r[k]` is the autocovariance at lag `k` divided by the variance. `r[0]` is always 1. A bar that stands out from the white-noise band `±1.96/√n` is evidence that the series depends on its past at that lag. Read together with [`pacf`](pacf.md): an autoregression of order `p` has autocorrelations that decay and partial autocorrelations that vanish after lag `p`; a moving average of order `q` has autocorrelations that vanish after lag `q`.

The default normalisation divides each autocovariance by `n` (`AcfNorm::Biased`), which guarantees the estimates form a valid autocorrelation sequence; `AcfNorm::Unbiased` divides by `n − k`, which removes the downward bias at high lags at the cost of noisier estimates. A `max_lag` as large as the series is clamped to the longest lag that exists.

## Parameters

`const pluss::Series& x`
: A one-dimensional series in time order, at least two observations long.

`const ts::AcfOptions& options`
: `max_lag` (default 20), `norm` (`Biased` default or `Unbiased`) and `demean` (subtract the sample mean first, default true); see [`AcfOptions`](AcfOptions.md).

`pluss::codem::Config cfg`
: Output configuration (dtype and platform of the returned series); the computation itself runs in double precision.

## Returns

A `Series` of `max_lag + 1` values, lag 0 first.

## Errors

- A series shorter than two observations is rejected.

## Practical Examples

### Example 1

The autocorrelations of an autoregressive series with coefficient 0.7 decay roughly geometrically.

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
std::vector<double> noise(int n) {
  std::vector<double> e(n);
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[i] = u - 0.5;
  }
  return e;
}
Series ar1(int n, double phi) {
  const auto e = noise(n);
  std::vector<double> x(n);
  double carry = 0.0;
  for (int i = 0; i < n; ++i) {
    carry = phi * carry + e[i];
    x[i] = carry;
  }
  return pluss::make_series(x, {n}, f64);
}
Series walk(int n) {
  const auto e = noise(n);
  std::vector<double> x(n);
  double sum = 0.0;
  for (int i = 0; i < n; ++i) {
    sum += e[i];
    x[i] = sum;
  }
  return pluss::make_series(x, {n}, f64);
}

int main() {
  const Series y = ar1(200, 0.7);
  ts::AcfOptions options;
  options.max_lag = 5;
  show("lag", pluss::make_series({0.0, 1.0, 2.0, 3.0, 4.0, 5.0}, f64));
  show("acf", ts::acf(y, options));
}
```

Output

```text
lag              0.0000   1.0000   2.0000   3.0000   4.0000   5.0000
acf              1.0000   0.6502   0.4426   0.4037   0.3413   0.2859
```

### Example 2

The unbiased normalisation inflates the higher lags slightly compared with the biased one.

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
std::vector<double> noise(int n) {
  std::vector<double> e(n);
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[i] = u - 0.5;
  }
  return e;
}
Series ar1(int n, double phi) {
  const auto e = noise(n);
  std::vector<double> x(n);
  double carry = 0.0;
  for (int i = 0; i < n; ++i) {
    carry = phi * carry + e[i];
    x[i] = carry;
  }
  return pluss::make_series(x, {n}, f64);
}
Series walk(int n) {
  const auto e = noise(n);
  std::vector<double> x(n);
  double sum = 0.0;
  for (int i = 0; i < n; ++i) {
    sum += e[i];
    x[i] = sum;
  }
  return pluss::make_series(x, {n}, f64);
}

int main() {
  const Series y = ar1(200, 0.7);
  ts::AcfOptions biased, unbiased;
  biased.max_lag = unbiased.max_lag = 5;
  unbiased.norm = ts::AcfNorm::Unbiased;
  show("biased", ts::acf(y, biased));
  show("unbiased", ts::acf(y, unbiased));
}
```

Output

```text
biased           1.0000   0.6502   0.4426   0.4037   0.3413   0.2859
unbiased         1.0000   0.6534   0.4471   0.4099   0.3482   0.2933
```

### Example 3

With `demean = false` the mean is not removed: a series with a large level has autocorrelations near 1 at every lag.

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
std::vector<double> noise(int n) {
  std::vector<double> e(n);
  for (int i = 0; i < n; ++i) {
    double u = std::sin(12.9898 * (i + 1.0) + 78.233) * 43758.5453;
    u -= std::floor(u);
    e[i] = u - 0.5;
  }
  return e;
}
Series ar1(int n, double phi) {
  const auto e = noise(n);
  std::vector<double> x(n);
  double carry = 0.0;
  for (int i = 0; i < n; ++i) {
    carry = phi * carry + e[i];
    x[i] = carry;
  }
  return pluss::make_series(x, {n}, f64);
}
Series walk(int n) {
  const auto e = noise(n);
  std::vector<double> x(n);
  double sum = 0.0;
  for (int i = 0; i < n; ++i) {
    sum += e[i];
    x[i] = sum;
  }
  return pluss::make_series(x, {n}, f64);
}

int main() {
  const Series y = ar1(200, 0.0) + 10.0;
  ts::AcfOptions with_mean, no_mean;
  with_mean.max_lag = no_mean.max_lag = 3;
  no_mean.demean = false;
  show("demeaned", ts::acf(y, with_mean));
  show("not demeaned", ts::acf(y, no_mean));
}
```

Output

```text
demeaned         1.0000  -0.0692  -0.1372   0.0919
not demeaned     1.0000   0.9940   0.9890   0.9841
```


## See Also

- [`pacf`](pacf.md)
- [`ljung_box`](ljung_box.md)
- [`AcfOptions`](AcfOptions.md)
