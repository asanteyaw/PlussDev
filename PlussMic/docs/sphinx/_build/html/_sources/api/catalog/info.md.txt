<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2f7777d2e098 -->

# Catalog::info

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::info() const -> void
```

## What It Does

Renders the catalog's ASCII-table view to `std::cout`.

```{important} No separate metadata summary — identical to display()/show()/operator<<
Despite the name suggesting a metadata-only summary (the way `Thread::info()` prints a
name/kind/dtype/null-count block distinct from `Thread::show()`'s row rendering),
`Catalog::info()` renders **exactly** the same full row-and-column table as
[`display()`](display.md), [`show()`](show.md), and `operator<<`. There is currently no
lighter-weight metadata view for `Catalog` — if you only want counts, use
[`nkeys()`](nkeys.md)/[`nrecords()`](nrecords.md)/[`shape()`](shape.md) directly instead of
`info()`.
```

## Parameters

`None`

## Returns

`void`
    Writes to `std::cout` as a side effect; produces no return value.

## Examples

Example 1. `info()` renders the full table, not a metadata summary.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});

ct.info();
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 3       | [1, 2, 3]                                        
========================================================================================================================
```

## Notes

- Renders identically to [`display()`](display.md)/[`show()`](show.md)/`operator<<` — see
  the important callout above.
- For counts alone (row count, stream count) without the full table, use
  [`nkeys()`](nkeys.md), [`nrecords()`](nrecords.md), or [`shape()`](shape.md) directly.

## See Also

- [`Catalog::display`](display.md) / [`Catalog::show`](show.md): equivalent renders.
- [`Catalog::shape`](shape.md): a lightweight `(rows, streams)` summary, if that's what
  you actually want instead of the full rendered table.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="head.html">
    <span class="label">Previous</span>
    <span class="title">head</span>
  </a>
  <a class="next-link" href="invalid.html">
    <span class="label">Next</span>
    <span class="title">invalid</span>
  </a>
</div>
