# Timeline::set_month

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::set_month(int month) const
```

## Move Dates To A Named Month

`set_month()` replaces the local month of every defined timestamp with one
numeric month in `[1, 12]`. Year, day-of-month, time of day, nanoseconds,
timezone, and shape are retained.

This method answers a precise calendar-editing need: "keep the day and clock,
but place these values in February." It does not mean "advance to the second
month from here," and it does not mean "move by a month." Because the original
day is retained, the requested month must contain that day.

## Example: One Month Across A Matrix

```cpp
const auto t = pluss::timeline::make({
    {"2024-01-15T23:58:59.123456789Z", "2024-06-20T04:05:06.000000004Z"},
    {"2024-09-28T12:30:45.900000000Z", "2024-12-01T00:00:00.000000001Z"},
});

std::cout << pluss::note::print(
    pluss::timeline::format(t.set_month(2), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["2024-02-15T23:58:59.123456789+00:00", "2024-02-20T04:05:06.000000004+00:00"], 
  ["2024-02-28T12:30:45.900000000+00:00", "2024-02-01T00:00:00.000000001+00:00"]
]
```

Rank and layout are preserved logically; this is not a flattening operation.
The example also shows that subsecond fields survive unchanged.

## Strict Day Compatibility

```cpp
try {
  const auto month_end = pluss::timeline::make("2024-05-31T08:00:00Z");
  (void)month_end.set_month(2);
} catch (const pluss::Error&) {
  std::cout << "day does not exist in requested month\n";
}
```

### Output

```text
day does not exist in requested month
```

There is deliberately no end-of-month clamp. If 31 May becoming February
should mean February's final day in your application, express that policy
explicitly before replacement; `set_month()` will not guess it.

Values outside `[1, 12]` throw before traversal, including for an empty
receiver. If any lane fails day compatibility, no result is returned. `NaT`
lanes remain `NaT` and do not make an otherwise valid replacement fail.

## Replacement Is Not A Month Offset

```cpp
const auto t = pluss::timeline::make("2024-11-10T09:30:00Z");
std::cout << pluss::timeline::format(t.set_month(2), "%F %T").at(0) << '\n';
```

### Output

```text
2024-02-10 09:30:00
```

The result is February of the same year, not January 2025. This makes
`set_month()` suitable for constructing fixed calendar scenarios, not for
measuring or shifting elapsed time.

## See Also

- [`month`](month.md): inspect the local month number.
- [`set_day`](set_day.md): replace the day within each retained year and month.
- [`set_year`](set_year.md): replace the civil year.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set_minute.html"><span class="label">Previous</span><span class="title">set_minute</span></a>
  <a class="next-link" href="set_second.html"><span class="label">Next</span><span class="title">set_second</span></a>
</div>
<!-- pluss-hand-authored -->
