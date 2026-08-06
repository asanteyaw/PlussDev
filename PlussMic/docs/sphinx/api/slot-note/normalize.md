<!-- pluss-hand-authored -->

# SlotNoteAccessor::normalize

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signature

```cpp
Slot SlotNoteAccessor::normalize(pluss::table::UnicodeForm form = pluss::table::UnicodeForm::NFC) const
```

## What It Does

Applies Unicode normalization to every value in the bound slot, returning
a new slot.

```cpp
enum class UnicodeForm : std::uint8_t {
  NFC = 0,
  NFD = 1,
  NFKC = 2,
  NFKD = 3,
};
```

## Parameters

**form** (`pluss::table::UnicodeForm`, defaults to `pluss::table::UnicodeForm::NFC`)
    Which of the four standard Unicode normalization forms to apply.

## Returns

`Slot`
    A new slot, every value normalized.

## Examples

Example 1. Default call normalizes to `NFC`.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("cafe")});
auto out = slot.nt().normalize();
std::cout << std::get<pluss::Note>(out.at(0)).at(0);
```

### Output

```text
cafe
```

Plain ASCII text has no composed/decomposed distinction to make, so it
round-trips unchanged — the visible difference shows up on text
containing combining marks or compatibility characters.

## Notes

- Defaults to `NFC`, the most common form for storage and comparison.
- Requires the slot to be `Note`-kind — a non-`Note` slot throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`SlotNoteAccessor::casefold`](casefold.md): another comparison-oriented
  transform, applied to case rather than composition.
- [`SlotNoteAccessor::encode`](encode.md): the other byte/text-level
  transform.
- [`Slot::nt`](../slot/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="matches.html">
    <span class="label">Previous</span>
    <span class="title">matches</span>
  </a>
  <a class="next-link" href="pad.html">
    <span class="label">Next</span>
    <span class="title">pad</span>
  </a>
</div>
