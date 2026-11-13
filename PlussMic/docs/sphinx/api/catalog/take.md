<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6a3c57cc34ac -->

# Catalog::take

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog Catalog::take(std::span<const std::int64_t> indices) const
```

## What It Does

Selects rows by arbitrary integer position — not necessarily contiguous, not necessarily
in ascending order, and with repeats allowed — from every stream in the catalog. Each
stream is independently re-indexed by `indices` via its own `take()`.

## Parameters

**indices** (`std::span<const std::int64_t>`)
    The row positions to select, in the order they should appear in the result. May repeat
    a position or list positions out of order.

## Returns

`Catalog`
    A new catalog with every stream re-indexed to just the requested positions, in the
    order given.

## Examples

Example 1. Select a non-contiguous set of rows.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi", "Ho", "Cape Coast"})),
});

std::vector<std::int64_t> picks{0, 2, 4};
auto out = ct.take(picks);
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
id                       | Series       | Series[float64]  | 3       | [1, 3, 5]                                        
city                     | Note         | Note             | 3       | [Accra, Kumasi, Cape Coast]                      
========================================================================================================================
```

## Notes

- Requires a defined catalog; throws `"Catalog: undefined handle"` otherwise.
- Positions may repeat or appear out of order — the result follows `indices`' own order,
  not the source's.
- Applies independently per stream, same as [`head()`](head.md)/[`tail()`](tail.md)/
  [`slice()`](slice.md) — works even on a ragged catalog.
- [`sample()`](sample.md) is built on `take()` internally, applied to a shuffled subset of
  row positions.

## See Also

- [`Catalog::sample`](sample.md): random row selection, built on this same mechanism.
- [`Catalog::slice`](slice.md): contiguous row selection, when you don't need arbitrary
  positions.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tail.html">
    <span class="label">Previous</span>
    <span class="title">tail</span>
  </a>
  <a class="next-link" href="clean.html">
    <span class="label">Next</span>
    <span class="title">clean</span>
  </a>
</div>
