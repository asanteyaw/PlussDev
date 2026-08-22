<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 97bdbfb7572e -->

# SlotNoteAccessor::upper

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signature

```cpp
Slot SlotNoteAccessor::upper() const
```

## What It Does

Uppercases every value in the bound slot and returns a new slot.

## Parameters

`None`

## Returns

`Slot`
    A new slot, every value uppercased.

## Examples

Example 1. Uppercase two values.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")});
auto out = slot.nt().upper();
std::cout << std::get<pluss::Note>(out.at(0)).at(0) << " "
          << std::get<pluss::Note>(out.at(1)).at(0);
```

### Output

```text
ACCRA TEMA
```

## Notes

- Requires the slot to be `Note`-kind — a non-`Note` slot throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`SlotNoteAccessor::lower`](lower.md): the inverse transform.
- [`SlotNoteAccessor::titlecase`](titlecase.md): capitalizes only the
  first letter of each word instead.
- [`Slot::nt`](../slot/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="titlecase.html">
    <span class="label">Previous</span>
    <span class="title">titlecase</span>
  </a>
</div>
