<!-- generated from ewm_mean.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::ewm_mean`

The exponentially weighted moving average of a series.

## Signature

```cpp
pluss::Series ts::ewm_mean(const pluss::Series& x, double alpha);
```

## What It Does

`ewm_mean` computes `y[0] = x[0]` and `y[t] = alpha · x[t] + (1 − alpha) · y[t − 1]`: a weighted average of all past observations whose weights fall geometrically with age at rate `1 − alpha`. Unlike a rolling mean it has no window edge and no `NaN` start-up: every position has a value. It is the smoother behind exponential smoothing and the RiskMetrics-style variance estimate.

## Parameters

`const pluss::Series& x`
: A one-dimensional series in time order. A series with more than one dimension is rejected.

`double alpha`
: The weight of the newest observation, in (0, 1]. A larger value forgets the past faster; `alpha = 1` returns the series unchanged. Values outside (0, 1] are rejected.

## Returns

A `Series` of the same length as `x`.

## Errors

- `alpha` outside (0, 1] is rejected.
- A series with more than one dimension is rejected.

## Practical Examples

### Example 1

Smoothing with `alpha = 0.5`: each value is the average of the new observation and the previous smoothed value.

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
  show("ewm_mean(x,.5)", ts::ewm_mean(x, 0.5));
}
```

Output

```text
x               10.0000  12.0000  11.0000  15.0000  14.0000  18.0000  17.0000
ewm_mean(x,.5)  10.0000  11.0000  11.0000  13.0000  13.5000  15.7500  16.3750
```

### Example 2

A small `alpha` follows the series slowly; `alpha = 1` reproduces it.

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
  show("alpha = 0.2", ts::ewm_mean(x, 0.2));
  show("alpha = 1", ts::ewm_mean(x, 1.0));
}
```

Output

```text
x               10.0000  12.0000  11.0000  15.0000  14.0000  18.0000  17.0000
alpha = 0.2     10.0000  10.4000  10.5200  11.4160  11.9328  13.1462  13.9170
alpha = 1       10.0000  12.0000  11.0000  15.0000  14.0000  18.0000  17.0000
```


## See Also

- [`ema`](ema.md): the same function under its common name
- [`ewm_std`](ewm_std.md)

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ema.html">
    <span class="label">Previous</span>
    <span class="title">ema</span>
  </a>
  <a class="next-link" href="ewm_std.html">
    <span class="label">Next</span>
    <span class="title">ewm_std</span>
  </a>
</div>
<!-- apiref:nav:end -->
