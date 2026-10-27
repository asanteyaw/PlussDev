<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 15f3924c4a03 -->

# Registry::ndistinct

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
Registry Registry::ndistinct(pluss::kindle::DistinctOptions options = {}) const
```

## What It Does

Returns a new registry where each field is collapsed to a **single value**: the
count of that field's distinct values, applying
[`Slot::ndistinct(options)`](../slot/ndistinct.md) to every field independently
via [`map()`](map.md).

## Parameters

**options** (`pluss::kindle::DistinctOptions`, optional)
    `{bool dropnull = true}` — whether null values are dropped before counting
    distinct values. Defaults to dropping them.

## Returns

`Registry`
    A new registry where each field's `Slot` has exactly one value: the distinct
    count.

## Examples

Example 1. A field with one repeated value among three has two distinct values.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(1), pluss::make_series(2)})},
});

auto out = rg.ndistinct();
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
[2]
```

## Notes

- Unlike most `Registry` transforms, this always collapses every field to
  `size() == 1` — a registry that was ragged before `ndistinct()` becomes
  aligned (`nstate() == 1`) afterward.
- Full semantics documented on [`Slot::ndistinct`](../slot/ndistinct.md).
- Never mutates the receiver — always returns a new `Registry`.

## See Also

- [`Registry::distinct`](distinct.md): the actual distinct values, rather than
  just their count.
- [`Registry::freq`](freq.md): per-value frequency counts.
- [`Catalog::ndistinct`](../catalog/ndistinct.md): the equivalent operation on
  the kindle model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="morph_.html">
    <span class="label">Previous</span>
    <span class="title">morph_</span>
  </a>
  <a class="next-link" href="nfields.html">
    <span class="label">Next</span>
    <span class="title">nfields</span>
  </a>
</div>
