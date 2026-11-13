<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 921625b2fbef -->

# DataFrameNoteAccessor::lstrip

[Back to DataFrameNoteAccessor API Reference](../../dataframe_note_accessor_api_reference)

## Signatures

```cpp
DataFrame DataFrameNoteAccessor::lstrip() const
DataFrame DataFrameNoteAccessor::lstrip(std::string_view chars) const
```

## What It Does

The left-only half of [`strip()`](strip.md): trims characters from the
*start* of every selected text feature's cells, leaving the trailing end
completely untouched. Same two policies as `strip()`:

- **No-argument overload**: trims the classic ASCII whitespace set (space,
  tab, newline, carriage return, form feed, vertical tab) from the left edge
  only.
- **`chars` overload**: trims by character-set membership from the left edge
  only — every leading character that's a member of `chars` is peeled off,
  one at a time, until a character outside the set is reached. See
  [`strip()`](strip.md)'s page for the full explanation of why this is
  set-membership, not literal substring matching.

Everything else about how the accessor applies an operation — to every
selected feature in one call, leaving null cells null, and reachable either
through [`DataFrame::nt()`](../dataframe/nt.md) or by constructing
`DataFrameNoteAccessor` directly against a mixed frame — is identical to
[`strip()`](strip.md); see that page for the full explanation and worked
examples of both.

## Parameters

**chars** (`std::string_view`, `chars`-overload only)

The set of characters to strip from each cell's left edge only.

## Returns

`DataFrame`
    A new frame with every selected feature's text left-trimmed; the trailing
    end of every cell is preserved byte-for-byte.

## Examples

Example 1. Default overload: trim leading whitespace only, leaving trailing
whitespace in place.

```cpp
using namespace pluss::table;

auto catalog = DataFrame({
    {"sku", Slate(
        pluss::make_note({"SKU-1001  ", "SKU-1002", "SKU-1003   ", "SKU-1004"}),
        pluss::make_series(std::vector<bool>{true, true, true, false}))},
}, Slate(pluss::make_note({"p1", "p2", "p3", "p4"})));

auto out = catalog.nt().lstrip();
std::cout << out;
```

### Output

```text
==================
     | sku        
------------------
p1   | SKU-1001   
p2   | SKU-1002   
p3   | SKU-1003   
p4   | NULL       
------------------
[4 cs x 1 feat]
```

None of `catalog`'s values had *leading* whitespace to begin with — the
trailing spaces on `p1` and `p3` are visibly still present in the rendered
column width, unlike [`strip()`'s Example 1](strip.md), which would have
removed them. `p4` stays `NULL`, exactly as it does for every accessor
method.

Example 2. `chars` overload: strip a character set from the left edge only,
using the same bracket data [`strip()`'s Example 2](strip.md) uses, to make
the one-sided difference obvious.

```cpp
auto codes = DataFrame({
    {"code", Slate(pluss::make_note({"[[[SKU-1001]]]", "((SKU-1002))", "SKU-1003"}))},
});

auto out2 = codes.nt().lstrip("[(");
std::cout << out2;
```

### Output

```text
==================
     | code       
------------------
0    | SKU-1001]]]
1    | SKU-1002)) 
2    | SKU-1003   
------------------
[3 cs x 1 feat]
```

Compare this directly to `strip("[]()")`'s result of `"SKU-1001"` for the same
row: `lstrip("[(")` only ever removes characters from the left, so the
trailing `]]]` and `))` survive completely intact — only the leading `[[[`
and `((` are gone.

## Notes

- Applies to every feature the accessor was built with, in one call — see
  [`strip()`](strip.md)'s Notes for the full explanation shared by every
  method on this accessor.
- A null cell on input stays null on output.
- For the mirror-image right-edge trim, see [`rstrip()`](rstrip.md).

## See Also

- [`DataFrameNoteAccessor::strip`](strip.md): both edges at once, and the
  canonical explanation of the accessor's shared mechanics.
- [`DataFrameNoteAccessor::rstrip`](rstrip.md): the right-only equivalent.
- [`DataFrameNoteAccessor::pad`](pad.md): the inverse — add characters on a
  chosen side instead of removing them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lower.html">
    <span class="label">Previous</span>
    <span class="title">lower</span>
  </a>
  <a class="next-link" href="matches.html">
    <span class="label">Next</span>
    <span class="title">matches</span>
  </a>
</div>
