<!-- generated from calendars_and_durations.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Calendars, durations and time zones

[Back to Timeline and Duration by example](../timeline.md)

Dates drive money and rosters: when is an invoice due, who paid late, when does a shift end in another city, and what happens on the night the clocks change. This page answers those questions with `Timeline` for instants and `Duration` for elapsed time, in complete programs that print their results.

## The mental model

Two types, kept apart on purpose.

- A **`Timeline`** is a block of instants. It knows calendar fields (`year`, `weekday`, `is_business_day`), calendar moves (`add_days`, `add_months`) and a time zone.
- A **`Duration`** is a block of elapsed lengths: thirty days, eight hours. It knows nothing about calendars, so `duration::days(30)` is exactly `30 x 24` hours however the calendar falls.

They combine with ordinary operators: `timeline + duration` is a `Timeline`, `timeline - timeline` is a `Duration`, and comparing durations gives a Boolean `Series`. The distinction matters whenever a civil day and an elapsed day differ, which the last program demonstrates. Results cross into text through `strftime` (a `Note`) and into numbers through `host()`.

## A payment schedule

Four invoices with payment terms of thirty days. The program computes due dates, moves a due date that falls on a weekend to the next Monday, compares payments with due dates, and ages the unpaid.

```cpp
#include <pluss/pluss.h>

#include <algorithm>
#include <iostream>

static void show(const char *label, const pluss::Timeline &t) { std::cout << label << ": " << pluss::timeline::print(t) << "\n"; }
static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }
static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  using pluss::ScalarType;
  auto issued = pluss::Timeline::from_ymd(pluss::make_series({2026, 2026, 2026, 2026}), pluss::make_series({9, 10, 10, 11}), pluss::make_series({15, 22, 23, 2}));
  show("issued", issued.strftime("%a %Y-%m-%d"));

  // 1. Payment terms are an elapsed length, a Duration: thirty days.
  auto terms = pluss::duration::days(30);
  auto due = issued + terms;
  show("due", due.strftime("%a %Y-%m-%d"));

  // 2. A due date on a Saturday or Sunday moves to the following Monday. weekday() numbers Monday as 0.
  auto weekday = due.weekday().to(ScalarType::Int64);
  auto push = weekday.eq(5).to(ScalarType::Int64).mul(2).add(weekday.eq(6).to(ScalarType::Int64));
  show("days to push", push);
  auto adjusted = due + pluss::duration::days(push.items<std::int64_t>());
  show("adjusted due", adjusted.strftime("%a %Y-%m-%d"));
  show("on a business day?", adjusted.is_business_day());

  // 3. Payments arrive on these dates. Subtracting two Timelines gives a Duration; comparing it with zero says who was late.
  auto paid = pluss::Timeline::from_ymd(pluss::make_series({2026, 2026, 2026, 2026}), pluss::make_series({10, 11, 11, 11}), pluss::make_series({14, 24, 23, 20}));
  auto delay = paid - adjusted;
  auto late = delay.gt(pluss::duration::days(0));
  show("late?", late);
  const auto nanos = delay.host();   // a Duration crosses to the host as signed nanoseconds
  std::cout << "days late (negative = early):";
  for (const auto ns : nanos) std::cout << " " << static_cast<double>(ns) / 86400e9;
  std::cout << "\n";

  // 4. Ageing: an invoice unpaid at the reporting date is overdue if the reporting date is later than its due date.
  auto reporting = pluss::Timeline::from_ymd(pluss::make_series(2026), pluss::make_series(11), pluss::make_series(25));
  auto age = reporting - adjusted;
  show("overdue at 2026-11-25?", age.gt(pluss::duration::days(0)));
  const auto waits = age.host();
  std::cout << "longest wait since the due date: " << static_cast<double>(*std::max_element(waits.begin(), waits.end())) / 86400e9 << " days\n";
}
```

Output

