<!-- generated from zscore_rolling.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::zscore_rolling`

The rolling z-score: each observation minus the window mean, divided by the window standard deviation.

## Signature

```cpp
pluss::Series ts::zscore_rolling(const pluss::Series& x, std::int64_t window, std::int64_t min_periods = 1, double epsilon = 1e-12, std::int64_t ddof = 1);
```

## What It Does

`zscore_rolling` standardises each observation against the window that ends at it: `(x[t] − mean) / (std + epsilon)`, where the mean and standard deviation (divisor `count − ddof`) are those of the trailing window including `x[t]`. A value far from zero marks an observation unusual for its recent past, which makes it a simple outlier or regime flag. Positions before `min_periods` observations are `NaN`.

## Parameters

`const pluss::Series& x`
: A one-dimensional series in time order. A series with more than one dimension is rejected.

`std::int64_t window`
: The number of consecutive observations in each window; it must be greater than 0.

`std::int64_t min_periods`
: The number of observations a window must hold before its statistic is reported (default 1); earlier positions are `NaN`. It cannot exceed `window`.

`double epsilon`
: A floor added under the standard deviation before dividing, so a window with no variation gives a finite value instead of an infinite one (default 1e-12).

`std::int64_t ddof`
: Delta degrees of freedom: the divisor is the window size minus `ddof`. The default 1 gives the sample statistic, 0 the population one. A window with no more than `ddof` observations gives `NaN`.

## Returns

A `Series` of the same length as `x`.

## Errors

- `window` must be greater than 0.
- `min_periods` cannot exceed `window`.

## Practical Examples

### Example 1

A four-observation rolling z-score, withheld until the window is full.

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

int main() {
  const Series x = pluss::make_series({10.0, 12.0, 11.0, 15.0, 14.0, 18.0, 17.0}, f64);
  show("x", x);
  show("zscore_rolling", ts::zscore_rolling(x, 4, 4));
}
```

Output

```text
x               10.0000  12.0000  11.0000  15.0000  14.0000  18.0000  17.0000
zscore_rolling      nan      nan      nan   1.3887   0.5477   1.2124   0.5477
```

### Example 2

A spike stands out: the z-score of the jump is the largest in the series.

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

int main() {
  const Series x = pluss::make_series({1.0, 1.1, 0.9, 1.0, 1.1, 5.0, 1.0}, f64);
  show("x", x);
  show("zscore", ts::zscore_rolling(x, 5, 5));
}
```

Output

```text
x                1.0000   1.1000   0.9000   1.0000   1.1000   5.0000   1.0000
zscore              nan      nan      nan      nan   0.9562   1.7869  -0.4469
```


## See Also

- [`rolling_mean`](rolling_mean.md)
- [`rolling_std`](rolling_std.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rolling_var.html">
    <span class="label">Previous</span>
    <span class="title">rolling_var</span>
  </a>
</div>
<!-- apiref:nav:end -->
