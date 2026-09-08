# Timeline::is_year_end

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_year_end() const
```

`is_year_end()` is the sole public spelling.

## Recognize December 31 In Local Time

`is_year_end()` returns `true` for timestamps whose local date is December 31.
It identifies calendar membership, not the chronologically greatest element
or the last available observation. Shape is preserved, dtype is boolean, and
`NaT` produces `false`.

## Example

```cpp
const auto calendar = pluss::timeline::make({
    "2019-12-31T12:00:00Z", "2020-01-01T12:00:00Z",
    "2020-02-29T12:00:00Z", "2020-03-31T12:00:00Z",
    "2020-04-01T12:00:00Z", "2020-12-31T12:00:00Z",
    "2021-01-01T12:00:00Z", "2024-02-29T12:00:00Z", "",
});
pluss::series::print(std::cout, calendar.is_year_end());
```

### Output

```text
Series(shape=[9], strides=[1], dtype=bool, platform=cpu, numel=9, contiguous=true)
data=[true, false, false, false, false, true, false, false, false]
```

Both 2019 and 2020 close on December 31. Leap years change the number of days
in the year, not its final month and day.

## Timezone Boundary

The instant `2021-01-01T00:30:00Z` is not a year end in UTC, but after
conversion to New York it renders as `2020-12-31T19:30:00-05:00` and
`is_year_end()` becomes `true`. Boundary classification must therefore follow,
not precede, timezone selection.

## Gotchas And Failure Modes

- This is calendar year end, not fiscal year end or final business day.
- Multiple events on December 31 all qualify.
- `false` does not imply a valid timestamp; check `isvalid()` when needed.

## See Also

- [`is_year_start`](is_year_start.md)
- [`is_quarter_end`](is_quarter_end.md)
- [`max`](min.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_year_start.html"><span class="label">Previous</span><span class="title">is_year_start</span></a>
  <a class="next-link" href="is_weekend.html"><span class="label">Next</span><span class="title">is_weekend</span></a>
</div>
<!-- pluss-hand-authored -->
