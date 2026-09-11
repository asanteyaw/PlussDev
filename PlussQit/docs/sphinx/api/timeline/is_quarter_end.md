# Timeline::is_quarter_end

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_quarter_end() const
```

## Calendar-Quarter Membership

`is_quarter_end()` marks the final local dates of March, June, September, and
December. It checks both the quarter-ending month and that month's actual last
day. March 30 is therefore false, as is February 29 even though leap day is a
month end.

These are fixed January-based civil quarters. “End” does not mean the latest
value in storage, the last observed date, or the final working day.

## Result Contract

The returned boolean `Series` preserves scalar, vector, matrix, three-dimensional,
and empty shapes exactly. `NaT` maps to `false`. The operation allocates its
result, leaves the receiver untouched, and interprets boundaries in the
Timeline's current timezone.

## Example 1: Month End Is Not Enough

```cpp
const auto calendar = pluss::timeline::make({
    "2019-12-31T12:00:00Z", "2020-01-01T12:00:00Z",
    "2020-02-29T12:00:00Z", "2020-03-31T12:00:00Z",
    "2020-04-01T12:00:00Z", "2020-12-31T12:00:00Z",
    "2021-01-01T12:00:00Z", "2024-02-29T12:00:00Z", "",
});
pluss::series::print(std::cout, calendar.is_quarter_end());
```

```text
Series(shape=[9], strides=[1], dtype=bool, platform=cpu, numel=9, contiguous=true)
data=[true, false, false, true, false, true, false, false, false]
```

February 29 is a month end but not a quarter end. December 31 is both a quarter
end and a year end.

## Example 2: Matrix Shape Is Preserved

```cpp
const auto dates = pluss::timeline::make({
    {"2024-01-01T23:59:59.999999999Z", "2024-01-02T00:00:00Z",
     "2024-02-29T00:00:00Z"},
    {"2024-03-31T12:00:00Z", "2024-04-01T00:00:00Z",
     "2024-12-31T00:00:00Z"},
});
pluss::series::print(std::cout, dates.is_quarter_end());
```

```text
Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=
⎡ false false false ⎤
⎣  true false  true ⎦
```

## Example 3: Timezone Can Reveal A Quarter End

```cpp
const auto utc = pluss::timeline::make("2021-01-01T00:30:00Z");
const auto local = utc.tz_convert("America/New_York");
std::cout << std::boolalpha
          << "UTC=" << utc.is_quarter_end().item<bool>() << '\n'
          << "New York=" << local.is_quarter_end().item<bool>() << '\n';
```

```text
UTC=false
New York=true
```

## Example 4: Verify The Calendar Relationship

```cpp
const auto ends = dates.is_quarter_end();
std::cout << std::boolalpha
          << ends.logical_and(dates.is_month_end()).equal(ends) << '\n';
```

```text
true
```

## Gotchas And Failure Modes

- Every quarter end is a month end, but not every month end closes a quarter.
- `NaT=false`; retain `isvalid()` when absence must be represented separately.
- Fiscal and working-day boundaries are outside this predicate's contract.
- Use `quarter_end_scalar()` for a native `bool` from scalar input.

## See Also

- [`quarter_end_scalar`](quarter_end_scalar.md)
- [`is_month_end`](is_month_end.md)
- [`is_year_end`](is_year_end.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_month_start.html"><span class="label">Previous</span><span class="title">is_month_start</span></a>
  <a class="next-link" href="is_quarter_start.html"><span class="label">Next</span><span class="title">is_quarter_start</span></a>
</div>
<!-- pluss-hand-authored -->
