# Timeline::is_monday

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_monday() const
```

`is_monday()` is the sole public spelling.

## Select Local Mondays

`is_monday()` returns a shape-preserving boolean mask that is `true` exactly
when the local weekday is Monday. It is the named equivalent of comparing
`weekday()` with `0`, but communicates scheduling intent directly. `NaT`
returns `false`.

## Example: The First Day In A Complete Week

```cpp
const auto week = pluss::timeline::make({
    "2024-03-04T12:00:00Z", "2024-03-05T12:00:00Z",
    "2024-03-06T12:00:00Z", "2024-03-07T12:00:00Z",
    "2024-03-08T12:00:00Z", "2024-03-09T12:00:00Z",
    "2024-03-10T12:00:00Z", "",
});
pluss::series::print(std::cout, week.is_monday());
```

### Output

```text
Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[true, false, false, false, false, false, false, false]
```

Use the mask for Monday-specific workflows such as weekly opening reports. It
does not establish that observations are weekly, sorted, or unique; it only
classifies each element independently.

## Gotchas And Failure Modes

- Monday is determined after applying the Timeline timezone.
- `NaT` and all six other weekdays return `false`.
- For any Monday-through-Friday date, `is_business_day()` is also `true`.

## See Also

- [`weekday`](weekday.md)
- [`is_business_day`](is_business_day.md)
- [`is_tuesday`](is_tuesday.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_business_day.html"><span class="label">Previous</span><span class="title">is_business_day</span></a>
  <a class="next-link" href="is_tuesday.html"><span class="label">Next</span><span class="title">is_tuesday</span></a>
</div>
<!-- pluss-hand-authored -->
