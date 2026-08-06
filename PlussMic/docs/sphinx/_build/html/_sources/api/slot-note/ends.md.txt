<!-- pluss-hand-authored -->

# SlotNoteAccessor::ends

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signature

```cpp
Slot SlotNoteAccessor::ends(std::string_view suffix) const
```

## What It Does

Checks whether each value in the bound slot ends with `suffix`, replacing
each `Note` value with a `Bool` `Series`.

## Parameters

**suffix** (`std::string_view`)
    The literal text to check for at the end of each value. Always a
    literal comparison — no regex or case-insensitive mode, unlike
    [`has()`](has.md).

## Returns

`Slot`
    A new `Series[bool]` slot.

## Examples

Example 1. Check which city names end in `"ma"`.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")});
auto out = slot.nt().ends("ma");
std::cout << std::boolalpha
          << std::get<pluss::Series>(out.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<bool>()[0];
```

### Output

```text
false true
```

## Notes

- Always a literal, case-sensitive comparison.
- Changes each value's kind from `Note` to `Series` (`Bool`).
- Requires the slot to be `Note`-kind — a non-`Note` slot throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`SlotNoteAccessor::starts`](starts.md): the mirror check at the
  beginning.
- [`SlotNoteAccessor::has`](has.md): a general substring/pattern search
  anywhere in the text.
- [`Slot::nt`](../slot/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="encode.html">
    <span class="label">Previous</span>
    <span class="title">encode</span>
  </a>
  <a class="next-link" href="findall.html">
    <span class="label">Next</span>
    <span class="title">findall</span>
  </a>
</div>
