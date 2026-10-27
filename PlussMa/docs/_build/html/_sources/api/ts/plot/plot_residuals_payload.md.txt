<!-- generated from plot_residuals_payload.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::plot_residuals_payload`

The numbers behind a residual plot: the time index and the residuals.

## Signature

```cpp
ts::plot::ResidualPlotPayload ts::plot::residuals_payload(const pluss::Series& residuals, pluss::codem::Config cfg = {});
```

## What It Does

`residuals_payload` pairs each residual with its position in time, for tables or for plotting with other tools; [`residuals`](plot_residuals.md) draws it.

## Parameters

`const pluss::Series& residuals`
: The residual series, for example `fit.infer(y)` of a model.

`pluss::codem::Config cfg`
: Output configuration of the payload arrays.

## Returns

A [`ResidualPlotPayload`](ResidualPlotPayload.md).

## Practical Examples

### Example 1

Residuals of an AR(1) fit, with their time index.

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
  const Series y = ar1(300, 0.7);
  const auto residuals = ts::Arima::fit_mle(y).infer(y);
  const auto payload = ts::plot::residuals_payload(residuals);
  std::cout << "index and residual count: " << payload.index.numel() << " and " << payload.residuals.numel() << "\n";
}
```

Output

```text
index and residual count: 300 and 300
```


## See Also

- [`residuals`](plot_residuals.md)
