<!-- pluss-hand-authored -->

# ShelfNoteAccessor::rstrip

[Back to ShelfNoteAccessor API Reference](../../shelf_note_accessor_api_reference)

## Signatures

```cpp
Shelf ShelfNoteAccessor::rstrip() const
Shelf ShelfNoteAccessor::rstrip(std::string_view chars) const
```

## What It Does

The right-only half of [`strip()`](strip.md): trims characters from the **end**
of the bound shelf's cells, leaving the leading edge untouched. A direct
forward to [`Shelf::rstrip()`](../shelf/rstrip.md). Same two policies as
`strip()` (whitespace by default, character-set membership with `chars`).

## Parameters

**chars** (`std::string_view`, `chars`-overload only)

The set of characters to strip from each cell's right edge only.

## Returns

`Shelf`
    A new `Note`-kind shelf, same length as the bound shelf, with each cell's
    trailing text trimmed.

## Examples

Example 1. Strip a trailing marker character, leaving the leading end
untouched.

```cpp
using namespace pluss::table;

auto ids = Shelf(pluss::make_note({"#203930", "##7788", "4455##"}));
auto out = ids.nt().rstrip("#");
std::cout << out;
```

### Output

```text
0  #203930
1  ##7788
2  4455

ptype: Note
```

Rows `0` and `1`'s leading `#` characters are untouched — only trailing `#`
characters are peeled off.

## Notes

- `ShelfNoteAccessor::rstrip()` is a one-line forward to
  [`Shelf::rstrip()`](../shelf/rstrip.md) — identical behavior and throws.
- See [`strip()`](strip.md) for the full explanation of the default-whitespace
  vs. `chars`-set-membership policies, and the non-`Note` throw.

## See Also

- [`Shelf::rstrip`](../shelf/rstrip.md): the direct, non-accessor form this
  method forwards to.
- [`ShelfNoteAccessor::lstrip`](lstrip.md): the mirror image, trimming the
  leading end only.
- [`ShelfNoteAccessor::strip`](strip.md): both ends at once.

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
