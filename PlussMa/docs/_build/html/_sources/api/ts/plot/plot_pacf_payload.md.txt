<!-- generated from plot_pacf_payload.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::plot_pacf_payload`

The numbers behind the partial autocorrelation plot: lags, values and the white-noise band.

## Signature

```cpp
ts::plot::CorrPlotPayload ts::plot::pacf_payload(const pluss::Series& x, const ts::plot::CorrPlotOptions& options = {}, pluss::codem::Config cfg = {});
```

## What It Does

`pacf_payload` returns the lags `1 … max_lag` (from 0 when `include_zero_lag`) and the sample partial autocorrelations at them. The band is `±z/√n` with `z` the normal quantile of `confidence_level` (1.96/√n at 95%): under no autocorrelation about one value in twenty lies outside it. Use the payload for tables or for plotting with your own tools; [`pacf`](plot_pacf.md) draws it.

## Parameters

`const pluss::Series& x`
: The series (a vector), at least two observations.

`const ts::plot::CorrPlotOptions& options`
: Lags, estimator and band; see [`CorrPlotOptions`](CorrPlotOptions.md).

`pluss::codem::Config cfg`
: Output configuration of the payload arrays.

## Returns

A [`CorrPlotPayload`](CorrPlotPayload.md).

## Errors

- A series shorter than two observations is rejected.

## Practical Examples

### Example 1

The partial autocorrelation values and band of an autoregressive series (coefficient 0.7, 400 observations).

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <pluss/pl/studio.h>

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
  const Series y = ar1(400, 0.7);
  ts::plot::CorrPlotOptions options;
  options.max_lag = 8;
  const auto p = ts::plot::pacf_payload(y, options);
  const auto lags = p.lags.host<std::int64_t>();
  const auto v = p.values.host<double>();
  std::cout << std::fixed << std::setprecision(3) << "95% band: +/- " << p.conf_high << "\n";
  for (std::size_t i = 0; i < lags.size(); ++i) std::cout << "lag " << lags[i] << "  " << v[i] << "\n";
}
```

Output

```text
95% band: +/- 0.098
lag 1  0.698
lag 2  -0.028
lag 3  0.125
lag 4  -0.028
lag 5  0.026
lag 6  -0.056
lag 7  0.006
lag 8  -0.017
```

### Example 2

`include_zero_lag` adds lag 0; a different `confidence_level` changes the band.

```cpp
#include <pluss/ma.h>

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <pluss/pl/studio.h>

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
  const Series y = ar1(400, 0.7);
  ts::plot::CorrPlotOptions options;
  options.max_lag = 3;
  options.include_zero_lag = true;
  options.confidence_level = 0.99;
  const auto p = ts::plot::pacf_payload(y, options);
  std::cout << "lags: ";
  for (auto l : p.lags.host<std::int64_t>()) std::cout << l << " ";
  std::cout << std::fixed << std::setprecision(3) << "\n99% band: +/- " << p.conf_high << "\n";
}
```

Output

```text
lags: 0 1 2 3 
99% band: +/- 0.129
```


## See Also

- [`pacf`](plot_pacf.md)
- [`CorrPlotOptions`](CorrPlotOptions.md)
