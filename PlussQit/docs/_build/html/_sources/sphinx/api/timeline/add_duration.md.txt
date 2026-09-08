# Timeline::add_duration

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::add_duration(const Duration& duration) const
```

## What It Does

Shifts every defined timestamp forward by the supplied duration.

This is the primary "move these timestamps later" surface on `Timeline`.

It works on:

- a scalar moment
- a 1D temporal vector
- a higher-rank timeline, when the duration operand is broadcast-compatible

The result keeps the temporal shape of the timeline and only changes the
instants.

## Parameters

**d**

The duration to add.

Common construction patterns include:

- `pluss::duration::seconds(...)`
- `pluss::duration::minutes(...)`
- `pluss::duration::hours(...)`
- `pluss::duration::days(...)`

In practical use, the main toggle is not a boolean argument but the duration
you construct:

- minutes for intraday shifts
- hours for session or timezone-style offsets
- days for an exact multiple of 24 elapsed hours

## Returns

`Timeline`
    A shifted timeline with the same shape and timezone label as the input.

## Examples

Example 1. Add 90 minutes to one scalar timestamp.

```cpp
auto out = scalar.add_duration(pluss::duration::minutes(90));
std::cout << pluss::timeline::print(out) << "\n";
```

### Output

```text
Timeline(shape=[], tz=UTC)
2024-11-01 10:30:00
```

Example 2. Add one hour to every timestamp in a 1D timeline.

```cpp
auto out = base.add_duration(pluss::duration::hours(1));
std::cout << pluss::timeline::print(out) << "\n";
```

### Output

```text
Timeline(shape=[4], tz=UTC)
[ 2024-11-01 10:00:00, 2024-11-01 11:15:30, 2025-01-01 00:59:59, 2025-01-01 01:00:00 ]
```

## Full Scope: Every Unit Factory, Same Instant

Every [`pluss::duration::*` unit factory](../duration/construction.md) works
here — they all just scale a fixed nanosecond offset, so the choice of
factory is purely about how you want to express the shift, not a different
code path. Starting from `1999-04-17T21:15:23Z`:

```cpp
const auto t0 = pluss::timeline::make("1999-04-17T21:15:23Z");
std::cout << pluss::timeline::print(t0 + pluss::duration::nanos(1)) << "\n";
std::cout << pluss::timeline::print(t0 + pluss::duration::microseconds(1)) << "\n";
std::cout << pluss::timeline::print(t0 + pluss::duration::milliseconds(1)) << "\n";
std::cout << pluss::timeline::print(t0 + pluss::duration::seconds(1)) << "\n";
std::cout << pluss::timeline::print(t0 + pluss::duration::minutes(1)) << "\n";
std::cout << pluss::timeline::print(t0 + pluss::duration::hours(1)) << "\n";
std::cout << pluss::timeline::print(t0 + pluss::duration::days(1)) << "\n";
std::cout << pluss::timeline::print(t0 + pluss::duration::weeks(1)) << "\n";
```

### Output

```text
plus_1ns  1999-04-17 21:15:23
plus_1us  1999-04-17 21:15:23
plus_1ms  1999-04-17 21:15:23
plus_1s   1999-04-17 21:15:24
plus_1min 1999-04-17 21:16:23
plus_1h   1999-04-17 22:15:23
plus_1d   1999-04-18 21:15:23
plus_1w   1999-04-24 21:15:23
```

`pluss::timeline::print` renders down to whole seconds, so the `1ns`/`1us`/`1ms`
shifts are real (the nanosecond count in `total_nanoseconds()` genuinely
changes) but do not show up in this text rendering — inspect
`total_nanoseconds()`/`total_microseconds()` directly if you need to see
sub-second precision.

The motivating case — adding 3 hours to `21:15:23` — rolls the instant into
the next calendar day:

```cpp
std::cout << pluss::timeline::print(t0 + pluss::duration::hours(3)) << "\n";
```

### Output

```text
Timeline(shape=[], tz=UTC)
1999-04-18 00:15:23
```

This is plain fixed-offset nanosecond arithmetic on the UTC epoch axis; it
has no concept of calendar irregularities. If you instead want "3 calendar
days in a given timezone, respecting offset transitions," or "add 1 calendar
month," use [`Timeline::add_days`](add_days.md),
[`Timeline::add_months`](add_months.md), or
[`Timeline::add_years`](add_years.md). Those methods operate on local calendar
fields rather than adding a fixed `Duration`.

## Operator Sugar Is the Named Method, Not a Separate Path

`t + d` and `t.add_duration(d)` compute identically — verified by comparing
them elementwise:

```cpp
const auto via_operator = t0 + pluss::duration::hours(3);
const auto via_method = t0.add_duration(pluss::duration::hours(3));
std::cout << pluss::series::print(via_operator.eq(via_method)) << "\n";
```

### Output

```text
Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=true
```

Addition is commutative at the public surface: both `timeline + duration` and
`duration + timeline` return the same Timeline. The reverse form is useful
when a duration expression naturally comes first; it is not a different
kernel or a different broadcasting rule.

`t += d` and `t -= d` also exist, as compound-assignment sugar around the
same two binary operators (`*this = *this + d; return *this;`), and are
verified identical to them:

```cpp
auto t_mut = t0;
t_mut += pluss::duration::hours(3);
std::cout << pluss::timeline::print(t_mut) << "\n";
std::cout << pluss::series::print(t_mut.eq(t0 + pluss::duration::hours(3))) << "\n";
```

### Output

```text
Timeline(shape=[], tz=UTC)
1999-04-18 00:15:23

Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=true
```

## Broadcasting

`add_duration`/`operator+` use standard trailing-axis broadcasting. A scalar
operand therefore pairs with every element of an N-D operand on the other
side.

Example: a scalar `Timeline` plus a 1D `Duration` broadcasts to a 1D result.

```cpp
const auto steps = pluss::duration::hours({1, 2, 3, 4});
std::cout << pluss::timeline::print(t0 + steps) << "\n";
```

### Output

```text
Timeline(shape=[4], tz=UTC)
[ 1999-04-17 22:15:23, 1999-04-17 23:15:23, 1999-04-18 00:15:23, 1999-04-18 01:15:23 ]
```

The reverse shape combination also broadcasts: a 1D `Timeline` plus a scalar
`Duration`.

```cpp
const auto t_vec = pluss::timeline::make({
    "2024-11-01T09:00:00Z", "2024-11-01T10:00:00Z",
    "2024-11-01T11:00:00Z", "2024-11-01T12:00:00Z",
});
std::cout << pluss::timeline::print(t_vec + pluss::duration::minutes(30)) << "\n";
```

### Output

```text
Timeline(shape=[4], tz=UTC)
[ 2024-11-01 09:30:00, 2024-11-01 10:30:00, 2024-11-01 11:30:00, 2024-11-01 12:30:00 ]
```

And matching shapes combine elementwise, not by broadcast:

```cpp
std::cout << pluss::timeline::print(t_vec + steps) << "\n";
```

### Output

```text
Timeline(shape=[4], tz=UTC)
[ 2024-11-01 10:00:00, 2024-11-01 12:00:00, 2024-11-01 14:00:00, 2024-11-01 16:00:00 ]
```

## Absent Instants

`NaT` propagates from either side of the addition: an absent instant or an
absent (`Duration::kNaT`) duration both produce `NaT`:

```cpp
const auto t_absent = pluss::timeline::make(""); // documented absent-instant text
std::cout << pluss::timeline::print(t_absent + pluss::duration::hours(1)) << "\n";

