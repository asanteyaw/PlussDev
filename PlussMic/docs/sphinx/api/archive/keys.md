<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ae1c531eeba0 -->

# Archive::keys

[Back to Archive API Reference](../../archive_api_reference)

## Signatures

```cpp
Archive Archive::keys(std::span<const std::string_view> keys) const
Archive Archive::keys(std::initializer_list<std::string_view> keys) const
```

## What It Does

Projects every catalog in the archive down to the named keys and returns that smaller
archive directly.

```{important} No no-argument overload — unlike Catalog::keys()
[`Catalog::keys()`](../catalog/keys.md) has a no-argument overload returning
`const std::vector<std::string>&` (every stream name). `Archive::keys()` has **no**
equivalent — every overload requires an explicit name list. To list a single catalog's own
key names, go through [`catalog(i)`](catalog.md)`.keys()` instead.
```

```{important} keys(names) is direct archive projection
`archive.keys({"id", "city"})` returns a smaller archive where each catalog contains only
those streams, in that order. Any chained operation then runs on the projected archive.
```

## Parameters

**keys** (`std::span<const std::string_view>` or `std::initializer_list<std::string_view>`)
    One or more stream names to select, applied to every catalog in the archive.

## Returns

`Archive`
    A smaller archive where each catalog contains exactly the requested keys.

## Examples

Example 1. Select a stream by name and then operate on the projected archive.

```cpp
using namespace pluss::kindle::literals;

auto mask0 = pluss::make_series(std::vector<bool>{true, false});
auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0}), mask0),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema"})),
});
auto mask1 = pluss::make_series(std::vector<bool>{true, false});
auto c1 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({3.0, 4.0}), mask1),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Ho", "Sunyani"})),
});
auto archive = pluss::kindle::Archive({c0, c1});

auto out = archive.keys({"id"}).fillnull(0.0);
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
id                       | Series       | Series[float64]  | 2       | [1, 0]                                           
city                     | Note         | Note             | 2       | [Accra, Tema]                                    
========================================================================================================================
```

Only the projected `id` stream remains in the result, so the fill runs on the smaller
archive directly.

## Notes

- No no-argument overload — see the first important callout above.
- The result is an `Archive`, not an accessor shell.
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise.

## See Also

- [`Archive::has`](has.md): check whether a key exists before selecting it.
- [`Archive::drop`](drop.md): remove whole streams from every catalog instead of scoping
  a transform to selected keys.
- [`Catalog::keys`](../catalog/keys.md): the single-catalog equivalent, including its
  no-argument overload this method has no counterpart for.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="keyaxis.html">
    <span class="label">Previous</span>
    <span class="title">keyaxis</span>
  </a>
  <a class="next-link" href="kindelize.html">
    <span class="label">Next</span>
    <span class="title">kindelize</span>
  </a>
</div>
