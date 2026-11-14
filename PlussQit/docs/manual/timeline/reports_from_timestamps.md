<!-- generated from reports_from_timestamps.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Reports from timestamps

[Back to Timeline and Duration by example](../timeline.md)

Three operational reports built from nothing but instants and the lengths between them: a support desk measured against its promise, subscription renewals with a fair refund, and a service's availability from its heartbeats. Each is one complete program using only `Timeline`, `Duration` and `Series`.

## The mental model

Every report here follows one pattern. Take two `Timeline`s, subtract them to get a `Duration` per row, compare the `Duration` with a limit to get a Boolean `Series`, and convert the `Duration` to numbers (`host()` gives nanoseconds) when you need statistics. Calendar questions (which weekday, which hour, which month end) are asked of the `Timeline` itself, and calendar moves (`add_months`) are kept apart from elapsed lengths (`duration::days`) because they disagree exactly where the money is.

## A support desk against its promise

Eight tickets with opening and closing times. The program computes the time each took, counts those that broke a 24-hour promise, summarises the typical and worst case, and looks at when trouble arrives.

```cpp
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  using pluss::ScalarType;
  // Eight tickets: when each was opened and when it was closed (UTC).
  auto opened = pluss::Timeline::from_ymdhms(pluss::make_series({2026, 2026, 2026, 2026, 2026, 2026, 2026, 2026}), pluss::make_series({11, 11, 11, 11, 11, 11, 11, 11}),
                                             pluss::make_series({2, 2, 3, 4, 5, 6, 9, 9}), pluss::make_series({9, 14, 10, 16, 9, 22, 8, 11}),
                                             pluss::make_series({15, 40, 5, 30, 0, 10, 45, 20}), pluss::make_series({0, 0, 0, 0, 0, 0, 0, 0}));
  auto closed = pluss::Timeline::from_ymdhms(pluss::make_series({2026, 2026, 2026, 2026, 2026, 2026, 2026, 2026}), pluss::make_series({11, 11, 11, 11, 11, 11, 11, 11}),
                                             pluss::make_series({2, 3, 3, 6, 5, 9, 9, 10}), pluss::make_series({11, 9, 15, 11, 12, 9, 10, 15}),
                                             pluss::make_series({0, 30, 20, 0, 30, 0, 15, 0}), pluss::make_series({0, 0, 0, 0, 0, 0, 0, 0}));

  // 1. Resolution time is closed minus opened: a Duration per ticket. Convert it to hours for reporting.
  auto took = closed - opened;
  const auto nanos = took.host();
  std::vector<double> hours;
  for (const auto ns : nanos) hours.push_back(static_cast<double>(ns) / 3600e9);
  auto resolution = pluss::make_series(hours);
  show("hours to resolve", resolution);

  // 2. The promise is 24 hours: compare the Duration with the limit.
  auto breached = took.gt(pluss::duration::hours(24));
  show("breached the 24 h promise", breached);
  std::cout << "breaches: " << breached.sum().item<double>() << " of " << breached.numel() << "\n";

  // 3. Typical and worst case. The median resists the one very slow ticket; the 90th percentile shows what a bad day looks like.
  std::cout << "mean " << resolution.mean().item<double>() << " h, median " << resolution.median().item<double>() << " h, 90th percentile "
            << resolution.quantile(0.9).item<double>() << " h, worst " << resolution.max().item<double>() << " h\n";

  // 4. When does trouble arrive? Calendar fields of the open time: the weekday (Monday = 0) and the hour of day.
  show("opened on weekday", opened.weekday());
  show("opened at hour", opened.hour());
  auto weekday = opened.weekday().to(ScalarType::Int64);
  show("mean hours to resolve by weekday (Mon..Sun)", resolution.grouped_mean(weekday, 7));
  show("tickets opened outside 09:00-17:00", opened.hour().lt(9).logical_or(opened.hour().ge(17)));
}
```

Output

