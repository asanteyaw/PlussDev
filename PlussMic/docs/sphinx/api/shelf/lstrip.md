<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 599f5554d91f -->

# Shelf::lstrip

[Back to Shelf API Reference](../../shelf_api_reference)

## Signatures

```cpp
Shelf Shelf::lstrip() const
Shelf Shelf::lstrip(std::string_view chars) const
```

## What It Does

The left-only half of [`strip()`](strip.md): trims characters from the **start** of
a `Note` shelf's cells, leaving the trailing end untouched. Same two policies as
`strip()` (whitespace by default, character-set membership with `chars`) — see that
page for the full explanation of why the `chars` overload is set-membership, not
literal substring matching.

## Parameters

**chars** (`std::string_view`, `chars`-overload only)

The set of characters to strip from each cell's left edge only.

## Returns

`Shelf`
    A new `Note`-kind shelf, same length as `*this`, with each cell's leading text
    trimmed.

## Examples

Example 1. Strip a leading marker character, leaving the trailing end untouched.

```cpp
using namespace pluss::table;

auto ids = Shelf(pluss::make_note({"#203930", "##7788", "4455##"}));
auto out = ids.lstrip("#");
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

- See [`strip()`](strip.md) for the full explanation of the default-whitespace vs.
  `chars`-set-membership policies, and the non-`Note` throw
  (`"Shelf::lstrip is only valid for Note payload"` /
  `"Shelf::lstrip(chars) is only valid for Note payload"`).

## See Also

- [`Shelf::rstrip`](rstrip.md): the mirror image, trimming the trailing end only.
- [`Shelf::strip`](strip.md): both ends at once.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lower.html">
    <span class="label">Previous</span>
    <span class="title">lower</span>
  </a>
  <a class="next-link" href="lt.html">
    <span class="label">Next</span>
    <span class="title">lt</span>
  </a>
</div>
