<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f6b2df7091a8 -->

# Archive::isnull

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive Archive::isnull() const
```

## What It Does

Returns a new archive where every catalog is replaced by its own boolean
[`Catalog::isnull()`](../catalog/isnull.md) result — each stream becomes a `Series[bool]`
stream marking which rows are `NULL`. Works uniformly across all payload kinds; delegates
to each catalog independently.

## Parameters

`None`

## Returns

`Archive`
    A new archive, same depth and depth labels as the receiver, with every stream in every
    catalog replaced by its own boolean `isnull()` result.

## Examples

Example 1. Check validity across a catalog with a mixed numeric/text stream set.

```cpp
using namespace pluss::kindle::literals;

auto mask_id = pluss::make_series(std::vector<bool>{true, false, true});
auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}), mask_id),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});
auto archive = pluss::kindle::Archive({c0});

auto out = archive.isnull();
std::cout << out.catalog(0);
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
city                     | Series       | Series[bool]     | 3       | [false, false, false]                            
========================================================================================================================
```

## Notes

- Works on every payload kind — no `Series`-only restriction.
- Every resulting stream is `Series[bool]`, regardless of its original kind.
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise.

## See Also

- [`Archive::notnull`](notnull.md): the exact logical complement of this method.
- [`Archive::invalid`](invalid.md): a closely related check, matching in ordinary use.
- [`Catalog::isnull`](../catalog/isnull.md): the single-catalog delegate this method
  applies to every catalog.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="invalid.html">
    <span class="label">Previous</span>
    <span class="title">invalid</span>
  </a>
  <a class="next-link" href="keyaxis.html">
    <span class="label">Next</span>
    <span class="title">keyaxis</span>
  </a>
</div>