const auto d_absent = pluss::duration::nanos(pluss::Duration::kNaT);
std::cout << pluss::timeline::print(t0 + d_absent) << "\n";
```

### Output

```text
Timeline(shape=[], tz=UTC)
NaT

Timeline(shape=[], tz=UTC)
NaT
```

## Timezone Handling

`add_duration`/`operator+` always keep the `tz` label of the left-hand
`Timeline` operand (`Duration` itself carries no timezone). The arithmetic
still happens on the underlying UTC instant, so the printed wall-clock time
reflects that timezone's offset at each instant (including DST, when it
applies):

```cpp
const auto t0_ny = t0.tz_convert("America/New_York");
std::cout << pluss::timeline::print(t0_ny) << "\n";
std::cout << pluss::timeline::print(t0_ny + pluss::duration::hours(3)) << "\n";
```

### Output

```text
Timeline(shape=[], tz=America/New_York)
1999-04-17 17:15:23

Timeline(shape=[], tz=America/New_York)
1999-04-17 20:15:23
```

`21:15:23 UTC` is `17:15:23` in `America/New_York` under April 1999 DST
(UTC-4); adding a fixed 3 hours lands on `20:15:23` in that same zone — the
shift is still exactly 3 hours of elapsed time, the wall-clock display just
reflects the timezone label.

## Notes

- Absent instants remain absent.
- The timeline shape is preserved (subject to broadcasting, see above).
- Crossing a date boundary is handled naturally by the underlying timestamp
  arithmetic, as shown by the verified `2024-12-31` to `2025-01-01` rollover in
  Example 2 above and the `21:15:23` to next-day `00:15:23` rollover above.
- A shift outside the representable nanosecond epoch range throws; it never
  wraps into another date or into `NaT`.
- Operator sugar also exists: `t + d`, and it is identical to `add_duration`,
  not an approximation of it. `t += d` is the compound-assignment form of the
  same thing.

## See Also

- [`Timeline::sub_duration`](sub_duration.md)
- [`Timeline::diff`](diff.md)
- [`Timeline::add_nanos`](add_nanos.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="operators.html">
    <span class="label">Previous</span>
    <span class="title">operators</span>
  </a>
  <a class="next-link" href="sub_duration.html">
    <span class="label">Next</span>
    <span class="title">sub_duration</span>
  </a>
</div>
