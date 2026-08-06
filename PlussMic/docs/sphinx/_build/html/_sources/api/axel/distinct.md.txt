<!-- pluss-hand-authored -->

# Axel::distinct

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel Axel::distinct(pluss::kindle::DistinctOptions options = {}) const
```

## What It Does

Returns a new axel with each field's values deduplicated, in every
registry frame, applying
[`Registry::distinct(options)`](../registry/distinct.md) to every frame
via [`map()`](map.md).

```{important} Can produce a ragged axel
Like [`dropnull()`](dropnull.md), deduplication happens per field,
independently, within every frame — fields with different numbers of
duplicates end up with different lengths. A previously-aligned axel can
come out ragged.
```

## Parameters

**options** (`pluss::kindle::DistinctOptions`, optional)
    `{bool dropnull = true}` — whether null values are dropped before
    deduplication. Defaults to dropping them.

## Returns

`Axel`
    A new axel with each field's values deduplicated, preserving first
    occurrence order, in every frame.

## Examples

Example 1. A field with a repeated value is reduced to its unique
values.

```cpp
auto r0 = pluss::bundle::Registry({{"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(1), pluss::make_series(2)})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.distinct();
std::cout << out.registry(0).fields("id").size();
```

### Output

```text
2
```

## Notes

- Per-field, independent deduplication — see the important callout
  above for the ragged-axel consequence.
- Full per-value equality/hashing semantics are documented on
  [`Slot::distinct`](../slot/distinct.md).
- Never mutates the receiver — always returns a new `Axel`.

## See Also

- [`Axel::ndistinct`](ndistinct.md): the distinct-value **count**,
  rather than the values themselves.
- [`Axel::freq`](freq.md): per-value frequency counts, a related
  summary.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="depth.html">
    <span class="label">Previous</span>
    <span class="title">depth</span>
  </a>
  <a class="next-link" href="drop.html">
    <span class="label">Next</span>
    <span class="title">drop</span>
  </a>
</div>
