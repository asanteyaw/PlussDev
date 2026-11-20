<!-- pluss-hand-authored -->

# ShelfNoteAccessor::lstrip

[Back to ShelfNoteAccessor API Reference](../../shelf_note_accessor_api_reference)

## Signatures

```cpp
Shelf ShelfNoteAccessor::lstrip() const
Shelf ShelfNoteAccessor::lstrip(std::string_view chars) const
```

## What It Does

The left-only half of [`strip()`](strip.md): trims characters from the **start** of
the bound shelf's cells, leaving the trailing end untouched. A direct forward to
[`Shelf::lstrip()`](../shelf/lstrip.md). Same two policies as `strip()` (whitespace
by default, character-set membership with `chars`).

## Parameters

**chars** (`std::string_view`, `chars`-overload only)

The set of characters to strip from each cell's left edge only.

## Returns

`Shelf`
    A new `Note`-kind shelf, same length as the bound shelf, with each cell's
    leading text trimmed.

## Examples

Example 1. Strip a leading marker character, leaving the trailing end untouched.

```cpp
using namespace pluss::table;

auto ids = Shelf(pluss::make_note({"#203930", "##7788", "4455##"}));
auto out = ids.nt().lstrip("#");
std::cout << out;
```

### Output

```text
0  203930
1  7788
2  4455##

ptype: Note
```

Row `2`'s trailing `##` is untouched — only leading `#` characters are peeled off.

## Notes

- `ShelfNoteAccessor::lstrip()` is a one-line forward to
  [`Shelf::lstrip()`](../shelf/lstrip.md) — identical behavior and throws.
- See [`strip()`](strip.md) for the full explanation of the default-whitespace vs.
  `chars`-set-membership policies.

## See Also

- [`Shelf::lstrip`](../shelf/lstrip.md): the direct, non-accessor form this
  method forwards to.
- [`ShelfNoteAccessor::rstrip`](rstrip.md): the mirror image, trimming the
  trailing end only.
- [`ShelfNoteAccessor::strip`](strip.md): both ends at once.

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