```text
hours to resolve: Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[1.7500, 18.8333, 5.2500, 42.5000, 3.5000, 58.8333, 1.5000, 27.6667]
breached the 24 h promise: Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[false, false, false, true, false, true, false, true]
breaches: 3 of 8
mean 19.9792 h, median 5.25 h, 90th percentile 47.4 h, worst 58.8333 h
opened on weekday: Series(shape=[8], strides=[1], dtype=int32, platform=cpu, numel=8, contiguous=true)
data=[0, 0, 1, 2, 3, 4, 0, 0]
opened at hour: Series(shape=[8], strides=[1], dtype=int32, platform=cpu, numel=8, contiguous=true)
data=[9, 14, 10, 16, 9, 22, 8, 11]
mean hours to resolve by weekday (Mon..Sun): Series(shape=[7], strides=[1], dtype=float64, platform=cpu, numel=7, contiguous=true)
data=[12.4375, 5.2500, 42.5000, 3.5000, 58.8333, nan, nan]
tickets opened outside 09:00-17:00: Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[false, false, false, false, false, true, true, false]
```
`closed - opened` is a `Duration` per ticket, and comparing it with `duration::hours(24)` flags three breaches out of eight. In hours the tickets took `1.75` to `58.8`. The mean is `20.0` hours but the median is `5.25`: a few slow tickets pull the mean far from what a typical customer sees, which is why a service-level report quotes a median and a high percentile (the 90th is `47.4`), not a mean alone. Calendar fields answer the other questions: weekday 0 is Monday, the mean time by weekday shows Wednesday and Friday tickets are the slow ones, and the last line flags the two tickets opened at 22:00 and 08:00, outside the 09:00 to 17:00 desk hours. Weekdays with no tickets, Saturday and Sunday here, come back as `nan`, not zero.

## Subscription renewals and refunds

Customers who started on the 31st, the 30th and the 15th of a month, and a second group who started in September. The program renews them monthly, shows why a calendar rule beats adding thirty days, and prorates a refund.

```cpp
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }
static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  // Three customers who started on the 31st, the 30th and the 15th of different months.
  auto started = pluss::Timeline::from_ymd(pluss::make_series({2026, 2026, 2026}), pluss::make_series({1, 3, 8}), pluss::make_series({31, 30, 15}));
  show("started", started.strftime("%a %d %b %Y"));

  // 1. A monthly plan renews on the same day of the month. add_months is a calendar rule: when the month is too short the date
  //    clamps to the month's last day, instead of spilling into the next month as "add 30 days" would.
  show("first renewal ", started.add_months(1).strftime("%a %d %b %Y"));
  show("second renewal", started.add_months(2).strftime("%a %d %b %Y"));
  show("30 days later  ", (started + pluss::duration::days(30)).strftime("%a %d %b %Y"));

  // 2. A second group of customers who started in September on the 30th, the 26th and the 15th. As of 2026-12-10 each is in the
  //    period that began at the renewal two months after the start and ends at the renewal three months after it.
  auto subscribed = pluss::Timeline::from_ymd(pluss::make_series({2026, 2026, 2026}), pluss::make_series({9, 9, 9}), pluss::make_series({30, 26, 15}));
  auto today = pluss::Timeline::from_ymd(pluss::make_series({2026, 2026, 2026}), pluss::make_series({12, 12, 12}), pluss::make_series({10, 10, 10}));
  auto last_renewal = subscribed.add_months(2);
  auto next_renewal = subscribed.add_months(3);
  show("period began", last_renewal.strftime("%a %d %b %Y"));
  show("next renewal", next_renewal.strftime("%a %d %b %Y"));
  auto left = next_renewal - today;                    // a Duration per customer
  const auto nanos = left.host();
  std::cout << "days left in the period:";
  for (const auto ns : nanos) std::cout << " " << static_cast<double>(ns) / 86400e9;
  std::cout << "\n";

  // 3. Prorated refund for cancelling today: the unused share of the period price. The period runs from the last renewal to the next.
  auto period = next_renewal - last_renewal;
  const auto period_nanos = period.host();
  const double price = 30.0;
  std::cout << "refund for cancelling today:";
  for (std::size_t i = 0; i < nanos.size(); ++i) std::cout << " " << price * static_cast<double>(nanos[i]) / static_cast<double>(period_nanos[i]);
  std::cout << "\n";

  // 4. Calendar flags for operations: which renewals land on a month end, and which charges fall on a weekend.
  show("second renewal of the first group on a month end?", started.add_months(2).is_month_end());
  show("next renewal of the second group on a weekend?", next_renewal.is_weekend());
}
```

