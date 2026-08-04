<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 86826e14ef4e -->

# Registry::show

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
void Registry::show(std::ostream& os) const
```

## What It Does

Writes [`to_string()`](to_string.md)'s output to a caller-provided stream —
identical content to `operator<<` and `to_string()`, just as a side effect on an
explicit `std::ostream&` rather than a returned `std::string`.

## Parameters

**os** (`std::ostream&`)
    The destination stream.

## Returns

`void`
    Writes to `os` as a side effect; produces no return value.

## Examples

Example 1. `show()` writes exactly what `to_string()` returns.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
});

std::ostringstream oss;
rg.show(oss);
std::cout << std::boolalpha << (oss.str() == rg.to_string());
```

### Output

```text
true
```

## Notes

- Identical content to [`to_string()`](to_string.md) and `operator<<` — `show()`
  exists for call sites that already have a target stream in hand.

## See Also

- [`Registry::to_string`](to_string.md): returns the same content as a
  `std::string` instead of writing to a stream.
- [`Catalog::show`](../catalog/show.md): the equivalent method on the kindle
  model.

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
