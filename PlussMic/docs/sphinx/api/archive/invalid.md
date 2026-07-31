<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6fc47d1b76d4 -->

# Archive::invalid

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::invalid() const -> Archive
```

## What It Does

Returns a new archive where every catalog is replaced by its own boolean
[`Catalog::invalid()`](../catalog/invalid.md) result — the same shape and, in ordinary use,
the same result as [`isnull()`](isnull.md). Works uniformly across all payload kinds;
delegates to each catalog independently.

## Parameters

`None`

## Returns

`Archive`
    A new archive, same depth and depth labels as the receiver, with every stream in every
    catalog replaced by its own boolean `invalid()` result.

## Examples

Example 1. `invalid()` reports the same rows as `isnull()` for a mask-based `NULL`.

```cpp
using namespace pluss::kindle::literals;

auto mask_id = pluss::make_series(std::vector<bool>{true, false, true});
auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}), mask_id),
});
auto archive = pluss::kindle::Archive({c0});

auto out = archive.invalid();
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
id                       | Series       | Series[bool]     | 3       | [false, true, false]                             
========================================================================================================================
```

## Notes

- Works on every payload kind — no `Series`-only restriction.
- Every resulting stream is `Series[bool]`, regardless of its original kind.
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise.

## See Also

- [`Archive::isnull`](isnull.md) / [`Archive::notnull`](notnull.md): the closely related
  validity checks this method agrees with in practice.
- [`Catalog::invalid`](../catalog/invalid.md): the single-catalog delegate this method
  applies to every catalog.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="head.html">
    <span class="label">Previous</span>
    <span class="title">head</span>
  </a>
  <a class="next-link" href="isnull.html">
    <span class="label">Next</span>
    <span class="title">isnull</span>
  </a>
</div>
