<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 14f88bc08100 -->

# Catalog::filter_by_text

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::filter_by_text(std::string_view field, std::string_view pattern) const -> Catalog
```

## What It Does

Keeps only the rows where `field`'s text contains `pattern` as a **case-insensitive
substring** — not a regular expression. Internally builds a boolean mask from the
substring search and applies it via [`filter()`](filter.md), so every stream in the
catalog is restricted to the matching rows, not just `field`.

## Parameters

**field** (`std::string_view`)
    The text stream to search. Must already exist.

**pattern** (`std::string_view`)
    The substring to search for, matched case-insensitively.

## Returns

`Catalog`
    A new catalog restricted to rows where `field` contains `pattern`.

## Examples

Example 1. Keep only rows whose text contains `"rain"`.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = ct.filter_by_text("text", "rain");
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [text]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
text                     | Note         | Note             | 1       | [heavy rain on road]                             
========================================================================================================================
```

## Notes

- Plain, case-insensitive substring search — not a regular expression. For regex matching
  on a single stream, see `CatalogNoteAccessor::matches()` (via
  [`keys(field).nt()`](keys.md)).
- Restricts every stream in the catalog, via [`filter()`](filter.md) — not just `field`.
- `field` must already exist, or the call throws.

## See Also

- [`Catalog::filter`](filter.md): the underlying row-selection mechanism.
- [`Catalog::query`](query.md): build an arbitrary mask from a callback, for filtering
  conditions beyond a plain substring match.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="filter.html">
    <span class="label">Previous</span>
    <span class="title">filter</span>
  </a>
  <a class="next-link" href="freq.html">
    <span class="label">Next</span>
    <span class="title">freq</span>
  </a>
</div>
