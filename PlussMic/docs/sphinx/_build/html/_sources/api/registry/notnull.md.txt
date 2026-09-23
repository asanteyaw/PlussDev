<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2b5b356ffe3c -->

# Registry::notnull

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::notnull() const
```

## What It Does

The logical complement of [`isnull()`](isnull.md) — returns a new registry where
every field's values are replaced by a boolean flag for whether each value is
**not** null, applying [`Slot::notnull()`](../slot/notnull.md) to every field via
[`map()`](map.md). Works uniformly across every field regardless of its own value
kind.

## Parameters

`None`

## Returns

`Registry`
    A new registry with every field's values replaced by boolean not-null flags,
    one per original value.

## Examples

Example 1. A field with one null value among numeric values.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::scribble::Slot::Value{std::monostate{}}, pluss::make_series(3)})},
});

auto out = rg.notnull();
std::cout << out.fields("id").to_json();
```

### Output

```text
{
  "_type": "Slot",
  "defined": true,
  "size": 3,
  "values": [
    "Series[bool]",
    "Series[bool]",
    "Series[bool]"
  ]
}
```

## Notes

- `notnull()` is exactly `isnull()` with every flag inverted.
- Never throws on a mismatched field kind — unlike [`fillnull()`](fillnull.md).
- Full per-value semantics documented on [`Slot::notnull`](../slot/notnull.md).
- Never mutates the receiver — always returns a new `Registry`.

## See Also

- [`Registry::isnull`](isnull.md): the logical complement.
- [`Registry::invalid`](invalid.md): a related but distinct check.
- [`Catalog::notnull`](../catalog/notnull.md): the equivalent operation on the
  kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nfields.html">
    <span class="label">Previous</span>
    <span class="title">nfields</span>
  </a>
  <a class="next-link" href="nstate.html">
    <span class="label">Next</span>
    <span class="title">nstate</span>
  </a>
</div>
