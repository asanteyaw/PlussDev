# Timeline::is_leap_year

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_leap_year() const
```

`is_leap_year()` is the sole public spelling.

## Classify The Local Calendar Year

`is_leap_year()` returns `true` for every timestamp whose local civil year has
366 days. The Gregorian rule is applied in full: a year divisible by 4 is a
leap year, except a century year must also be divisible by 400. The method
tests year membership, not whether the timestamp itself is 29 February.

The result is a shape-preserving boolean Series. `NaT` becomes `false`; combine
with `isvalid()` if absent and non-leap positions must remain distinguishable.

## Example: Membership Across Boundaries

```cpp
const auto calendar = pluss::timeline::make({
    "2019-12-31T12:00:00Z", "2020-01-01T12:00:00Z",
    "2020-02-29T12:00:00Z", "2020-03-31T12:00:00Z",
    "2020-04-01T12:00:00Z", "2020-12-31T12:00:00Z",
    "2021-01-01T12:00:00Z", "2024-02-29T12:00:00Z", "",
});
pluss::series::print(std::cout, calendar.is_leap_year());
```

### Output

```text
Series(shape=[9], strides=[1], dtype=bool, platform=cpu, numel=9, contiguous=true)
data=[false, true, true, true, true, true, false, true, false]
```

Every 2020 timestamp is `true`, not only leap day. The final `false` belongs to
`NaT`, not to a known common year.

## Gotchas And Failure Modes

- This is an elementwise classifier, not one scalar property of the Timeline.
- Timezone conversion can change the local year near New Year.
- Do not infer validity from `false`; use `isvalid()`.
- For leap-day itself, combine `month() == 2` and `day() == 29`.

## See Also

- [`year`](year.md)
- [`dayofyear`](dayofyear.md)
- [`is_month_start`](is_month_start.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="week.html"><span class="label">Previous</span><span class="title">week</span></a>
  <a class="next-link" href="is_month_start.html"><span class="label">Next</span><span class="title">is_month_start</span></a>
</div>
<!-- pluss-hand-authored -->
