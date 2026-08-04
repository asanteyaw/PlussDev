# TimeIndex::tz_convert

[Back to TimeIndex API Reference](../../timeindex_api_reference)

## Signatures

```cpp
TimeIndex::tz_convert(std::string_view tz_name) const -> TimeIndex
TimeIndex::to_utc() const -> TimeIndex
```

## What It Does

Converts the axis into a different timezone representation while preserving the
underlying instant in time.

`to_utc()` is the dedicated convenience case for conversion back to UTC.

So the moment stays the same while the wall-clock rendering changes.

## Parameters

**tz_name**

The destination timezone, for example:

- `"UTC"`
- `"America/New_York"`
- `"America/Edmonton"`

Choose the destination timezone based on how later human-facing or
calendar-facing work should interpret the same underlying instants.

## Returns

`TimeIndex`
    The same instant series rendered in the requested timezone.

## Examples

Example 1. Convert a UTC axis into New York wall-clock time.

```cpp
auto utc_midday = pluss::make_timeindex({
    "2024-11-01T16:00:00Z",
    "2024-11-01T18:30:00Z",
});

auto out = utc_midday.tz_convert("America/New_York");
std::cout << pluss::print_timeline(out) << "\n";
```

### Output

```text
Timeline(shape=[2], tz=America/New_York)
[ 2024-11-01 12:00:00, 2024-11-01 14:30:00 ]
```

Example 2. Dedicated conversion back to UTC.

```cpp
auto out = utc_midday.to_utc();
std::cout << pluss::print_timeline(out) << "\n";
```

### Output

```text
Timeline(shape=[2], tz=UTC)
[ 2024-11-01 16:00:00, 2024-11-01 18:30:00 ]
```

## Notes

- `tz_convert()` assumes the axis is already timezone-aware.
- Use `tz_localize()` instead when the timestamps are still naive local
  wall-clock values that need timezone meaning attached for the first time.

## See Also

- [`TimeIndex::tz_localize`](tz_localize.md)
- [`TimeIndex::to_note_iso`](to_note_iso.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_note_iso.html">
    <span class="label">Previous</span>
    <span class="title">to_note_iso</span>
  </a>
  <a class="next-link" href="tz_localize.html">
    <span class="label">Next</span>
    <span class="title">tz_localize</span>
  </a>
</div>
