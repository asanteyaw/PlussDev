<!-- generated from log_returns.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::log_returns`

The logarithmic return of a series: the natural log of the ratio of an observation to the one `periods` earlier.

## Signature

```cpp
pluss::Series ts::log_returns(const pluss::Series& x, std::int64_t periods = 1);
```

## What It Does

`log_returns` computes `log(x[t] / x[t − periods])`. Log returns add across time (the log return over two periods is the sum of the two one-period log returns), which makes them the usual input of volatility models. For small changes they are close to the simple returns of [`returns`](returns.md). The first `periods` positions are `NaN`. The ratio must be positive: a non-positive observation, or a sign change, gives `NaN`.

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

Logarithmic returns of a price series next to the simple returns they approximate.

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
  show("returns", ts::returns(x));
  show("log_returns", ts::log_returns(x));
}
```

Output

```text
x               10.0000  12.0000  11.0000  15.0000  14.0000  18.0000  17.0000
returns             nan   0.2000  -0.0833   0.3636  -0.0667   0.2857  -0.0556
log_returns         nan   0.1823  -0.0870   0.3102  -0.0690   0.2513  -0.0572
```

### Example 2

Log returns add across time: the two-period log return equals the sum of the two one-period ones.

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
  const Series r1 = ts::log_returns(x);
  show("log_returns(2)", ts::log_returns(x, 2));
  show("r[t]+r[t-1]", r1 + ts::lag(r1));
}
```

Output

```text
x               10.0000  12.0000  11.0000  15.0000  14.0000  18.0000  17.0000
log_returns(2)      nan      nan   0.0953   0.2231   0.2412   0.1823   0.1942
r[t]+r[t-1]         nan      nan   0.0953   0.2231   0.2412   0.1823   0.1942
```

### Example 3

A series that changes sign has no logarithmic return at the change: the result is `NaN`.

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
  const Series x = pluss::make_series({1.0, -1.0, 2.0}, f64);
  show("x", x);
  show("log_returns(x)", ts::log_returns(x));
}
```

Output

```text
x                1.0000  -1.0000   2.0000
log_returns(x)      nan      nan      nan
```


## See Also

- [`returns`](returns.md)
- [`diff`](diff.md)
