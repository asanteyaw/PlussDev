<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9cced71ef9c5 -->

# Archive::astype

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive Archive::astype(pluss::codem::ScalarType dtype) const
```

## What It Does

Shorthand for [`cast(SlateCastTarget::Series, {.series_dtype = dtype})`](cast.md) —
casts every stream in every catalog to `Series`, at the requested numeric dtype, in one
call. Delegates to each catalog's own [`Catalog::astype()`](../catalog/astype.md).

## Parameters

**dtype** (`pluss::codem::ScalarType`)
    The numeric scalar dtype to cast every stream to (e.g. `Int64`, `Float64`).

## Returns

`Archive`
    A new archive, same depth and depth labels as the receiver, with every stream in every
    catalog cast to `Series` at `dtype`.

## Examples

Example 1. Cast every stream to `Series[int64]`.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({ "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})) });
auto archive = pluss::kindle::Archive({c0});

auto out = archive.astype(pluss::codem::ScalarType::Int64);
std::cout << out.catalog(0);
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

- Applies to every stream in every catalog, unconditionally — same reach as
  [`cast()`](cast.md).
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise.

## See Also

- [`Archive::cast`](cast.md): the more general form this method is shorthand for.
- [`Catalog::astype`](../catalog/astype.md): the single-catalog delegate this method
  applies to every catalog.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="aggregate_over_catalogs.html">
    <span class="label">Previous</span>
    <span class="title">aggregate_over_catalogs</span>
  </a>
  <a class="next-link" href="bfill.html">
    <span class="label">Next</span>
    <span class="title">bfill</span>
  </a>
</div>
