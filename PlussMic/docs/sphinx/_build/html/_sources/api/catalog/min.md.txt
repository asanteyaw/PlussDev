<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1dff75dde6d8 -->

# Catalog::min

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
Catalog::min() const -> Catalog
Catalog::min(pluss::kindle::ReduceAxis axis) const -> Catalog
Catalog::min(const pluss::kindle::CatalogReduceOptions& options) const -> Catalog
```

## What It Does

Returns minima either per stream (`ReduceAxis::Record`) or across the selected numeric
keys row-by-row (`ReduceAxis::Key`).

## Notes

- The default is per-stream reduction.
- Cross-key reduction requires `Series` streams.

## See Also

- [`Catalog::max`](max.md)
- [`Catalog::sum`](sum.md)

