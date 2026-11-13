<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f3ec1d1f2dda -->

# Catalog::display

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
void Catalog::display() const
```

## What It Does

Renders the catalog's ASCII-table view to `std::cout` — a convenience wrapper equivalent
to `show(std::cout)`.

```{important} Identical to info(), show(), and operator<< — no separate metadata view
`display()`, [`info()`](info.md), [`show()`](show.md) (when passed `std::cout`), and
`operator<<` all render byte-identical output. There is no metadata-only summary view for
`Catalog` the way `Slate::info()` provides a name/kind/dtype/null-count summary distinct
from `Slate::show()`'s row rendering — pick whichever of these four names reads best at
the call site; the result is the same either way.
```

## Parameters

`None`

## Returns

`void`
    Writes to `std::cout` as a side effect; produces no return value.

## Examples

Example 1. Render a catalog to the console.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});

ct.display();
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

- Equivalent to `show(std::cout)` — see the important callout above for how this relates
  to `info()`/`show()`/`operator<<`.
- Always writes to `std::cout`; use [`show(os)`](show.md) directly for any other stream.

## See Also

- [`Catalog::info`](info.md) / [`Catalog::show`](show.md): equivalent renders — `info()`
  also targets `std::cout`; `show()` takes an explicit stream.
- [`Catalog::to_string`](to_string.md): the same rendering, returned as a `std::string`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="defined.html">
    <span class="label">Previous</span>
    <span class="title">defined</span>
  </a>
  <a class="next-link" href="distinct.html">
    <span class="label">Next</span>
    <span class="title">distinct</span>
  </a>
</div>
