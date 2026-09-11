# Timeline::is_month_end

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_month_end() const
```

## What Counts As A Month End

`is_month_end()` marks every instant on the actual final local date of its
month. It computes the month's calendar length, so February 28 in a common
year, February 29 in a leap year, and the appropriate 30th or 31st all work
without a hard-coded day test. Any time on that date qualifies.

The predicate answers calendar membership. It does not mean the greatest
timestamp in an array, the final observed date, or the last working day.

## Result Contract

The return value is a newly allocated, shape-preserving boolean `Series`.
Scalar input remains rank zero, higher-rank input keeps all extents, and `NaT`
maps to `false`. The operation is read-only and interprets dates in the
receiver's current timezone.

## Example 1: Variable Month Lengths

```cpp
const auto calendar = pluss::timeline::make({
    "2019-12-31T12:00:00Z", "2020-01-01T12:00:00Z",
    "2020-02-29T12:00:00Z", "2020-03-31T12:00:00Z",
    "2020-04-01T12:00:00Z", "2020-12-31T12:00:00Z",
    "2021-01-01T12:00:00Z", "2024-02-29T12:00:00Z", "",
});
pluss::series::print(std::cout, calendar.is_month_end());
```

```text
Series(shape=[9], strides=[1], dtype=bool, platform=cpu, numel=9, contiguous=true)
data=[true, false, true, true, false, true, false, true, false]
```

Testing only `day() == 31` would miss both leap-day month ends shown here.

## Example 2: Shape And Boundary Time

```cpp
const auto dates = pluss::timeline::make({
    {"2024-01-01T23:59:59.999999999Z", "2024-01-02T00:00:00Z",
     "2024-02-29T00:00:00Z"},
    {"2024-03-31T12:00:00Z", "2024-04-01T00:00:00Z",
     "2024-12-31T00:00:00Z"},
});
pluss::series::print(std::cout, dates.is_month_end());
```

```text
Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=
⎡ false false true ⎤
⎣  true false true ⎦
```

## Example 3: Rank-Zero Result

```cpp
const auto leap_day =
    pluss::timeline::make("2024-02-29T23:59:59.999999999Z");
pluss::series::print(std::cout, leap_day.is_month_end());
```

```text
Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=true
```

`month_end_scalar()` returns the same classification as a native `bool` and
rejects non-scalar receivers.

## Example 4: One Instant Can Cross The Boundary

```cpp
const auto utc = pluss::timeline::make("2021-01-01T00:30:00Z");
const auto local = utc.tz_convert("America/New_York");
std::cout << std::boolalpha
          << "UTC=" << utc.is_month_end().item<bool>() << '\n'
          << "New York=" << local.is_month_end().item<bool>() << '\n';
```

```text
UTC=false
New York=true
```

## Relationships And Failure Modes

Every quarter end and year end is a month end, but February and the other
non-quarter-ending months show why the reverse implication fails. `NaT=false`
means validity must be tracked separately when needed. The method accepts all
supported ranks and has no options or mutation path.

## See Also

- [`month_end_scalar`](month_end_scalar.md)
- [`is_quarter_end`](is_quarter_end.md)
- [`isvalid`](isvalid.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_monday.html"><span class="label">Previous</span><span class="title">is_monday</span></a>
  <a class="next-link" href="is_month_start.html"><span class="label">Next</span><span class="title">is_month_start</span></a>
</div>
<!-- pluss-hand-authored -->
