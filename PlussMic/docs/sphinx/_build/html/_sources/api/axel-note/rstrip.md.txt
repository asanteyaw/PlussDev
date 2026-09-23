<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 14aebf849713 -->

# AxelNoteAccessor::rstrip

[Back to AxelNoteAccessor API Reference](../../axel_note_accessor_api_reference)

## Signatures

```cpp
Axel AxelNoteAccessor::rstrip() const

Axel AxelNoteAccessor::rstrip(std::string_view chars) const
```

## What It Does

Trims characters from the **right** edge of each bound field's text, in
every registry frame, returning a new axel with those fields replaced.
The no-argument overload trims whitespace; the `chars`-argument
overload trims any of the given characters, in any order, repeated as
many times as they appear at the end. Applies
[`RegistryNoteAccessor::rstrip`](../registry-note/rstrip.md) to every
frame via [`Axel::map`](../axel/map.md).

## Parameters

**chars** (`std::string_view`, one-argument overload only)
    The set of characters to strip from the right edge. Every trailing
    character that appears anywhere in `chars` is removed, stopping at
    the first character (scanning from the end) that doesn't.

## Returns

`Axel`
    A new axel with every bound field's text right-trimmed, in every
    frame; every unbound field unchanged.

## Examples

Example 1. No-argument overload trims trailing whitespace.

```cpp
auto r0 = pluss::scribble::Registry({{"city", pluss::scribble::Slot({pluss::make_note("  Accra  ")})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"city"}).nt().rstrip();
std::cout << "'" << std::get<pluss::Note>(out.registry(0).fields("city").at(0)).at(0) << "'";
```

### Output

```text
'  Accra'
```

Only the trailing spaces are removed — the leading whitespace is
untouched, unlike [`strip()`](strip.md).

Example 2. `chars`-argument overload trims a specific character set
from the right edge only.

```cpp
auto r0 = pluss::scribble::Registry({{"code", pluss::scribble::Slot({pluss::make_note("xxAccraxx")})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"code"}).nt().rstrip("x");
std::cout << std::get<pluss::Note>(out.registry(0).fields("code").at(0)).at(0);
```

### Output

```text
xxAccra
```

## Notes

- Only trims the right edge — see [`lstrip()`](lstrip.md) for the left
  edge and [`strip()`](strip.md) for both.
- The `chars`-argument overload treats `chars` as a *set* of characters,
  not a literal suffix — order within `chars` doesn't matter.
- Requires every bound field, in every frame, to be `Note`-kind — a
  non-`Note` field throws `"Slot::nt: slot kind must be Note"`.

## See Also

- [`AxelNoteAccessor::lstrip`](lstrip.md): the left-edge counterpart.
- [`AxelNoteAccessor::strip`](strip.md): trims both edges at once.
- [`Axel::nt`](../axel/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="replace.html">
    <span class="label">Previous</span>
    <span class="title">replace</span>
  </a>
  <a class="next-link" href="starts.html">
    <span class="label">Next</span>
    <span class="title">starts</span>
  </a>
</div>
