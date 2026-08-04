# Timeline::add_nanos

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline::add_nanos(std::int64_t nanos) -> Timeline
```

## What It Does

Shifts every defined timestamp by a fixed nanosecond offset.

This is the low-level scalar-offset surface to use when your shift is already
expressed in raw nanoseconds.

## Parameters

**nanos**

The number of nanoseconds to add.

- positive values move forward in time
- negative values move backward in time

## Returns

`Timeline`
    A shifted timeline.

## Examples

Example 1. Add one second to every timestamp.

```cpp
auto out = base.add_nanos(1'000'000'000LL);
std::cout << pluss::print_timeline(out) << "\n";
```

### Output

```text
Timeline(shape=[4], tz=UTC)
[ 2024-11-01 09:00:01, 2024-11-01 10:15:31, 2025-01-01 00:00:00, 2025-01-01 00:00:01 ]
```

## Notes

- Missing timestamps remain missing.
- Use `add_duration(...)` when the shift is already represented as a
  `Duration`.

## See Also

- [`Timeline::add_duration`](add_duration.md)
- [`Timeline::diff`](diff.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="where.html">
    <span class="label">Previous</span>
    <span class="title">where</span>
  </a>
  <a class="next-link" href="searchsorted_left.html">
    <span class="label">Next</span>
    <span class="title">searchsorted_left</span>
  </a>
</div>
