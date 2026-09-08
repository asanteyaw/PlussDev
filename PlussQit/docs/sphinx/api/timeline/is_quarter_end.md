# Timeline::is_quarter_end

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_quarter_end() const
```

`is_quarter_end()` is the sole public spelling.

## Find Calendar-Quarter Closing Dates

`is_quarter_end()` recognizes the final local date of March, June, September,
and December. It checks both the quarter-ending month and that month's actual
last day, so an arbitrary date in those months does not qualify.

The result has boolean dtype and preserves all input dimensions. `NaT` is
`false`.

## Example: Month End Is Necessary But Not Sufficient

```cpp
const auto calendar = pluss::timeline::make({
    "2019-12-31T12:00:00Z", "2020-01-01T12:00:00Z",
    "2020-02-29T12:00:00Z", "2020-03-31T12:00:00Z",
    "2020-04-01T12:00:00Z", "2020-12-31T12:00:00Z",
    "2021-01-01T12:00:00Z", "2024-02-29T12:00:00Z", "",
});
pluss::series::print(std::cout, calendar.is_quarter_end());
```

### Output

```text
Series(shape=[9], strides=[1], dtype=bool, platform=cpu, numel=9, contiguous=true)
data=[true, false, false, true, false, true, false, false, false]
```

February 29 is a month end but not a quarter end. March 31 qualifies, while
December 31 is both a quarter end and year end.

## Composing Boundary Masks

Every `true` from `is_quarter_end()` is also `true` under `is_month_end()`, but
the reverse is not true. This containment is useful when progressively
narrowing a reporting calendar without reconstructing month-length rules.

## Gotchas And Failure Modes

- Calendar quarters are fixed to January-March, April-June, July-September,
  and October-December.
- This does not find the last business day or last observed row.
- `NaT` and ordinary non-boundary dates both return `false`.

## See Also

- [`is_month_end`](is_month_end.md)
- [`is_quarter_start`](is_quarter_start.md)
- [`is_year_end`](is_year_end.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_quarter_start.html"><span class="label">Previous</span><span class="title">is_quarter_start</span></a>
  <a class="next-link" href="is_year_start.html"><span class="label">Next</span><span class="title">is_year_start</span></a>
</div>
<!-- pluss-hand-authored -->
