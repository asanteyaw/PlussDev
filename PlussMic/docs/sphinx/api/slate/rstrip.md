<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 81784c128fb3 -->

# Slate::rstrip

[Back to Slate API Reference](../../slate_api_reference)

## Signatures

```cpp
Slate::rstrip() const -> Slate
Slate::rstrip(std::string_view chars) const -> Slate
```

## What It Does

The right-only half of [`strip()`](strip.md): trims characters from the **end** of
a `Note` slate's cells, leaving the leading edge untouched. Same two policies as
`strip()` (whitespace by default, character-set membership with `chars`).

## Parameters

**chars** (`std::string_view`, `chars`-overload only)

The set of characters to strip from each cell's right edge only.

## Returns

`Slate`
    A new `Note`-kind slate, same length as `*this`, with each cell's trailing
    text trimmed.

## Examples

Example 1. Strip a trailing marker character, leaving the leading end untouched.

```cpp
using namespace pluss::table;

auto ids = Slate(pluss::make_note({"#203930", "##7788", "4455##"}));
auto out = ids.rstrip("#");
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

- See [`strip()`](strip.md) for the full explanation of the default-whitespace vs.
  `chars`-set-membership policies, and the non-`Note` throw.

## See Also

- [`Slate::lstrip`](lstrip.md): the mirror image, trimming the leading end only.
- [`Slate::strip`](strip.md): both ends at once.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rpow.html">
    <span class="label">Previous</span>
    <span class="title">rpow</span>
  </a>
  <a class="next-link" href="rsub.html">
    <span class="label">Next</span>
    <span class="title">rsub</span>
  </a>
</div>
