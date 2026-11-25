<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 80b33611ad10 -->

# Catalog::max

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
Catalog Catalog::max() const
Catalog Catalog::max(pluss::kindle::ReduceAxis axis) const
Catalog Catalog::max(const pluss::kindle::CatalogReduceOptions& options) const
```

## What It Does

Returns maxima either per stream (`ReduceAxis::Record`) or across the selected numeric
keys row-by-row (`ReduceAxis::Key`).

## Notes

- The default is per-stream reduction.
- Cross-key reduction requires `Series` streams.

## See Also

- [`Catalog::min`](min.md)
- [`Catalog::sum`](sum.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="match_keys.html">
    <span class="label">Previous</span>
    <span class="title">match_keys</span>
  </a>
  <a class="next-link" href="maxentries.html">
    <span class="label">Next</span>
    <span class="title">maxentries</span>
  </a>
</div>
