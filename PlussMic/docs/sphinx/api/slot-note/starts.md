<!-- pluss-hand-authored -->

# SlotNoteAccessor::starts

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signature

```cpp
Slot SlotNoteAccessor::starts(std::string_view prefix) const
```

## What It Does

Checks whether each value in the bound slot starts with `prefix`,
replacing each `Note` value with a `Bool` `Series`.

## Parameters

**prefix** (`std::string_view`)
    The literal text to check for at the start of each value. Always a
    literal comparison.

## Returns

`Slot`
    A new `Series[bool]` slot.

## Examples

Example 1. Check which city names start with `"Ac"`.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")});
auto out = slot.nt().starts("Ac");
std::cout << std::boolalpha
          << std::get<pluss::Series>(out.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<bool>()[0];
```

### Output

```text
true false
```

## Notes

- Always a literal, case-sensitive comparison.
- Changes each value's kind from `Note` to `Series` (`Bool`).
- Requires the slot to be `Note`-kind — a non-`Note` slot throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`SlotNoteAccessor::ends`](ends.md): the mirror check at the end.
- [`SlotNoteAccessor::has`](has.md): a general substring/pattern search
  anywhere in the text.
- [`Slot::nt`](../slot/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="split.html">
    <span class="label">Previous</span>
    <span class="title">split</span>
  </a>
  <a class="next-link" href="strip.html">
    <span class="label">Next</span>
    <span class="title">strip</span>
  </a>
</div>
