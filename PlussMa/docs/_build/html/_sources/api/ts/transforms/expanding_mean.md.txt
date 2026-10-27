<!-- generated from expanding_mean.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::expanding_mean`

The mean of all observations up to each position.

## Signature

```cpp
pluss::Series ts::expanding_mean(const pluss::Series& x, std::int64_t min_periods = 1);
```

## What It Does

`expanding_mean` is a rolling mean whose window starts at the first observation and grows with each step: position `t` holds the mean of `x[0 .. t]`. It is the running average of the series so far. Positions before `min_periods` observations are available are `NaN`.

## Parameters

`const pluss::Series& x`
: A one-dimensional series in time order. A series with more than one dimension is rejected.

`std::int64_t min_periods`
: The number of observations a window must hold before its statistic is reported (default 1); earlier positions are `NaN`. It cannot exceed `window`.

## Returns

A `Series` of the same length as `x`.

## Errors

- `min_periods` must be greater than 0.

## Practical Examples

### Example 1

The running mean, reported from the first observation and then only from the third.

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
  show("min_periods 1", ts::expanding_mean(x));
  show("min_periods 3", ts::expanding_mean(x, 3));
}
```

Output

```text
x               10.0000  12.0000  11.0000  15.0000  14.0000  18.0000  17.0000
min_periods 1   10.0000  11.0000  11.0000  12.0000  12.4000  13.3333  13.8571
min_periods 3       nan      nan  11.0000  12.0000  12.4000  13.3333  13.8571
```

### Example 2

As the history grows the running mean settles; compare it with a short rolling mean.

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
  show("expanding", ts::expanding_mean(x));
  show("rolling 3", ts::rolling_mean(x, 3));
}
```

Output

```text
x               10.0000  12.0000  11.0000  15.0000  14.0000  18.0000  17.0000
expanding       10.0000  11.0000  11.0000  12.0000  12.4000  13.3333  13.8571
rolling 3       10.0000  11.0000  11.0000  12.6667  13.3333  15.6667  16.3333
```


## See Also

- [`expanding_std`](expanding_std.md)
- [`rolling_mean`](rolling_mean.md)
