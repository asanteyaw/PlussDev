<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c2acd568a7be -->

# Catalog::transform

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog Catalog::transform(const std::function<Stream(const Stream&)>& fn) const
```

## What It Does

Transforms every stream in the catalog and returns the updated catalog. This is the
whole-catalog root transform surface: one callback, called once per stream, with the
replacement stream returned directly.

```{important} Whole-catalog transform only
Scoped transforms no longer live on the root `Catalog` surface. If you want to
transform only some keys, select them first with [`keys(...)`](keys.md) and then
call `map(...)` on the accessor.
```

## Parameters

**fn** (`const std::function<Stream(const Stream&)>&`)
    Called once per stream; must return a defined `Stream` of the same length as
    its input.

## Returns

`Catalog`
    A new catalog with every stream replaced by `fn`'s result.

## Examples

Example 1. Transform every stream.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k     = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
});

auto out = ct.transform([](const pluss::kindle::Stream& s) {
  if (s.kind() != pluss::kindle::StreamKind::Series) return s;
  return pluss::kindle::Stream(s.as_series() + 1.0);
});
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, metric]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 3       | [2, 3, 4]                                        
metric                   | Series       | Series[float64]  | 3       | [11, 21, 31]                                     
========================================================================================================================
```

## Notes

- Every returned stream must be defined and the same length as its input.
- [`map()`](map.md) is a pure alias for this const-returning method.
- [`map_()`](map_.md) is the in-place sibling when you want to mutate the catalog object
  itself rather than receive a transformed copy.
- For scoped transforms, use [`keys(...)`](keys.md) followed by `map(...)` on the
  accessor.

## See Also

- [`Catalog::map`](map.md): a pure alias for the one-argument overload.
- [`Catalog::map_`](map_.md): the in-place sibling of this transform surface.
- [`Catalog::keys`](keys.md): select one or more keys, then transform only that
  selection with accessor `map(...)`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_string.html">
    <span class="label">Previous</span>
    <span class="title">to_string</span>
  </a>
  <a class="next-link" href="unique.html">
    <span class="label">Next</span>
    <span class="title">unique</span>
  </a>
</div>
