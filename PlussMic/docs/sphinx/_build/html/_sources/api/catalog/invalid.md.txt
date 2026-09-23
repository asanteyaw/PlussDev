<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e26049f9eb49 -->

# Catalog::invalid

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::invalid() const -> Catalog
```

## What It Does

Reports per-cell validity across every stream — each stream's own `invalid()`, applied
independently, returning a `Bool`-dtype result stream per key. `invalid()` and
[`isnull()`](isnull.md) call different underlying methods on each stream (`invalid()` vs.
`isnull()`), but in ordinary use — including a `NULL`-marked cell or an unmarked `NaN` in a
numeric stream — they report the same rows as invalid.

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

Example 2. An unmarked `NaN` in a numeric stream (no explicit validity mask at all) is
still reported as invalid.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, std::nan(""), 3.0})),
});

auto out = ct.invalid();
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
id                       | Series       | Series[bool]     | 3       | [false, true, false]                             
========================================================================================================================
```

## Notes

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
  <a class="next-link" href="isnull.html">
    <span class="label">Next</span>
    <span class="title">isnull</span>
  </a>
</div>
