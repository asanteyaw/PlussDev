# Timeline::is_month_end

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_month_end() const
```

`is_month_end()` is the sole public spelling.

## Recognize Each Month's Actual Last Date

`is_month_end()` compares each local date with the true final day of its month.
It therefore handles 28-, 29-, 30-, and 31-day months without a hand-written
day list. Any time on that final date returns `true`.

The output is a shape-preserving boolean Series. `NaT` returns `false`.

## Example: Variable Month Lengths

```cpp
const auto calendar = pluss::timeline::make({
    "2019-12-31T12:00:00Z", "2020-01-01T12:00:00Z",
    "2020-02-29T12:00:00Z", "2020-03-31T12:00:00Z",
    "2020-04-01T12:00:00Z", "2020-12-31T12:00:00Z",
    "2021-01-01T12:00:00Z", "2024-02-29T12:00:00Z", "",
});
pluss::series::print(std::cout, calendar.is_month_end());
```

### Output

```text
Series(shape=[9], strides=[1], dtype=bool, platform=cpu, numel=9, contiguous=true)
data=[true, false, true, true, false, true, false, true, false]
```

Both leap days qualify, as do March 31 and December 31. This is safer than
testing `day() == 31`, which misses shorter month ends.

## Calendar End Versus Last Observation

The predicate does not inspect neighboring elements. If a dataset stops on
March 30, that row is still not a calendar month end. Conversely, every event
on March 31 is marked even if several such events occur.

## Gotchas And Failure Modes

- Time-of-day does not need to be the last instant of the date.
- This is calendar month end, not business month end.
- `NaT` is `false`; use `isvalid()` to separate absent instants.

## See Also

- [`is_month_start`](is_month_start.md)
- [`is_quarter_end`](is_quarter_end.md)
- [`is_year_end`](is_year_end.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_month_start.html"><span class="label">Previous</span><span class="title">is_month_start</span></a>
  <a class="next-link" href="is_quarter_start.html"><span class="label">Next</span><span class="title">is_quarter_start</span></a>
</div>
<!-- pluss-hand-authored -->
