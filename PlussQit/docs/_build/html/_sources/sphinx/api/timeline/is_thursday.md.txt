# Timeline::is_thursday

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Series Timeline::is_thursday() const
```

Aliases: `thu_is()` and `is_thu()`.

## Select Local Thursdays

`is_thursday()` returns `true` where a timestamp's local date is Thursday. Its
boolean result preserves all dimensions and positions. `NaT` returns `false`,
as it does for every named-weekday predicate.

## Example

```cpp
const auto week = pluss::timeline::make({
    "2024-03-04T12:00:00Z", "2024-03-05T12:00:00Z",
    "2024-03-06T12:00:00Z", "2024-03-07T12:00:00Z",
    "2024-03-08T12:00:00Z", "2024-03-09T12:00:00Z",
    "2024-03-10T12:00:00Z", "",
});
pluss::series::print(std::cout, week.is_thursday());
```

### Output

```text
Series(shape=[8], strides=[1], dtype=bool, platform=cpu, numel=8, contiguous=true)
data=[false, false, false, true, false, false, false, false]
```

For recurring Thursday logic, the named method is clearer than the equivalent
numeric comparison `weekday() == 3`. For a configurable weekday supplied as
data, the numeric form is more appropriate.

## Gotchas And Failure Modes

- Thursday is classified as a business day by the built-in weekday rule.
- The method does not account for holidays or closures.
- Convert to the desired local timezone before classification.

## See Also

- [`is_wednesday`](is_wednesday.md)
- [`is_friday`](is_friday.md)
- [`is_business_day`](is_business_day.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="is_wednesday.html"><span class="label">Previous</span><span class="title">is_wednesday</span></a>
  <a class="next-link" href="is_friday.html"><span class="label">Next</span><span class="title">is_friday</span></a>
</div>
<!-- pluss-hand-authored -->
