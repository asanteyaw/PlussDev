<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: adf6d66a3fc0 -->

# Registry::maxstate

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
std::int64_t Registry::maxstate() const
```

## What It Does

Returns the **longest** field's length in the registry — never throws, even on a
ragged registry (fields of differing lengths), unlike [`nstate()`](nstate.md). This
is the same value [`shape()`](shape.md) reports as its row count.

## Parameters

`None`

## Returns

`std::int64_t`
    The longest field's value count; `0` for an empty registry.

## Examples

Example 1. A ragged registry reports its longest field's length.

```cpp
auto rg = pluss::bundle::Registry({
    {"a", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"b", pluss::bundle::Slot({pluss::make_series(10), pluss::make_series(20)})},
});

std::cout << rg.maxstate();
```

### Output

```text
3
```

Example 2. An explicitly empty registry reports `0`.

```cpp
pluss::bundle::Registry rg(std::vector<std::pair<std::string, pluss::bundle::Slot>>{});
std::cout << rg.maxstate();
```

### Output

```text
0
```

## Notes

- Never throws, regardless of field-length agreement — contrast with
  [`nstate()`](nstate.md), which requires it.
- [`shape()`](shape.md) is built directly from this method: `{maxstate(), nfields()}`.
- Requires a defined registry — throws `"Registry: undefined handle"` on an
  undefined one.

## See Also

- [`Registry::minstate`](minstate.md): the shortest-field counterpart.
- [`Registry::nstate`](nstate.md): the strict, throwing form that requires every
  field to share this same length.
- [`Registry::shape`](shape.md): pairs this value with the field count.
- [`Catalog::maxrows`](../catalog/maxrows.md): the equivalent check on the
  kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="map_.html">
    <span class="label">Previous</span>
    <span class="title">map_</span>
  </a>
  <a class="next-link" href="minstate.html">
    <span class="label">Next</span>
    <span class="title">minstate</span>
  </a>
</div>
