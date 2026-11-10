<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9b620d8e83aa -->

# CatalogNoteAccessor::has

[Back to CatalogNoteAccessor API Reference](../../catalog_note_accessor_api_reference)

## Signature

```cpp
Catalog CatalogNoteAccessor::has(
    std::string_view needle, const pluss::table::TextOptions& options = {}) const
```

## What It Does

Tests whether `needle` appears anywhere within every bound key's text, replacing each
`Note`-kind key with a `Bool` `Series`-kind key of results. Whether `needle` is treated
literally or as a regex pattern is switched between via `options`.

```{important} CatalogNoteAccessor requires every bound key to be Note-kind
Same precondition as every method on this accessor — see
[`CatalogNoteAccessor::strip`](strip.md)'s own important callout for the full
explanation and the exact throw message.
```

```{important} has() defaults to LITERAL matching -- the opposite default from count()
`has(needle)` with no `options` treats `needle` as literal text, not a regex pattern --
regex metacharacters (`.`, `*`, `+`, `?`, `(`, `)`, `|`, `[`, `]`) match themselves
literally by default. This is the opposite default from
[`count(pattern)`](count.md)'s single-argument overload, which defaults to regex mode.
Pass `TextOptions{.regex = true}` explicitly to search by pattern instead. See the
example below for a direct comparison.
```

`TextOptions` has four fields, though only two matter to `has()`:

### `TextOptions` fields

| Field | Type | Default |
|---|---|---|
| `regex` | `bool` | `false` |
| `case_sensitive` | `bool` | `true` |
| `limit` | `std::int64_t` | `-1` |
| `prefix` | `std::string` | — |

## Parameters

**needle** (`std::string_view`)
    The text to search for — literal text by default, or a regular expression pattern
    when `options.regex` is `true`.

**options** (`pluss::table::TextOptions`, defaults to `{}`)
    `options.regex` (default `false`): treat `needle` as a regex pattern instead of
    literal text. `options.case_sensitive` (default `true`): when `false`, the search
    ignores case.

## Returns

`Catalog`
    A new catalog with every bound key replaced by a `Bool` `Series` of per-value
    substring-presence results.

## Examples

Example 1. Default (literal) matching: a `.` in `needle` matches only a literal dot,
not "any character" — confirming the opposite default from
[`count()`](count.md)'s single-argument overload.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "txt"_k = pluss::kindle::Stream(pluss::make_note({"banana", "a.b.c"})),
});
auto litHas = ct.nt().has(".");
std::cout << "literal: " << litHas;

pluss::table::TextOptions regexOpt;
regexOpt.regex = true;
auto regexHas = ct.nt().has(".", regexOpt);
std::cout << "regex: " << regexHas;
```

### Output

```text
literal: ========================================================================================================================
kindle::Catalog  keys=1
keys: [txt]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
txt                      | Series       | Series[bool]     | 2       | [false, true]                                    
========================================================================================================================
regex: ========================================================================================================================
kindle::Catalog  keys=1
keys: [txt]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
txt                      | Series       | Series[bool]     | 2       | [true, true]                                     
========================================================================================================================
```

`"banana"` has no literal dot (`false`), while `"a.b.c"` does (`true`) — under literal
matching. Under `{.regex = true}`, `.` matches any character, so both values contain
at least one match.

## Notes

- See the important callouts above — the default-literal behavior is specific to
  `has()`; [`count()`](count.md)'s single-argument overload defaults the opposite way.
- Changes each bound key's kind from `Note` to `Series` (`Bool`).
- `options.limit`/`options.prefix` are part of `TextOptions` but have no effect on
  `has()`.

## See Also

- [`CatalogNoteAccessor::count`](count.md): a related pattern-search method whose
  single-argument default is regex — the mirror image of this gotcha.
- [`CatalogNoteAccessor::matches`](matches.md) / [`fullmatch()`](fullmatch.md):
  always-regex boolean pattern tests, with different anchoring than `has()`.
- [`CatalogNoteAccessor::starts`](starts.md) / [`ends()`](ends.md): position-anchored,
  always-literal substring tests.
- [`Catalog::nt`](../catalog/nt.md): the entry point this accessor is reached from.
- [`RegistryNoteAccessor::has`](../registry-note/has.md): the scribble-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fullmatch.html">
    <span class="label">Previous</span>
    <span class="title">fullmatch</span>
  </a>
  <a class="next-link" href="indicate.html">
    <span class="label">Next</span>
    <span class="title">indicate</span>
  </a>
</div>
