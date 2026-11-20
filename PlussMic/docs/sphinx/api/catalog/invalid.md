<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e26049f9eb49 -->

# Catalog::invalid

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog Catalog::invalid() const
```

## What It Does

Reports, for every stream, which records are not ordinary numbers — each stream's own
`invalid()`, applied independently, returning a `Bool`-dtype result stream per key. A record
is invalid when it is missing (`NULL`), a `NaN`, or an infinity: `invalid()` is the union of
[`isnull()`](isnull.md), [`isnan()`](isnan.md) and [`isinf()`](isinf.md). It therefore
agrees with `isnull()` for a missing record and **also** flags a `NaN`, which `isnull()` does
not, because a `NaN` is a value and not a missing record.

## Parameters

`None`

## Returns

`Catalog`
    A new catalog, same keys and row count as `*this`, with every stream replaced by its
    own boolean `invalid()` result.

## Examples

Example 1. Check validity across a mixed numeric/text catalog.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}),
        pluss::make_series(std::vector<bool>{true, false, true})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}),
        pluss::make_series(std::vector<bool>{true, true, false})),
});

auto out = ct.invalid();
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
id                       | Series       | Series[bool]     | 3       | [false, true, false]                             
city                     | Series       | Series[bool]     | 3       | [false, false, true]                             
========================================================================================================================
```

Example 2. A `NaN` in a numeric stream (no validity mask at all) is reported as invalid,
even though it is not missing: `isnull()` is `false` for the same stream.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, std::nan(""), 3.0})),
});

auto out = ct.invalid();
std::cout << out;
std::cout << ct.isnull();
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[bool]     | 3       | [false, true, false]                             
========================================================================================================================
========================================================================================================================
kindle::Catalog  keys=1
keys: [id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[bool]     | 3       | [false, false, false]                            
========================================================================================================================
```

## Notes

- `invalid` is the union of the three kinds: `true` for `NULL` ([`isnull`](isnull.md)), `NaN` ([`isnan`](isnan.md)) and both infinities ([`isinf`](isinf.md)). Its partners are [`dropinvalid`](dropinvalid.md) and [`fillinvalid`](fillinvalid.md). See [Missing, NaN and infinity](../../guide/missing_nan_and_inf.md).
- Every resulting stream is `Series[bool]`, regardless of the source stream's own payload
  kind.
- Requires a defined catalog; throws `"Catalog: undefined handle"` otherwise.

## See Also

- [`Catalog::isnull`](isnull.md) / [`Catalog::notnull`](notnull.md): the closely related
  per-cell validity checks.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="info.html">
    <span class="label">Previous</span>
    <span class="title">info</span>
  </a>
  <a class="next-link" href="isinf.html">
    <span class="label">Next</span>
    <span class="title">isinf</span>
  </a>
</div>
