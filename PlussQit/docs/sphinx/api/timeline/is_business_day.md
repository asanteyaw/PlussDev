# Timeline::is_business_day

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_business_day() const
```

## The Built-In Workweek Rule

`is_business_day()` marks local Monday through Friday dates. It exists for the
common case where a deterministic five-day workweek is the whole rule. It does
not consult `BusinessCalendar`, public holidays, exchange sessions, emergency
closures, half-days, or a region-specific weekend. Use an explicit calendar
when any of those policies matter.

The distinction is intentional: this method is a cheap weekday classifier,
not a promise that an organization was open.

## Result Contract

The result is a newly allocated boolean `Series` with exactly the receiver's
rank, shape, and element order. Scalars remain rank zero, empty extents remain
empty, and `NaT` becomes `false`. The receiver is not changed. The local date
is computed in the Timeline's current timezone, so call `tz_convert()` before
classification when the operating timezone differs.

## Example 1: A Complete Week

```cpp
const auto week = pluss::timeline::make({
    "2024-03-04T12:00:00Z", "2024-03-05T12:00:00Z",
    "2024-03-06T12:00:00Z", "2024-03-07T12:00:00Z",
    "2024-03-08T12:00:00Z", "2024-03-09T12:00:00Z",
    "2024-03-10T12:00:00Z", "",
});
pluss::series::print(std::cout, week.is_business_day());
```

```text
Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[true, true, true, true, true, false, false, false]
```

The last `false` is `NaT`, not a weekend.

## Example 2: Weekday Does Not Mean Holiday-Aware

```cpp
const auto dates = pluss::timeline::make({
    "2024-12-25T12:00:00Z",
    "2024-12-28T12:00:00Z",
});
pluss::series::print(std::cout, dates.is_business_day());
```

```text
Series(shape=[2], strides=[1], dtype=bool, platform=cpu, numel=2, contiguous=true)
data=[true, false]
```

The Wednesday is true under the built-in weekday rule regardless of its
holiday status in any particular jurisdiction. The Saturday is false.

## Example 3: Timezone Can Change The Workday

```cpp
const auto utc = pluss::timeline::make("2024-03-09T01:30:00Z");
const auto local = utc.tz_convert("America/New_York");
std::cout << std::boolalpha
          << "UTC=" << utc.is_business_day().item<bool>() << '\n'
          << "New York=" << local.is_business_day().item<bool>() << '\n';
```

```text
UTC=false
New York=true
```

The instant is Saturday in UTC but Friday in New York.

## Example 4: Empty Shape Is Preserved

```cpp
const auto scalar = pluss::timeline::make("2024-03-08T12:00:00Z");
const auto empty = pluss::timeline::full({0}, scalar);
pluss::series::print(std::cout, empty.is_business_day());
```

```text
Series(shape=[0], strides=[1], dtype=bool, platform=cpu, numel=0, contiguous=true)
[]
```

## Example 5: Define The Rule From Named Days

```cpp
const auto named = week.is_monday().logical_or(week.is_tuesday())
    .logical_or(week.is_wednesday()).logical_or(week.is_thursday())
    .logical_or(week.is_friday());
std::cout << std::boolalpha << named.equal(week.is_business_day()) << '\n';
```

```text
true
```

## Gotchas And Failure Modes

- On valid instants, `is_business_day()` is the complement of `is_weekend()`.
  At `NaT`, both are false, so plain logical negation is not equivalent.
- The method classifies values; it does not filter, reorder, or regularize.
- All supported Timeline ranks are accepted and no dimensions are reduced.
- One call performs one pass and allocates one boolean result.

## See Also

- [`is_weekend`](is_weekend.md)
- [`weekday`](weekday.md)
- [`span`](span.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hours_between.html"><span class="label">Previous</span><span class="title">hours_between</span></a>
  <a class="next-link" href="is_friday.html"><span class="label">Next</span><span class="title">is_friday</span></a>
</div>
<!-- pluss-hand-authored -->
