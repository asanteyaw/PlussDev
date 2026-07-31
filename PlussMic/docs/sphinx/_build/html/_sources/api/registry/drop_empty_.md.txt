<!-- pluss-hand-authored -->

# Registry::drop_empty_

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry& Registry::drop_empty_()
```

## What It Does

The in-place counterpart to [`drop_empty()`](drop_empty.md) — removes every
zero-length field from this registry directly, rather than returning a new one.

## Parameters

`None`

## Returns

`Registry&`
    A reference to this registry, with every zero-length field removed.

## Examples

Example 1. Drop an empty field in place.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)})},
    {"empty_field", pluss::bundle::Slot(std::vector<pluss::bundle::Slot::Value>{})},
});

rg.drop_empty_();
for (const auto& key : rg.fields()) std::cout << key << " ";
```

### Output

```text
id
```

## Notes

- Same emptiness definition as [`drop_empty()`](drop_empty.md) — zero values,
  not all-null values.
- Mutates the registry it's called on and returns `*this` by reference.
- Requires a defined registry — throws `"Registry: undefined handle"` on an
  undefined one.

## See Also

- [`Registry::drop_empty`](drop_empty.md): the non-mutating counterpart.
- [`Registry::dropnull`](dropnull.md): the value-level counterpart — drops null
  values within fields, rather than zero-length fields entirely.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="drop_empty.html">
    <span class="label">Previous</span>
    <span class="title">drop_empty</span>
  </a>
  <a class="next-link" href="dropnull.html">
    <span class="label">Next</span>
    <span class="title">dropnull</span>
  </a>
</div>
