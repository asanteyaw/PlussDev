<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 86826e14ef4e -->

# Slot::show

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
void Slot::show(std::ostream& os) const
```

## What It Does

Writes the same rendering [`to_string()`](to_string.md)/
[`to_json()`](to_json.md) produce to `os`, instead of returning it as a
string. What `operator<<` calls internally.

## Parameters

**os** (`std::ostream&`)
    The destination stream.

## Returns

`void`

## Examples

Example 1. Write to a caller-provided stream.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2)});
std::ostringstream oss;
slot.show(oss);
std::cout << std::boolalpha << (oss.str() == slot.to_string());
```

### Output

```text
true
```

## Notes

- Byte-identical output to [`to_string()`](to_string.md)/
  [`to_json()`](to_json.md) — just written to a stream instead of returned.
- `operator<<(std::ostream&, const Slot&)` is implemented as a call to
  `show()`.

## See Also

- [`Slot::to_string`](to_string.md) / [`Slot::to_json`](to_json.md): the
  string-returning equivalents.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shape.html">
    <span class="label">Previous</span>
    <span class="title">shape</span>
  </a>
  <a class="next-link" href="size.html">
    <span class="label">Next</span>
    <span class="title">size</span>
  </a>
</div>
