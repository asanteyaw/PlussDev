<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 82544779068b -->

# Archive::fillnull

[Back to Archive API Reference](../../archive_api_reference)

## Signatures

```cpp
Archive Archive::fillnull(double value) const
Archive Archive::fillnull(std::string_view value) const
Archive Archive::fillnull(const char* value) const
Archive Archive::fillnull(const pluss::Timeline& value) const
Archive Archive::fillnull(const pluss::Duration& value) const
```

## What It Does

Fills `NULL` cells across every catalog in the archive with `value` — delegating to each
catalog's own [`Catalog::fillnull()`](../catalog/fillnull.md), which only touches streams
whose payload kind matches `value`'s type. Every other stream, in every catalog, passes
through untouched, with no error.

```{important} Only streams matching value's kind are touched — silently, no throw
Same kind-selective rule as [`Catalog::fillnull()`](../catalog/fillnull.md), applied
archive-wide: `fillnull(0.0)` only fills `Series`-kind streams; `fillnull(std::string_view)`
only fills `Note`-kind streams; and so on. Streams of every other kind, in every catalog,
are returned exactly as they were.
```

## Parameters

**value** (`double` / `std::string_view` / `const char*` / `const pluss::Timeline&` /
`const pluss::Duration&`)
    The fill value. Its C++ type selects both the overload and which streams are eligible
    to be filled — see the important callout above.

## Returns

`Archive`
    A new archive, same depth and depth labels as the receiver, with `NULL` cells filled in
    every kind-matching stream across every catalog.

## Examples

Example 1. `fillnull(0.0)` fills only the numeric stream — the `Note`-kind stream's own
`NULL` is left as-is, in every catalog.

```cpp
using namespace pluss::kindle::literals;

auto mask_id = pluss::make_series(std::vector<bool>{true, false, true});
auto mask_city = pluss::make_series(std::vector<bool>{true, true, false});
auto c0 = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}), mask_id),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}), mask_city),
});
auto archive = pluss::kindle::Archive({c0});

auto out = archive.fillnull(0.0);
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
id                       | Series       | Series[float64]  | 3       | [1, 0, 3]                                        
city                     | Note         | Note             | 3       | [Accra, Tema, NA]                                
========================================================================================================================
```

## Notes

- Kind-selective and silent about it, applied uniformly across every catalog in the
  archive — see the important callout above.
- Requires a defined archive; throws `"Archive: undefined handle"` otherwise.
- Depth labels are preserved on the result.

## See Also

- [`Archive::dropnull`](dropnull.md): remove rows with any `NULL`, instead of filling them.
- [`Archive::ffill`](ffill.md) / [`Archive::bfill`](bfill.md): fill using neighboring
  values instead of a fixed scalar.
- [`Catalog::fillnull`](../catalog/fillnull.md): the single-catalog delegate this method
  applies to every catalog in the archive.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ffill.html">
    <span class="label">Previous</span>
    <span class="title">ffill</span>
  </a>
  <a class="next-link" href="freq.html">
    <span class="label">Next</span>
    <span class="title">freq</span>
  </a>
</div>
