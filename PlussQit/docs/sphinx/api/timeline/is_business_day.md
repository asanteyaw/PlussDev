# Timeline::is_business_day

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_business_day() const
```

`is_business_day()` is the sole public spelling.

## Apply The Built-In Weekday Rule

`is_business_day()` returns `true` from Monday through Friday and `false` on
Saturday, Sunday, and `NaT`. Its name describes a conventional five-day week,
not a holiday-aware business calendar.

The method preserves every input dimension and returns boolean values.

## Example: Weekday Classification

```cpp
const auto week = pluss::timeline::make({
    "2024-03-04T12:00:00Z", "2024-03-05T12:00:00Z",
    "2024-03-06T12:00:00Z", "2024-03-07T12:00:00Z",
    "2024-03-08T12:00:00Z", "2024-03-09T12:00:00Z",
    "2024-03-10T12:00:00Z", "",
});
pluss::series::print(std::cout, week.is_business_day());
```

### Output

```text
Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[true, true, true, true, true, false, false, false]
```

The final `false` marks `NaT`, not a weekend. On valid instants,
`is_business_day()` and `is_weekend()` are complements; across a Timeline that
may contain `NaT`, retain `isvalid()` to express that identity correctly.

## What The Method Does Not Model

Public holidays, emergency closures, exchange sessions, half-days, and regions
whose work week is not Monday-Friday require application data or a dedicated
calendar. This predicate intentionally supplies only the deterministic weekday
rule.

## Gotchas And Failure Modes

- Friday is always `true`, even when it is a holiday in your domain.
- `NaT` is `false`; it is not classified as a weekend.
- Classification uses local date, so choose the operating timezone first.

## See Also

- [`is_weekend`](is_weekend.md)
- [`is_monday`](is_monday.md)
- [`weekday`](weekday.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hours_between.html"><span class="label">Previous</span><span class="title">hours_between</span></a>
  <a class="next-link" href="is_monday.html"><span class="label">Next</span><span class="title">is_monday</span></a>
</div>
<!-- pluss-hand-authored -->
