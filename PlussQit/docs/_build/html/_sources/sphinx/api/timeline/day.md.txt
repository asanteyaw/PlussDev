# Timeline::day

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::day() const
```

Aliases: `d()`, `dom()`, `days()`, and `day_()`.

## Read The Day Of Month

`day()` extracts the one-based day within each local month. Valid values run
from `1` to the last day of that particular month, so the upper bound depends
on month and year. Leap-day timestamps correctly produce `29` in February.

The method preserves the Timeline's shape and returns `int32`. It is not the
same as `weekday()` (position in the week) or `dayofyear()` (position in the
year). A missing timestamp produces `-1`.

## Example: Month Boundaries And Leap Day

```cpp
const auto events = pluss::timeline::make({
    "2020-12-31T23:59:59.123456789Z",
    "2021-01-01T00:00:00.987654321Z",
    "2024-02-29T06:07:08.009010011Z",
    "",
});

pluss::series::print(std::cout, events.day());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=int32, platform=cpu, numel=4, contiguous=true)
data=[31, 1, 29, -1]
```

The `29` is meaningful only together with February 2024. If you need a unique
date key, retain `year()` and `month()` as well, or use `date()` to keep the
temporal representation.

## Choosing The Right Day Extractor

- Use `day()` for billing dates such as “the 15th of each month.”
- Use `weekday()` for Monday-through-Sunday rules.
- Use `dayofyear()` for annual seasonality and ordinal progress through a year.
- Use `date()` when later temporal operations still need timestamp semantics.

## Gotchas And Failure Modes

- Values are one-based and are not guaranteed to reach `31` in every month.
- `-1` means `NaT`; filter with `isvalid()` before arithmetic on day numbers.
- The current timezone controls which side of midnight an instant occupies.

## See Also

- [`month`](month.md)
- [`weekday`](weekday.md)
- [`dayofyear`](dayofyear.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month.html"><span class="label">Previous</span><span class="title">month</span></a>
  <a class="next-link" href="hour.html"><span class="label">Next</span><span class="title">hour</span></a>
</div>
<!-- pluss-hand-authored -->