Output

```text
started: Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Sat 31 Jan 2026", "Mon 30 Mar 2026", "Sat 15 Aug 2026"]
first renewal : Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Sat 28 Feb 2026", "Thu 30 Apr 2026", "Tue 15 Sep 2026"]
second renewal: Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Tue 31 Mar 2026", "Sat 30 May 2026", "Thu 15 Oct 2026"]
30 days later  : Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Mon 02 Mar 2026", "Wed 29 Apr 2026", "Mon 14 Sep 2026"]
period began: Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Mon 30 Nov 2026", "Thu 26 Nov 2026", "Sun 15 Nov 2026"]
next renewal: Note(shape=[3], dim=1, numel=3, encoding=utf8)
["Wed 30 Dec 2026", "Sat 26 Dec 2026", "Tue 15 Dec 2026"]
days left in the period: 20 16 5
refund for cancelling today: 20 16 5
second renewal of the first group on a month end?: Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[true, false, false]
next renewal of the second group on a weekend?: Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[false, true, false]
```
`add_months` is a calendar rule: a plan that started on 31 January renews on `Sat 28 Feb` and then on `Tue 31 Mar`, clamping to the end of a short month without drifting. Adding thirty days gives `Mon 02 Mar` for the same customer and would keep the customer at the wrong day of the month forever. The second group shows ageing and proration: as of 10 December the three customers have `20`, `16` and `5` days left in their paid period, each period is exactly 30 days long, and the unused share of a price of `30` is therefore `20`, `16` and `5`. The same lines work when the periods differ in length, because the proration divides one `Duration` by another instead of assuming a month. The last block flags the renewals an operations team cares about: the first customer's second renewal falls on a month end and the second customer's next charge lands on a Saturday.

## Availability from heartbeats

A service reports in about once a minute. The program finds the gaps between heartbeats, flags those longer than two expected beats as outages, ranks them and computes availability over the window.

```cpp
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  using pluss::ScalarType;
  // Heartbeats as seconds after 08:00:00 UTC on 2026-11-13. Normally one a minute; there are three gaps.
  const std::int64_t base = 1794528000 + 8 * 3600;
  std::vector<double> offsets;
  for (int s = 0; s <= 300; s += 60) offsets.push_back(s);            // 08:00 - 08:05
  for (int s = 600; s <= 780; s += 60) offsets.push_back(s);          // gap of 5 min, then 08:10 - 08:13
  for (int s = 1200; s <= 1500; s += 60) offsets.push_back(s);        // gap of 7 min, then 08:20 - 08:25
  for (int s = 1560; s <= 1680; s += 60) offsets.push_back(s);        // 08:26 - 08:28
  for (int s = 1980; s <= 2100; s += 60) offsets.push_back(s);        // gap of 5 min, then 08:33 - 08:35
  auto beats = pluss::Timeline::from_epoch_seconds(pluss::make_series(offsets).to(ScalarType::Int64).add(base));
  const auto clock = beats.strftime("%H:%M").host();   // keep the owning copy alive while reading it
  std::cout << beats.numel() << " heartbeats from " << clock.vector().front() << " to " << clock.vector().back() << "\n";

  // 1. The gap before each heartbeat is the difference of neighbouring instants: shift the axis by one and subtract.
  const auto n = beats.numel();
  auto later = pluss::Timeline::from_epoch_seconds(pluss::make_series(offsets).slice(0, 1, n).to(ScalarType::Int64).add(base));
  auto earlier = pluss::Timeline::from_epoch_seconds(pluss::make_series(offsets).slice(0, 0, n - 1).to(ScalarType::Int64).add(base));
  auto gaps = later - earlier;                                       // a Duration per pair of neighbouring heartbeats
  const auto gap_ns = gaps.host();
  std::vector<double> gap_minutes;
  for (const auto ns : gap_ns) gap_minutes.push_back(static_cast<double>(ns) / 60e9);
  auto gap = pluss::make_series(gap_minutes);
  show("minutes between heartbeats", gap);

  // 2. An outage is a gap longer than two expected beats: compare the Duration with the limit.
  auto outage = gaps.gt(pluss::duration::minutes(2));
  show("outage?", outage);
  auto outage_minutes = gap.masked_select(outage);
  show("outage lengths (minutes)", outage_minutes);
  std::cout << "outages " << outage_minutes.numel() << ", longest " << outage_minutes.max().item<double>() << " min, total down " << outage_minutes.sum().item<double>() << " min\n";

  // 3. When did the worst outage start? The start is the earlier heartbeat of the pair.
  auto worst = gap.argmax().item<std::int64_t>();
  const auto began = earlier.strftime("%H:%M").host();
  std::cout << "worst outage began at " << began.vector()[static_cast<std::size_t>(worst)] << "\n";

  // 4. Availability over the observed window, taking the minutes beyond one expected beat per outage as down time.
  const double window = (static_cast<double>(offsets.back()) - offsets.front()) / 60.0;
  const double down = outage_minutes.sub(1.0).sum().item<double>();
  std::cout << "window " << window << " min, down " << down << " min, availability " << 100.0 * (1.0 - down / window) << " %\n";
}
```

