<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8b862f0dccda -->

# Catalog::astype

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog Catalog::astype(pluss::codem::ScalarType dtype) const
```

## What It Does

A single-argument shorthand for [`cast()`](cast.md), applied across every stream:
`astype(dtype)` is exactly `cast(SlateCastTarget::Series, {.series_dtype = dtype})`.
Everything documented on `cast()`'s page — including the important callout that it applies
uniformly to every stream, not just the ones that obviously need it — applies here too.

## Parameters

**dtype** (`pluss::codem::ScalarType`)
    The target scalar dtype for every stream's numeric conversion.

## Returns

`Catalog`
    A new catalog with every stream cast to `Series` at the requested `dtype`.

## Examples

Example 1. Parse `Note`-kind numeric text into `Int64`.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_note({"1", "2", "3"})),
});

auto out = ct.astype(pluss::codem::ScalarType::Int64);
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
id                       | Series       | Series[int64]    | 3       | [1, 2, 3]                                        
========================================================================================================================
```

## Notes

- A thin shorthand for [`cast()`](cast.md) — see that page for the full conversion rules
  and the important callout about applying to every stream unconditionally.
- Always targets `Series` — use [`cast()`](cast.md) directly for `Note`/`Timeline`/
  `TimeIndex` targets.

## See Also

- [`Catalog::cast`](cast.md): the full, target-configurable casting method this is a
  shorthand for.
- [`Slate::astype`](../slate/astype.md): the underlying single-stream conversion this
  method applies uniformly.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="map_.html">
    <span class="label">Previous</span>
    <span class="title">map_</span>
  </a>
  <a class="next-link" href="at.html">
    <span class="label">Next</span>
    <span class="title">at</span>
  </a>
</div>
