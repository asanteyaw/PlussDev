# Timeline::floor

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline::floor(std::string_view unit) -> Timeline
```

## What It Does

Rounds each timestamp down to the start of the requested unit.

This is the "snap backward" temporal primitive. It is commonly used to build
calendar buckets, normalize intraday times to daily anchors, or prepare a time
column for grouping.

## Parameters

**unit**

The rounding unit.

Typical practical values include:

- `"D"`
  Day.
- `"H"`
  Hour.
- `"M"`
  Minute, when supported by the surrounding path.

The meaning is always "move backward to the boundary of that unit".

## Returns

`Timeline`
    A timeline of rounded timestamps.

## Examples

Example 1. Floor timestamps to the start of their day.

```cpp
auto out = base.floor("D");
std::cout << pluss::timeline::print(out) << "\n";
```

### Output

```text
Timeline(shape=[4], tz=UTC)
[ 2024-11-01 00:00:00, 2024-11-01 00:00:00, 2024-12-31 00:00:00, 2025-01-01 00:00:00 ]
```

## Notes

- Floor never moves a timestamp forward.
- [`normalize`](normalize.md) and [`date`](normalize.md) are day-level
  conveniences built on this idea.

## See Also

- [`Timeline::ceil`](ceil.md)
- [`Timeline::round`](round.md)
- [`Timeline::normalize`](normalize.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month_name.html">
    <span class="label">Previous</span>
    <span class="title">month_name</span>
  </a>
  <a class="next-link" href="ceil.html">
    <span class="label">Next</span>
    <span class="title">ceil</span>
  </a>
</div>
