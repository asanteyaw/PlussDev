<!-- pluss-hand-authored -->

# SlotNoteAccessor::titlecase

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signature

```cpp
Slot SlotNoteAccessor::titlecase() const
```

## What It Does

Capitalizes the first letter of each word in every value of the bound
slot and lowercases the rest, returning a new slot.

## Parameters

`None`

## Returns

`Slot`
    A new slot, every value titlecased.

## Examples

Example 1. Titlecase a mixed-case value.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("Accra City"), pluss::make_note("TEMA town")});
auto out = slot.nt().titlecase();
std::cout << std::get<pluss::Note>(out.at(0)).at(0) << " | "
          << std::get<pluss::Note>(out.at(1)).at(0);
```

### Output

```text
Accra City | Tema Town
```

## Notes

- Requires the slot to be `Note`-kind — a non-`Note` slot throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`SlotNoteAccessor::lower`](lower.md) / [`upper()`](upper.md): the
  simpler, whole-string case transforms.
- [`SlotNoteAccessor::casefold`](casefold.md): the comparison-oriented
  case-folding transform.
- [`Slot::nt`](../slot/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="strip.html">
    <span class="label">Previous</span>
    <span class="title">strip</span>
  </a>
  <a class="next-link" href="upper.html">
    <span class="label">Next</span>
    <span class="title">upper</span>
  </a>
</div>
