# Timeline::is_month_start

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_month_start() const
```

Aliases: `is_month_begin()` and `is_monthstart()`.

## Find The First Local Date Of Each Month

`is_month_start()` is `true` when the local day-of-month is `1`. Time-of-day is
irrelevant: midnight, noon, and the final nanosecond of the first date all
belong to the month start. The method does not ask whether the timestamp is the
first observed row in your data.

The boolean output preserves shape. `NaT` returns `false`.

## Example: Month And Year Starts

```cpp
const auto calendar = pluss::timeline::make({
    "2019-12-31T12:00:00Z", "2020-01-01T12:00:00Z",
    "2020-02-29T12:00:00Z", "2020-03-31T12:00:00Z",
    "2020-04-01T12:00:00Z", "2020-12-31T12:00:00Z",
    "2021-01-01T12:00:00Z", "2024-02-29T12:00:00Z", "",
});
pluss::series::print(std::cout, calendar.is_month_start());
```

### Output

```text
Series(shape=[9], strides=[1], dtype=bool, platform=cpu, numel=9, contiguous=true)
data=[false, true, false, false, true, false, true, false, false]
```

January 1 and April 1 qualify. February 29 does not, despite being a notable
calendar boundary of another kind.

## Using The Mask Correctly

Use this predicate to select actual observations landing on day 1. It does not
manufacture missing month starts, resample irregular data, or choose the first
available business day. Those require an explicit alignment policy.

## Gotchas And Failure Modes

- “Start” means calendar membership, not first array position.
- The Timeline timezone decides the local date.
- `false` includes both ordinary dates and `NaT`; preserve `isvalid()` when
  missingness matters.

## See Also

- [`is_month_end`](is_month_end.md)
- [`is_quarter_start`](is_quarter_start.md)
- [`day`](day.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_leap_year.html"><span class="label">Previous</span><span class="title">is_leap_year</span></a>
  <a class="next-link" href="is_month_end.html"><span class="label">Next</span><span class="title">is_month_end</span></a>
</div>
<!-- pluss-hand-authored -->