```text
issued: Note(shape=[4], dim=1, numel=4, encoding=utf8)
["Tue 2026-09-15", "Thu 2026-10-22", "Fri 2026-10-23", "Mon 2026-11-02"]
due: Note(shape=[4], dim=1, numel=4, encoding=utf8)
["Thu 2026-10-15", "Sat 2026-11-21", "Sun 2026-11-22", "Wed 2026-12-02"]
days to push: Series(shape=[4], strides=[1], dtype=int64, platform=cpu, numel=4, contiguous=true)
data=[0, 2, 1, 0]
adjusted due: Note(shape=[4], dim=1, numel=4, encoding=utf8)
["Thu 2026-10-15", "Mon 2026-11-23", "Mon 2026-11-23", "Wed 2026-12-02"]
on a business day?: Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, true, true, true]
late?: Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[false, true, false, false]
days late (negative = early): -1 1 0 -12
overdue at 2026-11-25?: Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, true, true, false]
longest wait since the due date: 41 days
```
The terms are a `Duration`, and `issued + terms` gives due dates directly: `Thu 15 Oct` for the first. The second and third invoices fall due on a Saturday and a Sunday, so the push is `[0, 2, 1, 0]` days and both move to Monday 23 November. The push is itself a `Duration` built from a list of whole days. Subtracting the adjusted due dates from the payment dates gives a `Duration` per invoice; compared with `duration::days(0)` it flags the one late payment, and the host copy, in nanoseconds, converts to days late: `-1`, `1`, `0` and `-12` (negative means early). The last block repeats the idea as of 25 November: three invoices are overdue, and the longest wait since a due date is 41 days.

## Seeing the same moment elsewhere

A shift starts at 22:00 London time on Friday and another at 06:00 on Saturday. The program shows the starts in New York and Edmonton, adds an eight-hour `Duration`, and then crosses the night the clocks go back.

```cpp
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }

int main() {
  // 1. Wall-clock times are meaningless until you say where they were read. Attach a zone with tz_localize; the result is a real instant.
  auto london_start = pluss::Timeline::from_ymdhms(pluss::make_series({2026, 2026}), pluss::make_series({11, 11}), pluss::make_series({13, 14}),
                                                  pluss::make_series({22, 6}), pluss::make_series({0, 0}), pluss::make_series({0, 0}))
                          .tz_localize("Europe/London");
  show("start, London", london_start.strftime("%a %d %b %H:%M %Z"));

  // 2. The same instants in two other cities (the zones this build knows are UTC, Europe/London, America/New_York and America/Edmonton): tz_convert changes the view, never the moment.
  show("start, New York", london_start.tz_convert("America/New_York").strftime("%a %d %b %H:%M %Z"));
  show("start, Edmonton", london_start.tz_convert("America/Edmonton").strftime("%a %d %b %H:%M %Z"));

  // 3. A shift is eight hours: add a Duration, and measure what is left with a subtraction that yields a Duration.
  auto shift_length = pluss::duration::hours(8);
  auto end = london_start + shift_length;
  show("end, London", end.strftime("%a %d %b %H:%M %Z"));
  const auto worked = (end - london_start).host();
  std::cout << "shift length in hours: " << static_cast<double>(worked[0]) / 3600e9 << "\n";

  // 4. The night the clocks go back (London, 2026-10-25). A civil day and an elapsed day differ: add_days keeps the wall clock,
  //    adding 24 hours keeps the elapsed time.
  auto saturday = pluss::Timeline::from_ymdhms(pluss::make_series(2026), pluss::make_series(10), pluss::make_series(24), pluss::make_series(12),
                                              pluss::make_series(0), pluss::make_series(0))
                      .tz_localize("Europe/London");
  show("Saturday noon", saturday.strftime("%a %d %b %H:%M %Z"));
  show("add_days(1)", saturday.add_days(1).strftime("%a %d %b %H:%M %Z"));
  show("+ 24 hours ", (saturday + pluss::duration::hours(24)).strftime("%a %d %b %H:%M %Z"));
  const auto elapsed = (saturday.add_days(1) - saturday).host();
  std::cout << "hours between the two noons: " << static_cast<double>(elapsed[0]) / 3600e9 << "\n";
}
```

Output

