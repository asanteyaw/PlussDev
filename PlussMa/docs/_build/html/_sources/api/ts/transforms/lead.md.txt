<!-- generated from lead.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::lead`

The series shifted earlier in time: position `t` holds the observation from `periods` steps later.

## Signature

```cpp
pluss::Series ts::lead(const pluss::Series& x, std::int64_t periods = 1);
```

## What It Does

`lead` places `x[t + periods]` at position `t`, so the series appears advanced. It pairs an observation with its future value, as when a target is the next period's value. The last `periods` positions have no later observation and are `NaN`.

## Parameters

`const pluss::Series& x`
: A one-dimensional series in time order. A series with more than one dimension is rejected.

`std::int64_t periods`
: The number of steps to advance (default 1).

## Returns

A `Series` of the same length as `x`.

## Errors

- A series with more than one dimension is rejected.

## Practical Examples

### Example 1

The series advanced by one and two steps.

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
  show("lead(x)", ts::lead(x));
  show("lead(x, 2)", ts::lead(x, 2));
}
```

Output

```text
x               10.0000  12.0000  11.0000  15.0000  14.0000  18.0000  17.0000
lead(x)         12.0000  11.0000  15.0000  14.0000  18.0000  17.0000      nan
lead(x, 2)      11.0000  15.0000  14.0000  18.0000  17.0000      nan      nan
```

### Example 2

`lead` and `lag` are inverses where both are defined.

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
  show("lag(lead(x))", ts::lag(ts::lead(x)));
}
```

Output

```text
x               10.0000  12.0000  11.0000  15.0000  14.0000  18.0000  17.0000
lag(lead(x))        nan  12.0000  11.0000  15.0000  14.0000  18.0000  17.0000
```


## See Also

- [`lag`](lag.md)
