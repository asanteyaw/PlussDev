<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d474f414ec2b -->

# CatalogNoteAccessor::matches

[Back to CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference)

## Signature

```cpp
Catalog CatalogNoteAccessor::matches(std::string_view pattern, bool full = false) const
```

## What It Does

Tests every bound key's text against a regular expression, replacing each `Note`-kind
key with a `Bool` `Series`-kind key of results.

```{important} CatalogNoteAccessor requires every bound key to be Note-kind
Same precondition as every method on this accessor — see
[`CatalogNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

```{important} pattern is ALWAYS a regex — unlike has(), there's no TextOptions overload
Unlike has(), pattern here is always treated as a regex -- there's no TextOptions
overload to switch to a literal search.
```

`full` controls the anchoring:
- `full = false` (the default): `std::regex_search` semantics — the pattern just needs
  to appear somewhere in the value, unanchored (start-anchored patterns like `^...`
  still work as expected).
- `full = true`: `std::regex_match` semantics — the pattern must match the *entire*
  value, equivalent to calling [`fullmatch(pattern)`](fullmatch.md).

## Parameters

**pattern** (`std::string_view`)
    A regular expression, always — never literal text.

**full** (`bool`, defaults to `false`)
    When `true`, the pattern must match the entire value rather than just appear
    somewhere within it.

## Returns

`Catalog`
    A new catalog with every bound key replaced by a `Bool` `Series` of per-value
    regex-match results.

## Examples

Example 1. Default (`full = false`): the pattern just needs to appear somewhere in the
value — a digits-only pattern still matches values with extra non-digit characters.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "code"_k = pluss::kindle::Stream(pluss::make_note({"123", "AB-123", "99XY"})),
});
auto out = ct.nt().matches("[0-9]+");
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [code]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
code                     | Series       | Series[bool]     | 3       | [true, true, true]                               
========================================================================================================================
```

Example 2. `full = true` requires the whole value to match — the same digits-only
pattern now rejects `"AB-123"` (which has letters and a dash) and `"99XY"` (which has
letters), leaving only `"123"`.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "code"_k = pluss::kindle::Stream(pluss::make_note({"123", "AB-123", "99XY"})),
});
auto out = ct.nt().matches("[0-9]+", true);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [code]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
code                     | Series       | Series[bool]     | 3       | [true, false, false]                             
========================================================================================================================
```

## Notes

- See the important callouts above.
- `matches(pattern, true)` and [`fullmatch(pattern)`](fullmatch.md) are equivalent.
- Changes each bound key's kind from `Note` to `Series` (`Bool`).
- No case-insensitivity flag here — use inline regex flags (e.g. `(?i)`) in `pattern`
  if needed.

## See Also

- [`CatalogNoteAccessor::fullmatch`](fullmatch.md): equivalent to `matches(pattern,
  true)`, without the extra argument.
- [`CatalogNoteAccessor::has`](has.md): a related pattern test with a `TextOptions`
  overload for literal-vs-regex switching, absent here.
- [`Catalog::nt`](../catalog/nt.md): the entry point this accessor is reached from.
- [`RegistryNoteAccessor::matches`](../registry-note/matches.md): the bundle-model
  equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lstrip.html">
    <span class="label">Previous</span>
    <span class="title">lstrip</span>
  </a>
  <a class="next-link" href="normalize.html">
    <span class="label">Next</span>
    <span class="title">normalize</span>
  </a>
</div>
