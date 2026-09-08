# Timeline::to_note_iso

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline::to_note_iso() -> Note
```

Formats the full timeline into canonical ISO-style text.

This is the shape-preserving text export surface for timestamps when you want a
stable, explicit, machine-friendly representation but your source is not just a
single scalar moment.

## Parameters

This method takes no parameters.

The important practical split is:

- use `to_note_iso()` for vectorized or higher-rank conversion
- use [`isoformat()`](isoformat.md) for a single scalar timestamp
- use [`strftime(...)`](strftime.md) when you want custom presentation rather
  than canonical ISO output

## Returns

`Note`
    One ISO-style string for each timestamp, preserving the source shape.

## Notes

- The output keeps explicit offsets and fractional precision.
- `NaT` stays visible as `NaT`, which is usually what you want for diagnostics,
  text joins, and export pipelines where missingness must remain obvious.
- This is the high-convenience choice when you do not want to think about a
  custom format string.

## Examples

Example 1. Convert a clean 1D timeline into canonical timestamp text.

```cpp
auto labels = base.to_note_iso();
std::cout << pluss::note::print(labels) << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["2024-11-01T09:00:00.000000000+00:00", "2024-11-01T10:15:30.000000000+00:00", "2024-12-31T23:59:59.000000000+00:00", "2025-01-01T00:00:00.000000000+00:00"]
```

Example 2. Preserve absent instants visibly during ISO conversion.

```cpp
auto labels = with_nat.to_note_iso();
std::cout << pluss::note::print(labels) << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["2024-11-01T09:00:00.000000000+00:00", "NaT", "2024-12-31T23:59:59.000000000+00:00", "NaT"]
```

Example 3. Compare the canonical ISO path with the more presentation-oriented
formatting path.

```cpp
auto canonical = base.to_note_iso();
auto display = base.strftime("%Y-%m-%d %H:%M");
std::cout << pluss::note::print(canonical) << "\n";
std::cout << pluss::note::print(display) << "\n";
```

### Output

Canonical ISO output:

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["2024-11-01T09:00:00.000000000+00:00", "2024-11-01T10:15:30.000000000+00:00", "2024-12-31T23:59:59.000000000+00:00", "2025-01-01T00:00:00.000000000+00:00"]
```

Display-oriented custom output:

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["2024-11-01 09:00", "2024-11-01 10:15", "2024-12-31 23:59", "2025-01-01 00:00"]
```

## See Also

- [`Timeline::isoformat`](isoformat.md)
- [`Timeline::strftime`](strftime.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="time.html">
    <span class="label">Previous</span>
    <span class="title">time</span>
  </a>
  <a class="next-link" href="to_timeindex.html">
    <span class="label">Next</span>
    <span class="title">to_timeindex</span>
  </a>
</div>
