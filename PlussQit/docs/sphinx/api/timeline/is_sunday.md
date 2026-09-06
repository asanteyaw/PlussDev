# Timeline::is_sunday

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_sunday() const
```

Aliases: `sun_is()` and `is_sun()`.

## Select Local Sundays

`is_sunday()` marks timestamps whose local date is Sunday. Although Sunday is
the final day in the ISO-style Monday-first week used by `weekday()`, this
method requires no numeric weekday knowledge. It returns a shape-preserving
boolean Series and maps `NaT` to `false`.

## Example

```cpp
const auto week = pluss::timeline::make({
    "2024-03-04T12:00:00Z", "2024-03-05T12:00:00Z",
    "2024-03-06T12:00:00Z", "2024-03-07T12:00:00Z",
    "2024-03-08T12:00:00Z", "2024-03-09T12:00:00Z",
    "2024-03-10T12:00:00Z", "",
});
pluss::series::print(std::cout, week.is_sunday());
```

### Output

```text
Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[false, false, false, false, false, false, true, false]
```

Across seven valid dates containing one complete week, exactly one named-day
predicate is true per element. At `NaT`, all seven predicates are false. Their
combined OR therefore equals `isvalid()` for this complete weekday partition.

## Gotchas And Failure Modes

- Sunday corresponds to `weekday() == 6`, not zero.
- It is part of `is_weekend()` and excluded by `is_business_day()`.
- Named weekday tests classify; they do not sort or aggregate.

## Summary

Use the named weekday methods when the day itself expresses the business rule.
Use `weekday()` when the weekday is data-driven. In both cases, choose the
timezone first and retain validity when missing timestamps are possible.

## See Also

- [`weekday`](weekday.md)
- [`is_saturday`](is_saturday.md)
- [`is_weekend`](is_weekend.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_saturday.html"><span class="label">Previous</span><span class="title">is_saturday</span></a>
  <a class="next-link" href="date_range.html"><span class="label">Next</span><span class="title">date_range</span></a>
</div>
<!-- pluss-hand-authored -->
