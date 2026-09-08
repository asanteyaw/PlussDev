# Timeline::is_weekend

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_weekend() const
```

`is_weekend()` is the sole public spelling.

## Classify Saturday And Sunday

`is_weekend()` is `true` when the local date is Saturday or Sunday. It is a
fixed weekday classification: it does not know regional holidays, shifted
work weeks, exchange calendars, or organization-specific closures.

The result is a shape-preserving boolean Series. `NaT` returns `false`.

## Example: A Full Week Plus NaT

```cpp
const auto week = pluss::timeline::make({
    "2024-03-04T12:00:00Z", "2024-03-05T12:00:00Z",
    "2024-03-06T12:00:00Z", "2024-03-07T12:00:00Z",
    "2024-03-08T12:00:00Z", "2024-03-09T12:00:00Z",
    "2024-03-10T12:00:00Z", "",
});
pluss::series::print(std::cout, week.is_weekend());
```

### Output

```text
Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[false, false, false, false, false, true, true, false]
```

Saturday and Sunday are selected; the `NaT` position remains `false`.

## Shape-Preserving Masks

On a `[2,2]` Timeline containing Monday, Saturday, `NaT`, and Sunday, the mask
is also `[2,2]` with values `[[false, true], [false, true]]`. This makes the
predicate suitable for elementwise `where(...)` logic without flattening the
temporal layout.

## Gotchas And Failure Modes

- Weekend means Saturday or Sunday only.
- `is_weekend()` is not the logical negation of `is_business_day()` at `NaT`;
  both return `false` there.
- Timezone conversion can change the weekday near local midnight.

## See Also

- [`is_business_day`](is_business_day.md)
- [`is_saturday`](is_saturday.md)
- [`is_sunday`](is_sunday.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_year_end.html"><span class="label">Previous</span><span class="title">is_year_end</span></a>
  <a class="next-link" href="is_business_day.html"><span class="label">Next</span><span class="title">is_business_day</span></a>
</div>
<!-- pluss-hand-authored -->
