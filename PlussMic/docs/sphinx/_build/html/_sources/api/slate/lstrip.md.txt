<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 599f5554d91f -->

# Slate::lstrip

[Back to Slate API Reference](../../slate_api_reference)

## Signatures

```cpp
Slate::lstrip() const -> Slate
Slate::lstrip(std::string_view chars) const -> Slate
```

## What It Does

The left-only half of [`strip()`](strip.md): trims characters from the **start** of
a `Note` slate's cells, leaving the trailing end untouched. Same two policies as
`strip()` (whitespace by default, character-set membership with `chars`) — see that
page for the full explanation of why the `chars` overload is set-membership, not
literal substring matching.

## Parameters

**chars** (`std::string_view`, `chars`-overload only)

The set of characters to strip from each cell's left edge only.

## Returns

`Slate`
    A new `Note`-kind slate, same length as `*this`, with each cell's leading text
    trimmed.

## Examples

Example 1. Strip a leading marker character, leaving the trailing end untouched.

```cpp
using namespace pluss::table;

auto ids = Slate(pluss::make_note({"#203930", "##7788", "4455##"}));
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
  (`"Slate::lstrip is only valid for Note payload"` /
  `"Slate::lstrip(chars) is only valid for Note payload"`).

## See Also

- [`Slate::rstrip`](rstrip.md): the mirror image, trimming the trailing end only.
- [`Slate::strip`](strip.md): both ends at once.

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
