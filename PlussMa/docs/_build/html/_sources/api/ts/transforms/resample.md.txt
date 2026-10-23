<!-- generated from resample.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::resample`

Aggregates a time-indexed series into coarser time bins, such as days, weeks or months.

## Signature

```cpp
ts::ResampleResult ts::resample(const pluss::Timeline& timeline, const pluss::Series& values, std::string_view freq, ts::ResampleOp op = ts::ResampleOp::Mean, std::int64_t step = 1);
```

## What It Does

`resample` groups the observations by the calendar bin of their timestamp and reduces each group with one of the operations of `ResampleOp`: the mean, sum, minimum, maximum, first or last value, or the count. It returns the aggregated values and a `Timeline` of the bin labels (the start of each bin), so the result is again an indexed series. Every bin between the first and the last observation appears in the result: a bin that holds no observation is `NaN` for the aggregations and 0 for the count.

`freq` is a frequency code such as `"D"` (day); `step` widens the bin to `step` such units.

## Parameters

`const pluss::Timeline& timeline`
: The timestamps of the observations, one per value.

`const pluss::Series& values`
: The observations (one-dimensional; real-valued), the same length as `timeline`.

`std::string_view freq`
: The bin unit, for example `"D"` for days.

`ts::ResampleOp op`
: `Mean` (default), `Sum`, `Min`, `Max`, `First`, `Last` or `Count`.

`std::int64_t step`
: The bin width in units of `freq` (default 1; greater than 0).

## Returns

A `ResampleResult` with `values` (one entry per bin) and `labels` (a `Timeline` of the bin starts).

## Errors

- `values` must be one-dimensional and as long as the timeline.
- `freq` must not be empty and `step` must be positive.

## Practical Examples

### Example 1

Five readings over three days reduced to one value per day, with each aggregation.

```cpp
#include <pluss/pluss.h>
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
  const auto timeline = pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-01-01T12:00:00Z", "2024-01-02T00:00:00Z", "2024-01-02T12:00:00Z", "2024-01-03T06:00:00Z"});
  const Series values = pluss::make_series({1.0, 3.0, 2.0, 4.0, 10.0}, f64);
  const char* names[] = {"mean", "sum", "min", "max", "first", "last", "count"};
  int i = 0;
  for (auto op : {ts::ResampleOp::Mean, ts::ResampleOp::Sum, ts::ResampleOp::Min, ts::ResampleOp::Max, ts::ResampleOp::First, ts::ResampleOp::Last, ts::ResampleOp::Count})
    show(names[i++], ts::resample(timeline, values, "D", op).values);
}
```

Output

```text
mean             2.0000   3.0000  10.0000
sum              4.0000   6.0000  10.0000
min              1.0000   2.0000  10.0000
max              3.0000   4.0000  10.0000
first            1.0000   2.0000  10.0000
last             3.0000   4.0000  10.0000
count            2.0000   2.0000   1.0000
```

### Example 2

The bin labels are the start of each day; `step = 2` makes two-day bins.

```cpp
#include <pluss/pluss.h>
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
  const auto timeline = pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-01-01T12:00:00Z", "2024-01-02T00:00:00Z", "2024-01-02T12:00:00Z", "2024-01-03T06:00:00Z"});
  const Series values = pluss::make_series({1.0, 3.0, 2.0, 4.0, 10.0}, f64);
  const auto daily = ts::resample(timeline, values, "D");
  std::cout << *daily.labels << "\n";
  const auto two_day = ts::resample(timeline, values, "D", ts::ResampleOp::Sum, 2);
  show("two-day sum", two_day.values);
  std::cout << *two_day.labels << "\n";
}
```

Output

```text
Timeline(shape=[3], tz=UTC)
[ 2024-01-01 00:00:00, 2024-01-02 00:00:00, 2024-01-03 00:00:00 ]
two-day sum     10.0000  10.0000
Timeline(shape=[2], tz=UTC)
[ 2024-01-01 00:00:00, 2024-01-03 00:00:00 ]
```

### Example 3

An empty day between two observations stays in the result as `NaN` (and as 0 for the count).

```cpp
#include <pluss/pluss.h>
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
  const auto timeline = pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-01-04T12:00:00Z"});
  const Series values = pluss::make_series({1.0, 3.0}, f64);
  show("mean", ts::resample(timeline, values, "D").values);
  show("count", ts::resample(timeline, values, "D", ts::ResampleOp::Count).values);
}
```

Output

```text
mean             1.0000      nan      nan   3.0000
count            1.0000   0.0000   0.0000   1.0000
```


## See Also

- [`fill_gaps`](fill_gaps.md)
- [`rolling_mean`](rolling_mean.md)
