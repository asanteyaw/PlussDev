<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b8f5e4667b89 -->

# Catalog::std

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
Catalog Catalog::std() const
Catalog Catalog::std(pluss::kindle::ReduceAxis axis) const
Catalog Catalog::std(const pluss::kindle::CatalogReduceOptions& options) const
```

## What It Does

Computes standard deviation either per stream (`ReduceAxis::Record`) or across the
selected numeric keys row-by-row (`ReduceAxis::Key`).

## Options

`CatalogReduceOptions::unbiased` controls whether sample (`ddof = 1`) or population
(`ddof = 0`) standard deviation is used.

## See Also

- [`Catalog::var`](var.md)
- [`Catalog::mean`](mean.md)

