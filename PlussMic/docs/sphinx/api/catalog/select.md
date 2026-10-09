<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 684340c92453 -->

# Catalog::select

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
static Catalog Catalog::select(
    pluss::msharp::RangeView::Of<pluss::table::Slate> conditions,
    pluss::msharp::RangeView::Of<Catalog> choices,
    const Catalog& default_value)

static Catalog Catalog::select(
    std::initializer_list<pluss::table::Slate> conditions,
    std::initializer_list<Catalog> choices,
    const Catalog& default_value)
```

## What It Does

Chooses row-wise between whole catalogs, using boolean `Slate` masks as the conditions.

Every choice catalog and the default catalog must have the same keys in the same order.
For each key, `Catalog::select(...)` delegates to [`Stream::select`](../stream/select.md).

To keep only named keys, do **not** use this method anymore. Project them with
[`keys(...)`](keys.md).

## Notes

- `conditions.size()` must equal `choices.size()`.
- Every choice catalog and the default catalog must be defined.
- Every choice catalog must match the default catalog's key set and key order exactly.

## See Also

- [`Stream::select`](../stream/select.md): the lower-level stream chooser this delegates
  to.
- [`Catalog::keys`](keys.md): projection by key selection.
- [`Catalog::drop`](drop.md): remove named streams from one catalog.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_txt.html">
    <span class="label">Previous</span>
    <span class="title">save_txt</span>
  </a>
  <a class="next-link" href="set.html">
    <span class="label">Next</span>
    <span class="title">set</span>
  </a>
</div>