Output

```text
22 heartbeats from 08:00 to 08:35
minutes between heartbeats: Series(shape=[21], strides=[1], dtype=float64, platform=cpu, numel=21, contiguous=true)
data=[1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 5.0000, 1.0000, 1.0000, 1.0000, 7.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 5.0000, 1.0000, 1.0000]
outage?: Series(shape=[21], strides=[1], dtype=bool, platform=cpu, numel=21, contiguous=true)
data=[false, false, false, false, false, true, false, false, false, true, false, false, false, false, false, false, false, false, true, false, false]
outage lengths (minutes): Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[5.0000, 7.0000, 5.0000]
outages 3, longest 7 min, total down 17 min
worst outage began at 08:13
window 35 min, down 14 min, availability 60 %
```
The gap before each heartbeat is the difference between neighbouring instants, which the program gets by building the `Timeline` twice, shifted by one place, and subtracting: a `Duration` per pair. Most gaps are one minute; three are not, `5`, `7` and `5` minutes. Comparing the `Duration` with `duration::minutes(2)` flags exactly those three, the worst began at 08:13 and lasted seven minutes, and the outages total 17 minutes of silence. The availability counts, for each outage, the minutes beyond the one beat that was expected anyway: 14 minutes down in a 35-minute window is `60 %`. How you define "down" is a policy choice; the code makes it a single line you can change.

## Decisions

- **Duration against a limit:** compare `Duration`s with `duration::hours(...)` or `duration::minutes(...)`; do not convert to numbers to compare.
- **Statistics:** convert to numbers with `host()` only when you need them, and quote medians and percentiles for waiting times.
- **Renewal rules:** `add_months` for "the same day next month", `duration::days` for "thirty days of terms"; they differ at month ends.
- **Gaps:** shift the axis by one and subtract; do not loop.

## Where next

Reference pages: [`from_ymdhms`](../../sphinx/api/timeline/from_ymdhms.md), [`weekday`](../../sphinx/api/timeline/weekday.md), [`hour`](../../sphinx/api/timeline/hour.md), [`add_months`](../../sphinx/api/timeline/add_months.md), [`is_month_end`](../../sphinx/api/timeline/is_month_end.md), [`is_weekend`](../../sphinx/api/timeline/is_weekend.md), [`strftime`](../../sphinx/api/timeline/strftime.md), [`operators`](../../sphinx/api/timeline/operators.md), [`hours`](../../sphinx/api/duration/hours.md), [`minutes`](../../sphinx/api/duration/minutes.md), [`gt`](../../sphinx/api/duration/gt.md), [`host`](../../sphinx/api/duration/host.md). Back to [Timeline and Duration by example](../timeline.md).
