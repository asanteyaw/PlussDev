<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1be5cf8689a7 -->

# Catalog::ffill

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog Catalog::ffill() const
```

## What It Does

Forward-fills `NULL` cells in **every** stream — each stream's own `ffill()`, applied
independently, regardless of payload kind. Unlike [`fillnull()`](fillnull.md), there's no
kind-matching restriction here: every stream, numeric or text or temporal, gets its own
`NULL`s replaced with the most recent preceding valid value.

## Parameters

`None`

## Returns

`Catalog`
    A new catalog with every stream's `NULL` cells forward-filled.

## Examples

Example 1. Forward-fill across a mixed numeric/text catalog — both streams are filled,
regardless of kind.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}),
        pluss::make_series(std::vector<bool>{true, false, true})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}),
        pluss::make_series(std::vector<bool>{true, true, false})),
});

auto out = ct.ffill();
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
id                       | Series       | Series[float64]  | 3       | [1, 1, 3]                                        
city                     | Note         | Note             | 3       | [Accra, Tema, Tema]                              
========================================================================================================================
```

Row `1`'s `NULL` in `id` becomes `1` (carried forward from row `0`); row `2`'s `NULL` in
`city` becomes `"Tema"` (carried forward from row `1`) — each stream fills independently
from its own preceding values.

## Notes

- Unlike [`fillnull()`](fillnull.md), `ffill()` has no kind-matching restriction — every
  stream is filled, regardless of payload kind.
- A leading `NULL` with nothing valid before it in that stream stays `NULL` — there's
  nothing to carry forward.
- Requires a defined catalog; throws `"Catalog: undefined handle"` otherwise.

## See Also

- [`Catalog::bfill`](bfill.md): the mirror operation, filling from the next valid value
  instead of the previous one.
- [`Catalog::fillnull`](fillnull.md): fill with a fixed scalar instead of a neighboring
  value.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="keywords.html">
    <span class="label">Previous</span>
    <span class="title">keywords</span>
  </a>
  <a class="next-link" href="fillnull.html">
    <span class="label">Next</span>
    <span class="title">fillnull</span>
  </a>
</div>
