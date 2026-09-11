# Timeline::is_leap_year

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_leap_year() const
```

## What The Predicate Asks

`is_leap_year()` classifies the **local civil year** containing each instant.
It returns `true` for every instant in a 366-day Gregorian year, not only for
29 February. This makes it useful before calendar arithmetic whose result may
depend on February's length, or when partitioning observations by calendar
type.

The complete Gregorian rule is used: years divisible by four are leap years,
except century years are common years unless divisible by 400. Consequently,
1900 and 2100 are common years while 2000 is a leap year.

## Result Contract

The result is a newly allocated boolean `Series` with exactly the receiver's
rank and shape. A scalar Timeline produces a rank-zero Series; a matrix
produces a matrix; an empty extent remains empty. `NaT` maps to `false`, so use
`isvalid()` as a second mask when an absent instant must remain distinguishable
from an instant in a common year.

The receiver is not modified. The timezone is read as calendar context: the
same stored instant can belong to different local years after `tz_convert()`.
The operation performs one pass over the values and loads timezone rules once
for the call.

## Example 1: Classify Years, Not Leap Days

```cpp
const auto calendar = pluss::timeline::make({
    "2019-12-31T12:00:00Z", "2020-01-01T12:00:00Z",
    "2020-02-29T12:00:00Z", "2020-03-31T12:00:00Z",
    "2020-04-01T12:00:00Z", "2020-12-31T12:00:00Z",
    "2021-01-01T12:00:00Z", "2024-02-29T12:00:00Z", "",
});
pluss::series::print(std::cout, calendar.is_leap_year());
```

```text
Series(shape=[9], strides=[1], dtype=bool, platform=cpu, numel=9, contiguous=true)
data=[false, true, true, true, true, true, false, true, false]
```

Every 2020 instant is `true`. The final `false` represents `NaT`, not a known
common year.

## Example 2: Century Exceptions

```cpp
const auto centuries = pluss::timeline::make({
    "1900-06-15T12:00:00Z",
    "2000-06-15T12:00:00Z",
    "2100-06-15T12:00:00Z",
});
pluss::series::print(std::cout, centuries.is_leap_year());
```

```text
Series(shape=[3], strides=[1], dtype=bool, platform=cpu, numel=3, contiguous=true)
data=[false, true, false]
```

## Example 3: Scalar Rank Is Preserved

```cpp
const auto leap_day =
    pluss::timeline::make("2024-02-29T23:59:59.999999999Z");
pluss::series::print(std::cout, leap_day.is_leap_year());
```

```text
Series(shape=[], strides=[], dtype=bool, platform=cpu, numel=1, contiguous=true)
data=true
```

Use `leap_scalar()` when the receiver is rank zero and a native `bool`, rather
than a rank-zero `Series`, is the desired result.

## Example 4: Timezone Can Change The Year

```cpp
const auto utc = pluss::timeline::make("2021-01-01T00:30:00Z");
const auto local = utc.tz_convert("America/New_York");
std::cout << std::boolalpha
          << "UTC=" << utc.is_leap_year().item<bool>() << '\n'
          << "New York=" << local.is_leap_year().item<bool>() << '\n';
```

```text
UTC=false
New York=true
```

Both values represent one instant. It lies in 2021 in UTC and in 2020 in New
York.

## Gotchas And Failure Modes

- `false` means either a common local year or `NaT`.
- Time-of-day and month do not affect membership once the local year is known.
- The method accepts every supported Timeline rank; it is not a reduction.
- To identify leap day itself, combine `month() == 2` with `day() == 29`.

## See Also

- [`leap_scalar`](leap_scalar.md)
- [`year`](year.md)
- [`isvalid`](isvalid.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_friday.html"><span class="label">Previous</span><span class="title">is_friday</span></a>
  <a class="next-link" href="is_monday.html"><span class="label">Next</span><span class="title">is_monday</span></a>
</div>
<!-- pluss-hand-authored -->
