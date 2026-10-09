<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c04398a764e0 -->

# DataFrameNoteAccessor::starts

[Back to DataFrameNoteAccessor API Reference](../../dataframe_note_accessor_api_reference)

## Signature

```cpp
DataFrameNoteAccessor::starts(std::string_view prefix) const -> DataFrame
```

## What It Does

Checks every selected text feature's cells for a literal, case-sensitive
prefix match, replacing each cell with a `Bool` — `true` where the cell's
text begins with `prefix`, `false` where it doesn't. The feature's payload
kind changes from `Note` to `Series` (`Bool`), same as every other
boolean-producing method on this accessor ([`has()`](has.md),
[`matches()`](matches.md), [`fullmatch()`](fullmatch.md)).

`starts()` has exactly one overload and takes no options — it is always a
plain, case-sensitive, literal substring-at-position-0 check. There's no
regex mode and no case-insensitive toggle; for either of those, use
[`has()`](has.md) with a `TextOptions` and an anchoring pattern (e.g.
`has("^prefix", {.regex = true})`), or [`matches()`](matches.md) with a
regex pattern anchored at the start.

## Parameters

**prefix** (`std::string_view`)
    The literal text to check for at the start of each cell. An empty
    prefix matches every non-null cell (every string starts with the empty
    string).

## Returns

`DataFrame`
    A new frame with every selected feature replaced by a `Bool` Series:
    `true` where the cell starts with `prefix`, `false` where it doesn't.

## Examples

Example 1. Check a text feature for a literal prefix.

```cpp
using namespace pluss::table;

auto catalog = DataFrame({
    {"code", Slate(pluss::make_note(
        {"SKU-1001", "sku-1002", "REF-2003", "SKU-1004"}),
        pluss::make_series(std::vector<bool>{true, true, true, false}))},
});
std::cout << catalog;

auto out = catalog.nt().starts("SKU-");
std::cout << out;
```

### Output (before)

```text
===============
     | code    
---------------
0    | SKU-1001
1    | sku-1002
2    | REF-2003
3    | NULL    
---------------
[4 cs x 1 feat]
```

### Output (after)

```text
============
     | code 
------------
0    | true 
1    | false
2    | false
3    | NULL 
------------
[4 cs x 1 feat]
```

Row 0 (`"SKU-1001"`) starts with `"SKU-"`, so it's `true`. Row 1
(`"sku-1002"`) does not — the comparison is case-sensitive, so a lowercase
`"sku-"` is not a match for `"SKU-"`. Row 2 doesn't start with `"SKU-"` at
all. Row 3 was constructed invalid; it stays `NULL` rather than being
evaluated as `true` or `false`.

Example 2. Chain `starts()` directly into `DataFrame::filter()` to keep only
the matching rows — the most common real use of a prefix check.

```cpp
auto skus_only = catalog.filter(catalog.nt().starts("SKU-").features("code"));
std::cout << skus_only;
```

### Output

```text
===============
     | code    
---------------
0    | SKU-1001
---------------
[1 cs x 1 feat]
```

Only row 0 survives: it's the sole row where `code` is both valid and starts
with `"SKU-"`. Rows with `false` or `NULL` in the boolean mask are dropped by
`filter()`.

Example 3. An empty prefix matches every valid cell, regardless of content.

```cpp
auto trivial = catalog.nt().starts("");
std::cout << trivial;
```

### Output

```text
============
     | code 
------------
0    | true 
1    | true 
2    | true 
3    | NULL 
------------
[4 cs x 1 feat]
```

Every valid row is `true` — any string starts with the empty string. Row 3
is still `NULL`, since validity is checked before the prefix comparison ever
runs.

## Notes

- Case-sensitive, literal comparison only — no regex mode, no
  case-insensitivity toggle. Use [`has()`](has.md) with `TextOptions` (or a
  prior [`lower()`](lower.md)/[`casefold()`](casefold.md) normalization
  step) for anything more flexible.
- Changes the feature's payload kind from `Note` to `Series` (`Bool`).
- A null cell on input stays null on output — it evaluates to neither `true`
  nor `false`.
- Pairs naturally with `DataFrame::filter()` to keep only rows matching a
  prefix (Example 2).

## See Also

- [`DataFrameNoteAccessor::ends`](ends.md): the same check anchored at the
  end of the cell instead of the start.
- [`DataFrameNoteAccessor::has`](has.md): substring search anywhere in the
  cell, with optional regex and case-insensitive modes.
- [`DataFrameNoteAccessor::matches`](matches.md): a regex-anchored-at-start
  check, for patterns more flexible than a literal prefix.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="split.html">
    <span class="label">Previous</span>
    <span class="title">split</span>
  </a>
  <a class="next-link" href="strip.html">
    <span class="label">Next</span>
    <span class="title">strip</span>
  </a>
</div>
