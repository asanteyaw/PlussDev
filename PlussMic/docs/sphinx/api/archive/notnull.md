<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: aeb5fdb28116 -->

# Archive::notnull

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive Archive::notnull() const
```

## What It Does

Returns a new archive where every catalog is replaced by its own boolean
[`Catalog::notnull()`](../catalog/notnull.md) result — the exact logical complement of
[`isnull()`](isnull.md). Works uniformly across all payload kinds; delegates to each
catalog independently.

## Parameters

`None`

## Returns

`Archive`
    A new archive, same depth and depth labels as the receiver, with every stream in every
    catalog replaced by its own boolean `notnull()` result.

## Examples

Example 1. `notnull()` is the exact inverse of `isnull()`, row for row.

```cpp
using namespace pluss::kindle::literals;

auto mask_id = pluss::make_series(std::vector<bool>{true, false, true});
auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}), mask_id),
});
auto archive = pluss::kindle::Archive({c0});

auto out = archive.notnull();
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
id                       | Series       | Series[bool]     | 3       | [true, false, true]                              
========================================================================================================================
```

## Notes

- Works on every payload kind — no `Series`-only restriction.
- Every resulting stream is `Series[bool]`, regardless of its original kind.
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise.

## See Also

- [`Archive::isnull`](isnull.md): the exact logical complement of this method.
- [`Archive::dropnull`](dropnull.md): removes the rows this method marks `false` for,
  directly.
- [`Catalog::notnull`](../catalog/notnull.md): the single-catalog delegate this method
  applies to every catalog.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nkeys.html">
    <span class="label">Previous</span>
    <span class="title">nkeys</span>
  </a>
  <a class="next-link" href="nrecords.html">
    <span class="label">Next</span>
    <span class="title">nrecords</span>
  </a>
</div>
