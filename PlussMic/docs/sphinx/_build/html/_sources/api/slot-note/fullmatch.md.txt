<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 69c997a00558 -->

# SlotNoteAccessor::fullmatch

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signature

```cpp
Slot SlotNoteAccessor::fullmatch(std::string_view pattern) const
```

## What It Does

Tests whether each value's *entire* text matches `pattern` as a regular
expression, replacing each `Note` value with a `Bool` `Series`.

Unlike [`matches()`](matches.md), there's no `full` toggle — `fullmatch()`
always anchors the pattern to both ends of the value.

## Parameters

**pattern** (`std::string_view`)
    A regular expression the entire value must match, start to end.

## Returns

`Slot`
    A new `Series[bool]` slot.

## Examples

Example 1. A code pattern that matches one value fully and not the other.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_note("AB-123"), pluss::make_note("99XY")});
auto out = slot.nt().fullmatch("[A-Z]{2}-[0-9]{3}");
std::cout << std::boolalpha
          << std::get<pluss::Series>(out.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<bool>()[0];
```

### Output

```text
true false
```

## Notes

- Always anchored to both ends — contrast [`matches(pattern)`](matches.md)
  (unanchored by default).
- No `TextOptions` overload — use inline regex flags (e.g. `(?i)`) for
  case-insensitivity.
- Changes each value's kind from `Note` to `Series` (`Bool`).
- Requires the slot to be `Note`-kind — a non-`Note` slot throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`SlotNoteAccessor::matches`](matches.md): the more flexible sibling.
- [`SlotNoteAccessor::has`](has.md): substring/pattern presence, not a
  whole-value shape check.
- [`Slot::nt`](../slot/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="findall.html">
    <span class="label">Previous</span>
    <span class="title">findall</span>
  </a>
  <a class="next-link" href="has.html">
    <span class="label">Next</span>
    <span class="title">has</span>
  </a>
</div>