```text
start, London: Note(shape=[2], dim=1, numel=2, encoding=utf8)
["Fri 13 Nov 22:00 Europe/London", "Sat 14 Nov 06:00 Europe/London"]
start, New York: Note(shape=[2], dim=1, numel=2, encoding=utf8)
["Fri 13 Nov 17:00 America/New_York", "Sat 14 Nov 01:00 America/New_York"]
start, Edmonton: Note(shape=[2], dim=1, numel=2, encoding=utf8)
["Fri 13 Nov 15:00 America/Edmonton", "Fri 13 Nov 23:00 America/Edmonton"]
end, London: Note(shape=[2], dim=1, numel=2, encoding=utf8)
["Sat 14 Nov 06:00 Europe/London", "Sat 14 Nov 14:00 Europe/London"]
shift length in hours: 8
Saturday noon: Note(shape=[], dim=0, numel=1, encoding=utf8)
"Sat 24 Oct 12:00 Europe/London"
add_days(1): Note(shape=[], dim=0, numel=1, encoding=utf8)
"Sun 25 Oct 12:00 Europe/London"
+ 24 hours : Note(shape=[], dim=0, numel=1, encoding=utf8)
"Sun 25 Oct 11:00 Europe/London"
hours between the two noons: 25
```
`tz_localize("Europe/London")` gives the wall-clock times a zone, which is what turns them into real instants; `tz_convert` then changes only the view. 22:00 on Friday in London is 17:00 in New York and 15:00 in Edmonton, the same instant three ways. Adding `duration::hours(8)` moves the first shift's end to 06:00, and subtracting the two instants returns the eight hours as a `Duration`. The last block is the reason for keeping the two types apart. On Saturday 24 October at noon in London, `add_days(1)` is the civil rule "same wall clock tomorrow" and lands on Sunday 12:00; adding `duration::hours(24)` is the elapsed rule and lands on Sunday 11:00 because the clocks went back that night. The days between the two noons are 25 hours long.

## Decisions

- **Calendar or elapsed:** use `add_days` and `add_months` when the rule is stated in calendar terms ("the same time tomorrow", "the last day of the month"); use `Duration` when it is stated in elapsed terms ("30 days of terms", "an eight-hour shift").
- **Local or instant:** parse and display local times with a zone; compute and compare instants.
- **Business days:** `is_business_day` is the plain Monday-to-Friday rule, with no holidays; push dates yourself, as the program does, when you need a different rule.
- **Zones:** this build knows UTC, Europe/London, America/New_York and America/Edmonton; ask for another and you are told so.

## Pitfalls

A local time that does not exist, and a zone that is not available, are both refused rather than guessed.

```cpp
#include <pluss/pluss.h>

#include <iostream>

int main() {
  auto local = pluss::Timeline::from_ymdhms(pluss::make_series(2026), pluss::make_series(3), pluss::make_series(29), pluss::make_series(1),
                                           pluss::make_series(30), pluss::make_series(0));
  // 1. 01:30 on 2026-03-29 never happens in London: the clocks jump from 01:00 to 02:00. The default is to refuse.
  try {
    std::cout << pluss::note::print(local.tz_localize("Europe/London").strftime("%H:%M")) << "\n";
  } catch (const pluss::Error &e) {
    std::cout << "error: " << e.message() << "\n";
  }
  // 2. Say what to do instead: shift forward past the gap.
  std::cout << "with nonexistent=\"shift_forward\": " << pluss::note::print(local.tz_localize("Europe/London", 0, "shift_forward").strftime("%H:%M")) << "\n";
  // 3. A zone this build does not know is named in the error.
  try {
    (void)local.tz_localize("Asia/Tokyo");
  } catch (const pluss::Error &e) {
    std::cout << "error: " << e.message() << "\n";
  }
}
```

Output

```text
error: tz_localize: nonexistent local time (DST spring-forward gap)
with nonexistent="shift_forward": Note(shape=[], dim=0, numel=1, encoding=utf8)
"02:00"
error: Unknown timezone: Asia/Tokyo (currently supported: UTC, America/Edmonton, America/New_York, Europe/London)
```
01:30 on the morning the clocks go forward never happens in London, so `tz_localize` refuses it; the argument `"shift_forward"` says to move past the gap. If the data may contain such times, decide on the rule rather than discover it in production.

## Where next

Reference pages: [`from_ymd`](../../sphinx/api/timeline/from_ymd.md), [`from_ymdhms`](../../sphinx/api/timeline/from_ymdhms.md), [`add_days`](../../sphinx/api/timeline/add_days.md), [`weekday`](../../sphinx/api/timeline/weekday.md), [`weekday_name`](../../sphinx/api/timeline/weekday_name.md), [`is_business_day`](../../sphinx/api/timeline/is_business_day.md), [`strftime`](../../sphinx/api/timeline/strftime.md), [`tz_localize`](../../sphinx/api/timeline/tz_localize.md), [`tz_convert`](../../sphinx/api/timeline/tz_convert.md), [`operators`](../../sphinx/api/timeline/operators.md), [`days`](../../sphinx/api/duration/days.md), [`hours`](../../sphinx/api/duration/hours.md), [`host`](../../sphinx/api/duration/host.md), [`gt`](../../sphinx/api/duration/gt.md). Next: [Event streams on a clock](event_streams.md).
