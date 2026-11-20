<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 32b918499de5 -->

# Catalog::var

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
Catalog Catalog::var() const
Catalog Catalog::var(pluss::kindle::ReduceAxis axis) const
Catalog Catalog::var(const pluss::kindle::CatalogReduceOptions& options) const
```

## What It Does

Computes variance either per stream (`ReduceAxis::Record`) or across the selected numeric
keys row-by-row (`ReduceAxis::Key`).

## Options

`CatalogReduceOptions::unbiased` controls the divisor:

- `true` uses sample variance (`ddof = 1`)
- `false` uses population variance (`ddof = 0`)

## See Also

- [`Catalog::std`](std.md)
- [`Catalog::mean`](mean.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unique.html">
    <span class="label">Previous</span>
    <span class="title">unique</span>
  </a>
  <a class="next-link" href="where.html">
    <span class="label">Next</span>
    <span class="title">where</span>
  </a>
</div>
