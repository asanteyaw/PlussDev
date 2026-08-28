# Timeline::isoformat

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
Timeline::isoformat() -> std::string
```

Formats one scalar timestamp into canonical ISO-style text.

This is the scalar convenience surface for the common question:
"give me the machine-readable text for this one moment".

Unlike [`strftime(...)`](strftime.md), `isoformat()` does not ask you to supply
a format pattern. Unlike [`to_note_iso()`](to_note_iso.md), it is not the
vectorized text-export surface for a full timeline.

## Parameters

This method takes no parameters.

The main usage choice is about the shape of the source object:

- use `isoformat()` for a scalar `Timeline`
- use [`to_note_iso()`](to_note_iso.md) for 1D or higher-rank timelines

## Returns

`std::string`
    Canonical ISO-style timestamp text with explicit offset and fractional
    second precision.

## Notes

- `isoformat()` is the right surface for logs, keys, diagnostics, and scalar
  serialization.
- The output is stable and explicit: the verified example includes full
  nanosecond precision and a `+00:00` offset.
- If you are formatting many timestamps at once, `to_note_iso()` is usually
  the more natural API.

## Examples

Example 1. Format one scalar timestamp into canonical text.

```cpp
auto text = scalar.isoformat();
std::cout << text << "\n";
```

### Output

```text
2024-11-01T09:00:00.000000000+00:00
```

Example 2. When the source is not scalar, switch to the vectorized ISO
conversion surface instead of forcing `isoformat()`.

```cpp
auto labels = with_nat.to_note_iso();
std::cout << pluss::note::print(labels) << "\n";
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["2024-11-01T09:00:00.000000000+00:00", "NaT", "2024-12-31T23:59:59.000000000+00:00", "NaT"]
```

## See Also

- [`Timeline::to_note_iso`](to_note_iso.md)
- [`Timeline::strftime`](strftime.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="argsort.html">
    <span class="label">Previous</span>
    <span class="title">argsort</span>
  </a>
  <a class="next-link" href="strftime.html">
    <span class="label">Next</span>
    <span class="title">strftime</span>
  </a>
</div>
