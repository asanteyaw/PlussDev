<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0141c1a0304e -->

# SlotNoteAccessor::lower

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signature

```cpp
Slot SlotNoteAccessor::lower() const
```

## What It Does

Lowercases every value in the bound slot and returns a new slot.

## Parameters

`None`

## Returns

`Slot`
    A new slot, every value lowercased.

## Examples

Example 1. Lowercase two values.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("Accra City"), pluss::make_note("TEMA town")});
auto out = slot.nt().lower();
std::cout << std::get<pluss::Note>(out.at(0)).at(0) << " | "
          << std::get<pluss::Note>(out.at(1)).at(0);
```

### Output

```text
accra city | tema town
```

## Notes

- For case-insensitive *comparison* rather than display, prefer
  [`casefold()`](casefold.md).
- Requires the slot to be `Note`-kind — a non-`Note` slot throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`SlotNoteAccessor::upper`](upper.md): the inverse transform.
- [`SlotNoteAccessor::casefold`](casefold.md): the comparison-oriented
  case-folding transform.
- [`SlotNoteAccessor::titlecase`](titlecase.md): capitalizes the first
  letter of each word instead.
- [`Slot::nt`](../slot/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="length.html">
    <span class="label">Previous</span>
    <span class="title">length</span>
  </a>
  <a class="next-link" href="lstrip.html">
    <span class="label">Next</span>
    <span class="title">lstrip</span>
  </a>
</div>
