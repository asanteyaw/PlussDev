<!-- generated from event_streams.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Event streams on a clock

[Back to Timeline and Duration by example](../timeline.md)

Events do not arrive at regular intervals, yet reports are hourly. This page turns raw event timestamps and their values into per-hour totals, a rolling average whose window is a length of time rather than a number of rows, and a lookup of the latest reading at any moment.

## The mental model

A `Timeline` becomes an **axis** with `index()`: the call checks that it is one-dimensional, free of missing values and sorted, and returns an accessor whose operations take a value `Series` of the same length. The accessor does the clock arithmetic: `resample_sum(values, "1h")` assigns each value to its hour and adds, `rolling_mean(values, window)` averages the values inside a window given as a `Duration`, and `asof(moment)` finds the last event at or before a moment. The values stay an ordinary `Series`.

## A traffic report

Twelve events with request counts, spread over nearly three hours from 09:00 UTC. The program prints the event times, totals each hour, smooths over a half-hour window, and finds the latest event before 10:06:40.

```cpp
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }
static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  // Request counts logged at irregular moments, as seconds since the epoch, starting at 2026-11-13 09:00 UTC.
  const std::int64_t start = 1794560400;
  auto stamps = pluss::make_series({0, 600, 1500, 2400, 3500, 3700, 4300, 5400, 7000, 7300, 9000, 10100}).add(start);
  auto count = pluss::make_series({12.0, 18.0, 9.0, 22.0, 15.0, 30.0, 25.0, 11.0, 40.0, 35.0, 8.0, 14.0});
  auto when = pluss::Timeline::from_epoch_seconds(stamps);
  show("first events", when.strftime("%H:%M"));

  // 1. A Timeline used as an axis: index() checks it is one-dimensional, present and sorted and unlocks the ordered operations.
  auto axis = when.index();

  // 2. Total requests per clock hour: resample sums the counts that fall inside each hour-wide bin.
  auto hourly = axis.resample_sum(count, "1h");
  show("requests per hour", hourly);
  std::cout << "grand total " << count.sum().item<double>() << ", sum of the hours " << hourly.sum().item<double>() << "\n";

  // 3. A rolling mean over the previous 30 minutes, measured on the clock: it adapts to the irregular spacing.
  auto smooth = axis.rolling_mean(count, pluss::duration::minutes(30));
  show("30-minute rolling mean", smooth);

  // 4. As-of lookup: the last event at or before a given moment, for an event that happens later than the data starts.
  auto query = pluss::Timeline::from_epoch_seconds(pluss::make_series(start + 4000));
  const auto position = axis.asof(query);
  const auto label = when.strftime("%H:%M:%S").take({position}).host();
  std::cout << "latest event at or before 10:06:40 is number " << position << ", at " << label.vector()[0] << "\n";
}
```

Output

```text
first events: Note(shape=[12], dim=1, numel=12, encoding=utf8)
["09:00", "09:10", "09:25", "09:40", "09:58", "10:01", "10:11", "10:30", "10:56", "11:01", "11:30", "11:48"]
requests per hour: Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[76.0000, 106.0000, 57.0000]
grand total 239, sum of the hours 239
30-minute rolling mean: Series(shape=[12], strides=[1], dtype=float64, platform=cpu, numel=12, contiguous=true)
data=[12.0000, 15.0000, 13.0000, 16.3333, 18.5000, 22.3333, 23.3333, 22.0000, 25.5000, 37.5000, 21.5000, 11.0000]
latest event at or before 10:06:40 is number 5, at 10:01:40
```
The totals `76`, `106` and `57` add up to the grand total of `239`: resampling moves every value into exactly one bin. The rolling mean is not a row-count window. Its value at 10:01 averages the events of the previous half hour (the 09:40 and 09:58 events and itself), so the same code behaves correctly when events bunch up or thin out, which a window of "the last three rows" cannot do. The as-of lookup answers "what was the latest known reading at 10:06:40": event number 5, at 10:01:40.

## Decisions

- **Window by rows or by time:** use the clock whenever events are irregular; a window of rows silently changes meaning when the spacing changes.
- **Bins:** a frequency string such as `"1h"` fixes the bin width; empty hours appear as zero in a sum, so check the number of bins against what you expect.
- **Sortedness:** `index()` insists on a sorted axis; sort the events (and carry the values with them) before you build it.
- **As-of, not equality:** to align two streams, ask for the latest reading at or before each moment instead of requiring the same timestamp.

## Where next

Reference pages: [`from_epoch_seconds`](../../sphinx/api/timeline/from_epoch_seconds.md), [`index`](../../sphinx/api/timeline/index.md), [`strftime`](../../sphinx/api/timeline/strftime.md), [`resample_sum`](../../sphinx/api/timeline-index/resample_sum.md), [`rolling_mean`](../../sphinx/api/timeline-index/rolling_mean.md), [`asof`](../../sphinx/api/timeline-index/asof.md), [`minutes`](../../sphinx/api/duration/minutes.md). Next: [Reports from timestamps](reports_from_timestamps.md).
