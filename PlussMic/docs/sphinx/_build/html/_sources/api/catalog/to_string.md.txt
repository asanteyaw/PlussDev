<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: adfc335854d0 -->

# Catalog::to_string

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::to_string() const -> std::string
```

## What It Does

Renders the catalog's ASCII-table view as a `std::string` — the same rendering
`operator<<`, [`display()`](display.md), [`info()`](info.md), and [`show()`](show.md) all
write to a stream; `to_string()` returns it instead.

## Parameters

`None`

## Returns

`std::string`
    The rendered catalog table.

## Examples

Example 1. Render a catalog to a string.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});

auto out = ct.to_string();
std::cout << out;
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

- Same rendering as `operator<<`/[`display()`](display.md)/[`info()`](info.md)/
  [`show()`](show.md) — see [`display()`](display.md)'s important callout for how these
  all relate — but not quite byte-identical: `show()`/`operator<<` append a trailing
  newline that `to_string()` does not have. Verified directly: `to_string()` is one byte
  shorter than an `ostringstream` populated via `show()` for the same catalog.
- A distinct text representation from [`to_json()`](to_json.md), which returns a
  structural JSON document rather than this plain-text table.

## See Also

- [`Catalog::show`](show.md): the same rendering, written to a stream instead of
  returned as a string, with a trailing newline this method omits.
- [`Catalog::to_json`](to_json.md): a structural JSON dump of the same catalog, distinct
  from this plain-text rendering.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_json.html">
    <span class="label">Previous</span>
    <span class="title">to_json</span>
  </a>
  <a class="next-link" href="transform.html">
    <span class="label">Next</span>
    <span class="title">transform</span>
  </a>
</div>
