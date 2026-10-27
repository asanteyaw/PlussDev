<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: eaa9360d9b2d -->

# Registry::distinct

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::distinct(pluss::kindle::DistinctOptions options = {}) const
```

## What It Does

Returns a new registry with each field's values deduplicated, applying
[`Slot::distinct(options)`](../slot/distinct.md) to every field independently via
[`map()`](map.md).

```{important} Can produce a ragged registry
Like [`dropnull()`](dropnull.md), deduplication happens per field, independently
— fields with different numbers of duplicates end up with different lengths. A
previously-aligned registry can come out ragged.
```

## Parameters

**options** (`pluss::kindle::DistinctOptions`, optional)
    `{bool dropnull = true}` — whether null values are dropped before
    deduplication. Defaults to dropping them.

## Returns

`Registry`
    A new registry with each field's values deduplicated, preserving first
    occurrence order.

## Examples

Example 1. A field with a repeated value is reduced to its unique values.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(1), pluss::make_series(2)})},
});

auto out = rg.distinct();
auto ids = out.fields("id");
std::cout << "[";
for (std::int64_t i = 0; i < ids.size(); ++i) {
  if (i) std::cout << ", ";
  std::cout << std::get<pluss::Series>(ids.at(i)).host<std::int64_t>()[0];
}
std::cout << "]";
```

### Output

```text
[1, 2]
```

## Notes

- Per-field, independent deduplication — see the important callout above for the
  ragged-registry consequence.
- Full per-value equality/hashing semantics are documented on
  [`Slot::distinct`](../slot/distinct.md).
- Never mutates the receiver — always returns a new `Registry`.

## See Also

- [`Registry::ndistinct`](ndistinct.md): the distinct-value **count**, rather
  than the values themselves.
- [`Registry::freq`](freq.md): per-value frequency counts, a related summary.
- [`Catalog::distinct`](../catalog/distinct.md): the equivalent operation on the
  kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="defined.html">
    <span class="label">Previous</span>
    <span class="title">defined</span>
  </a>
  <a class="next-link" href="drop.html">
    <span class="label">Next</span>
    <span class="title">drop</span>
  </a>
</div>
