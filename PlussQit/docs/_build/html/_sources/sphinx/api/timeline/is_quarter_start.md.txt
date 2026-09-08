# Timeline::is_quarter_start

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_quarter_start() const
```

`is_quarter_start()` is the sole public spelling.

## Find Calendar-Quarter Opening Dates

`is_quarter_start()` is `true` on January 1, April 1, July 1, and October 1 in
the Timeline's current timezone. It uses calendar quarters beginning in
January; it has no inferred fiscal-year offset.

The result is boolean and shape-preserving. `NaT` becomes `false`.

## Example

```cpp
const auto calendar = pluss::timeline::make({
    "2019-12-31T12:00:00Z", "2020-01-01T12:00:00Z",
    "2020-02-29T12:00:00Z", "2020-03-31T12:00:00Z",
    "2020-04-01T12:00:00Z", "2020-12-31T12:00:00Z",
    "2021-01-01T12:00:00Z", "2024-02-29T12:00:00Z", "",
});
pluss::series::print(std::cout, calendar.is_quarter_start());
```

### Output

```text
Series(shape=[9], strides=[1], dtype=bool, platform=cpu, numel=9, contiguous=true)
data=[false, true, false, false, true, false, true, false, false]
```

January 1 is both a year and quarter start; April 1 is a quarter and month
start but not a year start. Predicates can be combined when that distinction
is useful.

## Policy Boundary

A fiscal quarter beginning in February is not represented by this predicate.
Define that offset explicitly rather than relabeling the calendar result,
because the associated fiscal year may also differ.

## Gotchas And Failure Modes

- The date must be the first day of a quarter-opening month.
- “Start” does not mean first observed timestamp in the quarter.
- Timezone conversion can move an instant across the boundary.

## See Also

- [`quarter`](quarter.md)
- [`is_quarter_end`](is_quarter_end.md)
- [`is_year_start`](is_year_start.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_month_end.html"><span class="label">Previous</span><span class="title">is_month_end</span></a>
  <a class="next-link" href="is_quarter_end.html"><span class="label">Next</span><span class="title">is_quarter_end</span></a>
</div>
<!-- pluss-hand-authored -->
