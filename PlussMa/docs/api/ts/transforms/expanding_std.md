<!-- generated from expanding_std.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::expanding_std`

The standard deviation of all observations up to each position.

## Signature

```cpp
pluss::Series ts::expanding_std(const pluss::Series& x, std::int64_t min_periods = 1, double epsilon = 1e-12, std::int64_t ddof = 1);
```

## What It Does

`expanding_std` reports, at each position, the standard deviation of `x[0 .. t]`, with divisor `count − ddof` (the sample standard deviation by default). The first position has one observation and is `NaN` for the default `ddof = 1`. Positions before `min_periods` observations are `NaN`.

## Parameters

`const pluss::Series& x`
: A one-dimensional series in time order. A series with more than one dimension is rejected.

`std::int64_t min_periods`
: The number of observations a window must hold before its statistic is reported (default 1); earlier positions are `NaN`. It cannot exceed `window`.

`double epsilon`
: A floor added under the standard deviation before dividing, so a window with no variation gives a finite value instead of an infinite one (default 1e-12).

`std::int64_t ddof`
: Delta degrees of freedom: the divisor is the window size minus `ddof`. The default 1 gives the sample statistic, 0 the population one. A window with no more than `ddof` observations gives `NaN`.

## Returns

A `Series` of the same length as `x`.

## Errors

- `min_periods` must be greater than 0.

## Practical Examples

### Example 1

The running standard deviation, from the third observation.

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

int main() {
  const Series x = pluss::make_series({10.0, 12.0, 11.0, 15.0, 14.0, 18.0, 17.0}, f64);
  show("x", x);
  show("expanding_std", ts::expanding_std(x, 3));
}
```

Output

```text
x               10.0000  12.0000  11.0000  15.0000  14.0000  18.0000  17.0000
expanding_std       nan      nan   1.0000   2.1602   2.0736   2.9439   3.0237
```

### Example 2

The population version divides by the count itself.

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

int main() {
  const Series x = pluss::make_series({10.0, 12.0, 11.0, 15.0, 14.0, 18.0, 17.0}, f64);
  show("x", x);
  show("ddof 1", ts::expanding_std(x, 3, 1e-12, 1));
  show("ddof 0", ts::expanding_std(x, 3, 1e-12, 0));
}
```

Output

```text
x               10.0000  12.0000  11.0000  15.0000  14.0000  18.0000  17.0000
ddof 1              nan      nan   1.0000   2.1602   2.0736   2.9439   3.0237
ddof 0              nan      nan   0.8165   1.8708   1.8547   2.6874   2.7994
```


## See Also

- [`expanding_mean`](expanding_mean.md)
- [`rolling_std`](rolling_std.md)
