<!-- pluss-hand-authored -->

# SlotNoteAccessor::strip

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signatures

```cpp
Slot SlotNoteAccessor::strip() const

Slot SlotNoteAccessor::strip(std::string_view chars) const
```

## What It Does

Trims characters from **both** edges of each value in the bound slot,
returning a new slot. The no-argument overload trims whitespace; the
`chars`-argument overload trims any of the given characters, from either
end.

```{important} SlotNoteAccessor requires the bound slot to be Note-kind
Every method on this accessor — not just `strip()` — routes through
`Slot::nt()`. If the slot isn't `Note`-kind (`Series`, `Timeline`,
`TimeIndex`, `Mixed`, `Null`, or `Empty`), the call throws
`"Slot::nt: slot kind must be Note"`.
```

## Parameters

**chars** (`std::string_view`, one-argument overload only)
    The set of characters to strip from both edges.

## Returns

`Slot`
    A new slot, every value trimmed on both edges.

## Examples

Example 1. No-argument overload trims leading and trailing whitespace.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("  Accra  "), pluss::make_note("  Tema  ")});
auto out = slot.nt().strip();
std::cout << "'" << std::get<pluss::Note>(out.at(0)).at(0) << "' '"
          << std::get<pluss::Note>(out.at(1)).at(0) << "'";
```

### Output

```text
'Accra' 'Tema'
```

Example 2. `chars`-argument overload trims a specific character set from
both edges.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("xxAccraxx")});
auto out = slot.nt().strip("x");
std::cout << std::get<pluss::Note>(out.at(0)).at(0);
```

### Output

```text
Accra
```

Example 3. Calling any method on this accessor with a non-`Note` slot
throws — see the important callout above.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1)});
try {
  auto out = slot.nt().strip();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Slot::nt: slot kind must be Note
```

## Notes

- See the important callout above — applies to every method on
  `SlotNoteAccessor`, not only `strip()`.
- The `chars`-argument overload treats `chars` as a *set*, not a literal
  prefix/suffix.

## See Also

- [`SlotNoteAccessor::lstrip`](lstrip.md) / [`rstrip()`](rstrip.md): the
  one-sided variants.
- [`SlotNoteAccessor::pad`](pad.md): the inverse operation, adding
  characters instead of removing them.
- [`Slot::nt`](../slot/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="starts.html">
    <span class="label">Previous</span>
    <span class="title">starts</span>
  </a>
  <a class="next-link" href="titlecase.html">
    <span class="label">Next</span>
    <span class="title">titlecase</span>
  </a>
</div>
