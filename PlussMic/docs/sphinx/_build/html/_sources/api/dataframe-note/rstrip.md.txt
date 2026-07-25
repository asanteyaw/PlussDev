<!-- pluss-hand-authored -->

# DataFrameNoteAccessor::rstrip

[Back to DataFrameNoteAccessor API Reference](../../dataframe_note_accessor_api_reference)

## Signatures

```cpp
DataFrameNoteAccessor::rstrip() const -> DataFrame
DataFrameNoteAccessor::rstrip(std::string_view chars) const -> DataFrame
```

## What It Does

The right-only half of [`strip()`](strip.md): trims characters from the *end*
of every selected text feature's cells, leaving the leading edge completely
untouched. Same two policies as `strip()` and [`lstrip()`](lstrip.md):

- **No-argument overload**: trims the classic ASCII whitespace set (space,
  tab, newline, carriage return, form feed, vertical tab) from the right edge
  only.
- **`chars` overload**: trims by character-set membership from the right
  edge only — every trailing character that's a member of `chars` is peeled
  off, one at a time, until a character outside the set is reached. See
  [`strip()`](strip.md) for the full explanation of why this is
  set-membership, not literal substring matching.

Everything else — applying to every selected feature in one call, leaving
null cells null, and the two ways to reach the accessor — is identical to
[`strip()`](strip.md); see that page for the full explanation.

## Parameters

**chars** (`std::string_view`, `chars`-overload only)

The set of characters to strip from each cell's right edge only.

## Returns

`DataFrame`
    A new frame with every selected feature's text right-trimmed; the leading
    edge of every cell is preserved byte-for-byte.

## Examples

Example 1. Default overload: trim trailing whitespace only, leaving leading
whitespace in place.

```cpp
using namespace pluss::table;

auto catalog = DataFrame({
    {"sku", Thread(
        pluss::make_note({"  SKU-1001", "SKU-1002   ", "  SKU-1003", "SKU-1004"}),
        pluss::make_series(std::vector<bool>{true, true, true, false}))},
}, Thread(pluss::make_note({"p1", "p2", "p3", "p4"})));

auto out = catalog.note().rstrip();
std::cout << out;
```

### Output

```text
=================
     | sku       
-----------------
p1   |   SKU-1001
p2   | SKU-1002  
p3   |   SKU-1003
p4   | NULL      
-----------------
[4 cs x 1 feat]
```

`p1` and `p3`'s leading two spaces are still there in the output (visible as
the extra indentation before `SKU-1001`/`SKU-1003`) — `rstrip()` never touches
the left edge. `p2`'s trailing spaces are gone. `p4` stays `NULL`.

Example 2. `chars` overload: strip a character set from the right edge only,
reusing bracket-style data (opening brackets only this time, to keep the
right-edge effect visible) to isolate the one-sided behavior.

```cpp
auto codes = DataFrame({
    {"code", Thread(pluss::make_note({"[[[SKU-1001", "((SKU-1002", "SKU-1003"}))},
});

auto out2 = codes.note().rstrip("])");
std::cout << out2;
```

### Output

```text
==================
     | code       
------------------
0    | [[[SKU-1001
1    | ((SKU-1002 
2    | SKU-1003   
------------------
[3 cs x 1 feat]
```

None of these values end in `]` or `)`, so `rstrip("])")` leaves every row
exactly as it was — the leading `[[[` and `((` (which `rstrip` never
inspects) stay put. This deliberately shows the flip side of
[`lstrip()`'s Example 2](lstrip.md): trimming the wrong edge's character set
is a safe no-op, not an error.

## Notes

- Applies to every feature the accessor was built with, in one call — see
  [`strip()`](strip.md)'s Notes for the mechanics shared by every method on
  this accessor.
- A null cell on input stays null on output.
- For the mirror-image left-edge trim, see [`lstrip()`](lstrip.md).

## See Also

- [`DataFrameNoteAccessor::strip`](strip.md): both edges at once, and the
  canonical explanation of the accessor's shared mechanics.
- [`DataFrameNoteAccessor::lstrip`](lstrip.md): the left-only equivalent.
- [`DataFrameNoteAccessor::pad`](pad.md): the inverse — add characters on a
  chosen side instead of removing them.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="replace.html">
    <span class="label">Previous</span>
    <span class="title">replace</span>
  </a>
  <a class="next-link" href="split.html">
    <span class="label">Next</span>
    <span class="title">split</span>
  </a>
</div>
