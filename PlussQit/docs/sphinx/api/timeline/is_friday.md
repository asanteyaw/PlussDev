# Timeline::is_friday

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_friday() const
```

`is_friday()` is the sole public spelling.

## Select Local Fridays

`is_friday()` marks every timestamp whose local date is Friday. The output is
a shape-preserving boolean Series; `NaT` becomes `false`.

## Example

```cpp
const auto week = pluss::timeline::make({
    "2024-03-04T12:00:00Z", "2024-03-05T12:00:00Z",
    "2024-03-06T12:00:00Z", "2024-03-07T12:00:00Z",
    "2024-03-08T12:00:00Z", "2024-03-09T12:00:00Z",
    "2024-03-10T12:00:00Z", "",
});
pluss::series::print(std::cout, week.is_friday());
```

### Output

```text
Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[false, false, false, false, true, false, false, false]
```

Friday is the final `true` under the built-in Monday-Friday business-day rule.
That does not imply it is an open day for a particular market, country, or
organization; holiday policy remains external.

## Combining Friday With Other Rules

A rule such as “Friday and month end” can be expressed by combining this mask
with `is_month_end()`. Each predicate describes calendar membership, so their
intersection remains elementwise and shape-aligned.

## Gotchas And Failure Modes

- Friday is not treated as a weekend.
- `false` can mean another weekday or `NaT`.
- A timezone change near midnight can alter the result.

## See Also

- [`is_thursday`](is_thursday.md)
- [`is_saturday`](is_saturday.md)
- [`is_month_end`](is_month_end.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_thursday.html"><span class="label">Previous</span><span class="title">is_thursday</span></a>
  <a class="next-link" href="is_saturday.html"><span class="label">Next</span><span class="title">is_saturday</span></a>
</div>
<!-- pluss-hand-authored -->
