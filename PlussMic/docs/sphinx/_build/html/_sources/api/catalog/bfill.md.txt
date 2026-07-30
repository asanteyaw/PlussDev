<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 09fde4225279 -->

# Catalog::bfill

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::bfill() const -> Catalog
```

## What It Does

Backward-fills `NULL` cells in **every** stream — each stream's own `bfill()`, applied
independently, regardless of payload kind. The mirror image of [`ffill()`](ffill.md):
instead of carrying the most recent preceding valid value forward, it pulls the next
following valid value backward.

## Parameters

`None`

## Returns

`Catalog`
    A new catalog with every stream's `NULL` cells backward-filled.

## Examples

Example 1. Backward-fill across a mixed numeric/text catalog.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}),
        pluss::make_series(std::vector<bool>{true, false, true})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}),
        pluss::make_series(std::vector<bool>{true, true, false})),
});

auto out = ct.bfill();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, city]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 3       | [1, 3, 3]                                        
city                     | Note         | Note             | 3       | [Accra, Tema, NA]                                
========================================================================================================================
```

Row `1`'s `NULL` in `id` becomes `3` (pulled backward from row `2`). Row `2`'s `NULL` in
`city` stays `NULL` — there's no valid value *after* it to pull backward.

## Notes

- Unlike [`fillnull()`](fillnull.md), `bfill()` has no kind-matching restriction — every
  stream is filled, regardless of payload kind.
- A trailing `NULL` with nothing valid after it in that stream stays `NULL`.
- Requires a defined catalog; throws `"Catalog: undefined handle"` otherwise.

## See Also

- [`Catalog::ffill`](ffill.md): the mirror operation, filling from the previous valid
  value instead of the next one.
- [`Catalog::fillnull`](fillnull.md): fill with a fixed scalar instead of a neighboring
  value.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="begin.html">
    <span class="label">Previous</span>
    <span class="title">begin</span>
  </a>
  <a class="next-link" href="cast.html">
    <span class="label">Next</span>
    <span class="title">cast</span>
  </a>
</div>
