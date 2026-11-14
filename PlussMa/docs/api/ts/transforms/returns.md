<!-- generated from returns.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::returns`

The simple return of a series: the relative change over `periods` observations.

## Signature

```cpp
pluss::Series ts::returns(const pluss::Series& x, std::int64_t periods = 1);
```

## What It Does

`returns` computes `x[t] / x[t − periods] − 1`. For a price series this is the ordinary percentage return (0.20 means a rise of 20%). The first `periods` positions are `NaN`. Simple returns add up across assets at one date and multiply across time; for returns that add across time use [`log_returns`](log_returns.md).

## Parameters

`const pluss::Series& x`
: A one-dimensional series in time order. A series with more than one dimension is rejected.

`std::int64_t periods`
: How many observations back the operation reaches (default 1). The first `periods` positions of the result have nothing to refer to and are `NaN`.

## Returns

A `Series` of the same length as `x`.

## Errors

- A series with more than one dimension is rejected.
- A zero in the denominator position gives an infinite or `NaN` return, as plain division does.

## Practical Examples

### Example 1

One-period and two-period simple returns of a price series.

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
  show("returns(x)", ts::returns(x));
  show("returns(x, 2)", ts::returns(x, 2));
}
```

Output

```text
x               10.0000  12.0000  11.0000  15.0000  14.0000  18.0000  17.0000
returns(x)          nan   0.2000  -0.0833   0.3636  -0.0667   0.2857  -0.0556
returns(x, 2)       nan      nan   0.1000   0.2500   0.2727   0.2000   0.2143
```

### Example 2

A missing price leaves the returns that use it missing; the later returns are computed from the later prices alone.

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
  const Series x = pluss::make_series({100.0, 102.0, nan, 101.0, 105.0}, f64);
  show("x", x);
  show("returns(x)", ts::returns(x));
}
```

Output

```text
x              100.0000 102.0000      nan 101.0000 105.0000
returns(x)          nan   0.0200      nan      nan   0.0396
```


## See Also

- [`log_returns`](log_returns.md)
- [`diff`](diff.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="resample.html">
    <span class="label">Previous</span>
    <span class="title">resample</span>
  </a>
  <a class="next-link" href="rolling_max.html">
    <span class="label">Next</span>
    <span class="title">rolling_max</span>
  </a>
</div>
<!-- apiref:nav:end -->
