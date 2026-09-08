# Timeline::set_second

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline Timeline::set_second(int second) const
```

## Replace Seconds Without Losing Fractions

`set_second()` replaces the whole-second field of every defined local
timestamp. Its argument must be in `[0, 59]`. Calendar fields, hour, minute,
the fractional nanosecond field, timezone, rank, and shape are preserved.

The separation between seconds and fractional nanoseconds is important.
Setting seconds to zero produces `HH:MM:00.fraction`; it does not round or
truncate to an exact minute. Use a rounding operation when discarded
subseconds are the intended result.

## Example: Preserve Nanosecond Identity

```cpp
const auto t = pluss::timeline::make({
    {"2024-01-15T23:58:59.123456789Z", "2024-06-20T04:05:06.000000004Z"},
    {"2024-09-28T12:30:45.900000000Z", "2024-12-01T00:00:00.000000001Z"},
});

std::cout << pluss::note::print(
    pluss::timeline::format(t.set_second(0), "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["2024-01-15T23:58:00.123456789+00:00", "2024-06-20T04:05:00.000000004+00:00"], 
  ["2024-09-28T12:30:00.900000000+00:00", "2024-12-01T00:00:00.000000001+00:00"]
]
```

Notice the final value: it already had second zero, so its full timestamp is
unchanged. The method still returns a new timeline rather than conditionally
returning the receiver.

## Replacement Versus Flooring

```cpp
const auto t = pluss::timeline::make("2024-08-19T09:30:42.750000003Z");

std::cout << pluss::timeline::format(t.set_second(0), "%T.%f").at(0) << '\n';
std::cout << pluss::timeline::format(t.floor("min"), "%T.%f").at(0) << '\n';
```

### Output

```text
09:30:00.750000003
09:30:00.000000000
```

`set_second(0)` preserves the fraction because only the second field was
named. Minute flooring clears every field below minute precision.

## Failure And Edge Behavior

- Negative values and values greater than `59` throw; leap-second text is not
  represented by passing `60`.
- The argument is checked even if the timeline contains no elements.
- `NaT` propagates unchanged.
- The replacement is applied at every logical position of scalar, 1D, and
  higher-rank timelines, including strided storage.
- Local timestamps are reconstructed under `tz()`. Nonexistent local results
  throw; ambiguous results select their first occurrence.

## See Also

- [`second`](second.md): inspect the whole-second field.
- [`nanosecond`](nanosecond.md): inspect the fractional nanosecond remainder.
- [`floor`](floor.md): clear all fields below a chosen precision.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set_month.html"><span class="label">Previous</span><span class="title">set_month</span></a>
  <a class="next-link" href="set_year.html"><span class="label">Next</span><span class="title">set_year</span></a>
</div>
<!-- pluss-hand-authored -->
