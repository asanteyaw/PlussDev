<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 37816f8e6c23 -->

# Axel::ndistinct

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel Axel::ndistinct(pluss::kindle::DistinctOptions options = {}) const
```

## What It Does

Returns a new axel where each field, in every frame, is replaced by a
single value — the count of distinct values that field held — applying
[`Registry::ndistinct(options)`](../registry/ndistinct.md) to every
frame via [`map()`](map.md).

## Parameters

**options** (`pluss::kindle::DistinctOptions`, optional)
    `{bool dropnull = true}` — whether null values are excluded before
    counting distinct values. Defaults to excluding them.

## Returns

`Axel`
    A new axel where each field holds a single distinct-value count, in
    every frame.

## Examples

Example 1. A field with values `[1, 1, 2]` has 2 distinct values.

```cpp
auto r0 = pluss::scribble::Registry({{"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(1), pluss::make_series(2)})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.ndistinct();
std::cout << std::get<pluss::Series>(out.registry(0).fields("id").at(0)).host<std::int64_t>()[0];
```

### Output

```text
2
```

## Notes

- Every field, in every frame, collapses to a single-value result —
  this reshapes each field's length to `1`, not just its content.
- Never mutates the receiver — always returns a new `Axel`.

## See Also

- [`Axel::distinct`](distinct.md): the distinct values themselves,
  rather than just their count.
- [`Axel::freq`](freq.md): per-value frequency counts, a related
  summary.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="map.html">
    <span class="label">Previous</span>
    <span class="title">map</span>
  </a>
  <a class="next-link" href="notnull.html">
    <span class="label">Next</span>
    <span class="title">notnull</span>
  </a>
</div>
