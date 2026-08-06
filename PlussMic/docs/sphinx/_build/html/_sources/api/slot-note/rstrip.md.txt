<!-- pluss-hand-authored -->

# SlotNoteAccessor::rstrip

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signatures

```cpp
Slot SlotNoteAccessor::rstrip() const

Slot SlotNoteAccessor::rstrip(std::string_view chars) const
```

## What It Does

Trims characters from the **right** edge of each value in the bound
slot, returning a new slot. The no-argument overload trims whitespace;
the `chars`-argument overload trims any of the given characters.

## Parameters

**chars** (`std::string_view`, one-argument overload only)
    The set of characters to strip from the right edge.

## Returns

`Slot`
    A new slot, every value right-trimmed.

## Examples

Example 1. No-argument overload trims trailing whitespace.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("  Accra  ")});
auto out = slot.nt().rstrip();
std::cout << "'" << std::get<pluss::Note>(out.at(0)).at(0) << "'";
```

### Output

```text
'  Accra'
```

Example 2. `chars`-argument overload trims a specific character set from
the right edge only.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("xxAccraxx")});
auto out = slot.nt().rstrip("x");
std::cout << std::get<pluss::Note>(out.at(0)).at(0);
```

### Output

```text
xxAccra
```

## Notes

- Only trims the right edge — see [`lstrip()`](lstrip.md) for the left
  edge and [`strip()`](strip.md) for both.
- Requires the slot to be `Note`-kind — a non-`Note` slot throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`SlotNoteAccessor::lstrip`](lstrip.md): the left-edge counterpart.
- [`SlotNoteAccessor::strip`](strip.md): trims both edges at once.
- [`Slot::nt`](../slot/nt.md): the entry point this accessor is reached
  from.

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
