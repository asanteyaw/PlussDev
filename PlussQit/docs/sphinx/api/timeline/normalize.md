# Timeline::normalize

[Back to Timeline API Reference](../../timeline_api_reference)

## Signatures

```cpp
Timeline::normalize() -> Timeline
Timeline::date() -> Timeline
Timeline::time() -> Timeline
```

## What It Does

Projects timestamps into simpler temporal views.

This page covers three closely related surfaces:

- `normalize()`
  Floor each timestamp to the start of its day.
- `date()`
  The same day-level projection as `normalize()`.
- `time()`
  Keep only the time-of-day portion and anchor it to the epoch date
  `1970-01-01`.

## Returns

`Timeline`
    A projected timeline.

## Examples

Example 1. Normalize timestamps to daily anchors.

```cpp
auto out = base.normalize();
std::cout << pluss::print_timeline(out) << "\n";
```

### Output

```text
Timeline(shape=[4], tz=UTC)
[ 2024-11-01 00:00:00, 2024-11-01 00:00:00, 2024-12-31 00:00:00, 2025-01-01 00:00:00 ]
```

Example 2. `date()` is the same day-level projection with a more literal name.

```cpp
auto out = base.date();
std::cout << pluss::print_timeline(out) << "\n";
```

### Output

```text
Timeline(shape=[4], tz=UTC)
[ 2024-11-01 00:00:00, 2024-11-01 00:00:00, 2024-12-31 00:00:00, 2025-01-01 00:00:00 ]
```

Example 3. `time()` keeps the clock component and discards the calendar date.

```cpp
auto out = base.time();
std::cout << pluss::print_timeline(out) << "\n";
```

### Output

```text
Timeline(shape=[4], tz=UTC)
[ 1970-01-01 09:00:00, 1970-01-01 10:15:30, 1970-01-01 23:59:59, 1970-01-01 00:00:00 ]
```

## Notes

- `normalize()` and `date()` are equivalent in the current implementation.
- `time()` is especially useful when you want to compare intraday patterns
  independent of calendar date.

## See Also

- [`Timeline::floor`](floor.md)
- [`Timeline::to_utc`](to_utc.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="round.html">
    <span class="label">Previous</span>
    <span class="title">round</span>
  </a>
  <a class="next-link" href="tz_convert.html">
    <span class="label">Next</span>
    <span class="title">tz_convert</span>
  </a>
</div>
