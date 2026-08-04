# TimeIndex::to_note_iso

[Back to TimeIndex API Reference](../../timeindex_api_reference)

## Signatures

```cpp
TimeIndex::to_note_iso() const -> Note
TimeIndex::tostring() const -> Note
TimeIndex::to_string() const -> Note
TimeIndex::format_iso() const -> Note
TimeIndex::format() const -> Note
```

## What It Does

Formats the time axis as ISO-style text.

The alias family exists so callers can choose whichever naming style reads best
in their context, but the underlying operation is the same.

This is the main text-export surface for the axis.

## Returns

`Note`
    A 1D text object containing ISO-formatted timestamps.

## Examples

Example 1. Export the axis as explicit ISO text.

```cpp
auto out = base.to_note_iso();
std::cout << pluss::print_note(out) << "\n";
```

### Output

```text
Note(shape=[5], dim=1, numel=5, encoding=utf8)
["2024-11-01T00:00:00.000000000+00:00", "2024-11-01T00:20:00.000000000+00:00", "2024-11-01T00:40:00.000000000+00:00", "2024-11-01T01:00:00.000000000+00:00", "2024-11-01T01:20:00.000000000+00:00"]
```

Example 2. Use the resulting `Note` directly in text-oriented Pluss workflows.

```cpp
auto labels = base.to_note_iso();
std::cout << pluss::print_note(labels) << "\n";
```

## Notes

- This is the most direct export surface for exact textual timestamps.
- `+00:00` or similar offsets are expected in the formatted output; they are the
  explicit-offset form of timezone information.
- The output is a `Note`, so it can continue directly into Note-native text
  processing without an extra conversion step.

## See Also

- [`TimeIndex::tz_convert`](tz_convert.md)
- [`TimeIndex::timeline`](timeline.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="normalize.html">
    <span class="label">Previous</span>
    <span class="title">normalize</span>
  </a>
  <a class="next-link" href="tz_convert.html">
    <span class="label">Next</span>
    <span class="title">tz_convert</span>
  </a>
</div>
