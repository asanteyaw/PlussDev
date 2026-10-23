<!-- generated from diff.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::diff`

The first difference of a series: each observation minus the one `periods` positions earlier.

## Signature

```cpp
pluss::Series ts::diff(const pluss::Series& x, std::int64_t periods = 1);
```

## What It Does

`diff` returns `x[t] − x[t − periods]` for every position `t`. Differencing removes a trend (`periods = 1` turns a series that drifts into one that fluctuates around the drift) or a seasonal pattern (`periods` equal to the season length). The first `periods` positions have no earlier observation and are `NaN`; the result therefore has the same length as the input, which keeps it aligned for later joins with other series.

Use `diff` on levels. Use [`returns`](returns.md) or [`log_returns`](log_returns.md) on prices, where the change should be relative to the level.

## Parameters

`const pluss::Series& x`
: A one-dimensional series in time order. A series with more than one dimension is rejected.

`std::int64_t periods`
: How many observations back the operation reaches (default 1). The first `periods` positions of the result have nothing to refer to and are `NaN`.

## Returns

A `Series` of the same length as `x`.

## Errors

- A series with more than one dimension is rejected.

## Practical Examples

### Example 1

The first difference with the default `periods = 1` and a lag-2 difference of the same series.

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
  show("diff(x)", ts::diff(x));
  show("diff(x, 2)", ts::diff(x, 2));
}
```

Output

```text
x               10.0000  12.0000  11.0000  15.0000  14.0000  18.0000  17.0000
diff(x)             nan   2.0000  -1.0000   4.0000  -1.0000   4.0000  -1.0000
diff(x, 2)          nan      nan   1.0000   3.0000   3.0000   3.0000   3.0000
```

### Example 2

A missing value makes every difference that uses it missing.

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
  const double nan = std::nan("");
  const Series x = pluss::make_series({100.0, 102.0, nan, 101.0, 105.0}, f64);
  show("x", x);
  show("diff(x)", ts::diff(x));
}
```

Output

```text
x              100.0000 102.0000      nan 101.0000 105.0000
diff(x)             nan   2.0000      nan      nan   4.0000
```

### Example 3

A `periods` as long as the series leaves nothing to compare: every position is `NaN`.

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
  show("diff(x, 7)", ts::diff(x, 7));
}
```

Output

```text
x               10.0000  12.0000  11.0000  15.0000  14.0000  18.0000  17.0000
diff(x, 7)          nan      nan      nan      nan      nan      nan      nan
```


## See Also

- [`returns`](returns.md): the relative change
- [`log_returns`](log_returns.md): the logarithmic change
- [`lag`](lag.md): the shifted series itself
