<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6e54f34f1e29 -->

# Catalog::mean

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
Catalog::mean() const -> Catalog
Catalog::mean(pluss::kindle::ReduceAxis axis) const -> Catalog
Catalog::mean(const pluss::kindle::CatalogReduceOptions& options) const -> Catalog
```

## What It Does

Computes arithmetic means either per stream (`ReduceAxis::Record`) or across the selected
keys row-by-row (`ReduceAxis::Key`).

The default is `ReduceAxis::Record`, which yields one scalar stream per key.

## Notes

- Cross-key reduction requires `Series` streams.
- `skipnull`, `align`, `align_by`, and `fill_scalar` are controlled by
  `CatalogReduceOptions`.

## See Also

- [`Catalog::sum`](sum.md)
- [`Catalog::var`](var.md)
- [`Catalog::std`](std.md)

