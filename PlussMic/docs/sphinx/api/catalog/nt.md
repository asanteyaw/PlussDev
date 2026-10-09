<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 55eed3954cb5 -->

# Catalog::nt

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
CatalogNoteAccessor Catalog::nt() const
```

## What It Does

The entry point into text-processing operations on the catalog's `Note`-kind streams.
`nt()` binds the current catalog and hands back a `CatalogNoteAccessor`, which exposes
23 text-transformation methods (trimming, case conversion, length/substring queries,
pattern matching, splitting, padding, encoding, and replacement) — see the
[CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference) for
every method's own dedicated page.

```{important} nt() binds EVERY key in the catalog, not just the Note-kind ones
Unlike Registry's `fields(keys).nt()` (which lets you pre-select a subset of fields
before binding), Catalog has no equivalent narrowing step -- `Catalog::nt()` takes no
arguments and always binds every key currently in the catalog. Calling any
CatalogNoteAccessor method on a catalog that has even one non-Note-kind key throws --
see the edge case example below. To operate on only your text columns, pre-filter with
[`Catalog::keys(names)`](keys.md) or [`Catalog::keyaxis().note()`](keyaxis.md) before
calling `.nt()`.
```

## Parameters

`None`

## Returns

`CatalogNoteAccessor`
    A binder for the current catalog — call one of its 23 methods to transform every
    `Note`-kind key's text.

## Examples

Example 1. `nt()` binds a catalog and exposes text-processing methods — see
[`CatalogNoteAccessor::strip`](../catalog-note/strip.md) for this specific method's own
page.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "city"_k = pluss::kindle::Stream(pluss::make_note({"  Accra  ", "  Tema  "})),
});
auto out = ct.nt().strip();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [city]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
city                     | Note         | Note             | 2       | [Accra, Tema]                                    
========================================================================================================================
```

Example 2 (edge case). Calling any `CatalogNoteAccessor` method on a catalog with a
non-`Note`-kind key throws, since `nt()` binds every key unconditionally.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
});
try {
  auto out = ct.nt().strip();
} catch (const pluss::Error& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Slate::strip is only valid for Note payload
```

## Notes

- Not `noexcept`; the returned accessor itself does no work, but every method it
  exposes eagerly re-checks every bound key's kind.
- See the important callout above — every one of the 23 methods on the returned
  accessor shares this every-key-bound, no-narrowing behavior.
- Requires a defined catalog; throws `"Catalog: undefined handle"` otherwise (checked
  when a method is actually called, not by `nt()` itself).

## See Also

- [CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference): the
  full list of 23 methods this accessor exposes.
- [`Catalog::tl`](tl.md) / [`Catalog::ts`](ts.md): the sibling accessors for
  timestamp-feature and time-series operations.
- [`Catalog::keyaxis`](keyaxis.md): filter down to just the `Note`-kind keys first, via
  `.note()`, before falling back to per-key operations if you need to mix text and
  non-text processing in one pipeline.
- [`Registry::fields(keys).nt()`](../registry/nt.md): the scribble-model equivalent,
  which *does* let you narrow to a key subset before binding.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nrows.html">
    <span class="label">Previous</span>
    <span class="title">nrows</span>
  </a>
  <a class="next-link" href="plot.html">
    <span class="label">Next</span>
    <span class="title">plot</span>
  </a>
</div>
