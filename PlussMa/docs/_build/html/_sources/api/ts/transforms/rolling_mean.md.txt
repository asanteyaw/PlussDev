<!-- generated from rolling_mean.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::rolling_mean`

The mean of each window of `window` consecutive observations.

## Signature

```cpp
pluss::Series ts::rolling_mean(const pluss::Series& x, std::int64_t window, std::int64_t min_periods = 1);
```

## What It Does

`rolling_mean` slides a window of `window` consecutive observations along the series and reports the arithmetic mean of each window at the window's last position. It smooths a series: the longer the window, the smoother the result and the later it reacts. A position is reported only when its window holds at least `min_periods` observations; before that the result is `NaN`, so a window that has not yet filled is either reported (small `min_periods`) or withheld (`min_periods = window`).

## Parameters

`const pluss::Series& x`
: A one-dimensional series in time order. A series with more than one dimension is rejected.

`std::int64_t window`
: The number of consecutive observations in each window; it must be greater than 0.

`std::int64_t min_periods`
: The number of observations a window must hold before its statistic is reported (default 1); earlier positions are `NaN`. It cannot exceed `window`.

## Returns

A `Series` of the same length as `x`: position `t` holds the statistic of `x[t − window + 1 .. t]`.

## Errors

- `window` must be greater than 0.
- `min_periods` cannot exceed `window`.
- A series with more than one dimension is rejected.

## Practical Examples

### Example 1

A window of three observations. With `min_periods = 1` the early windows report what they have; with `min_periods = 3` the first two positions are `NaN`.

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
  show("rolling_mean(x, 3)", ts::rolling_mean(x, 3));
  show("rolling_mean(x, 3, 3)", ts::rolling_mean(x, 3, 3));
}
```

Output

```text
x               10.0000  12.0000  11.0000  15.0000  14.0000  18.0000  17.0000
rolling_mean(x, 3)  10.0000  11.0000  11.0000  12.6667  13.3333  15.6667  16.3333
rolling_mean(x, 3, 3)      nan      nan  11.0000  12.6667  13.3333  15.6667  16.3333
```

### Example 2

A `NaN` inside a window makes every window that contains it `NaN`.

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
  const double nan = std::nan("");
  const Series x = pluss::make_series({10.0, nan, 11.0, 12.0, 13.0}, f64);
  show("x", x);
  show("rolling_mean(x, 2)", ts::rolling_mean(x, 2));
}
```

Output

```text
x               10.0000      nan  11.0000  12.0000  13.0000
rolling_mean(x, 2)  10.0000      nan      nan  11.5000  12.5000
```


## See Also

- [`expanding_mean`](expanding_mean.md): the same with a window that grows
- [`zscore_rolling`](zscore_rolling.md)
