<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f365cf8171de -->

# SlotNoteAccessor::lstrip

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signatures

```cpp
Slot SlotNoteAccessor::lstrip() const

Slot SlotNoteAccessor::lstrip(std::string_view chars) const
```

## What It Does

Trims characters from the **left** edge of each value in the bound slot,
returning a new slot. The no-argument overload trims whitespace; the
`chars`-argument overload trims any of the given characters.

## Parameters

**chars** (`std::string_view`, one-argument overload only)
    The set of characters to strip from the left edge.

## Returns

`Slot`
    A new slot, every value left-trimmed.

## Examples

Example 1. No-argument overload trims leading whitespace.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_note("  Accra  ")});
auto out = slot.nt().lstrip();
std::cout << "'" << std::get<pluss::Note>(out.at(0)).at(0) << "'";
```

### Output

```text
'Accra  '
```

Example 2. `chars`-argument overload trims a specific character set from
the left edge only.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_note("xxAccraxx")});
auto out = slot.nt().lstrip("x");
std::cout << std::get<pluss::Note>(out.at(0)).at(0);
```

### Output

```text
Accraxx
```

## Notes

- Only trims the left edge — see [`rstrip()`](rstrip.md) for the right
  edge and [`strip()`](strip.md) for both.
- The `chars`-argument overload treats `chars` as a *set*, not a literal
  prefix.
- Requires the slot to be `Note`-kind — a non-`Note` slot throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`SlotNoteAccessor::rstrip`](rstrip.md): the right-edge counterpart.
- [`SlotNoteAccessor::strip`](strip.md): trims both edges at once.
- [`Slot::nt`](../slot/nt.md): the entry point this accessor is reached
  from.

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
