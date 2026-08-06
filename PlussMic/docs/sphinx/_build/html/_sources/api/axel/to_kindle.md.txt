<!-- pluss-hand-authored -->

# Axel::to_kindle

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
pluss::kindle::Archive Axel::to_kindle() const
```

## What It Does

Converts this `Axel` to a `pluss::kindle::Archive` — the depth-stacked
`kindle` counterpart to `Axel`, holding one `Catalog` per registry frame
(via each frame's own [`Registry::to_kindle()`](../registry/to_kindle.md))
plus an optional label `Stream` if [`labels()`](labels.md) is engaged.

## Parameters

`None`

## Returns

`pluss::kindle::Archive`
    A new archive with `depth()` catalogs, one per registry frame, in
    the same order.

## Examples

Example 1. Convert a two-frame `Axel` to an `Archive` and check its
depth.

```cpp
auto axel = pluss::bundle::Axel({
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(1)})}}),
    pluss::bundle::Registry({{"ids", pluss::bundle::Slot({pluss::make_series(2)})}}),
});
auto arc = axel.to_kindle();
std::cout << arc.depth();
```

### Output

```text
2
```

## Notes

- Each registry frame's conversion can throw whatever
  `Registry::to_kindle()` throws for that frame (typically over
  slot heterogeneity that isn't stream-compatible) — see
  [`Registry::to_kindle`](../registry/to_kindle.md).
- Requires `defined()` — an undefined handle throws
  `"Axel: undefined handle"`.
- [`as_kindle()`](as_kindle.md) and [`kindelize()`](kindelize.md) are
  byte-identical aliases for this method.

## See Also

- [`Axel::from_kindle`](from_kindle.md): the reverse conversion, static
  factory back to `Axel`.
- [`Axel::as_kindle`](as_kindle.md), [`Axel::kindelize`](kindelize.md):
  the two aliases.
- [`Registry::to_kindle`](../registry/to_kindle.md): the per-frame
  conversion this method calls for each registry.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_json.html">
    <span class="label">Previous</span>
    <span class="title">to_json</span>
  </a>
  <a class="next-link" href="to_string.html">
    <span class="label">Next</span>
    <span class="title">to_string</span>
  </a>
</div>
