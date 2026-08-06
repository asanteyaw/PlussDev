<!-- pluss-hand-authored -->

# SlotNoteAccessor::length

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signature

```cpp
Slot SlotNoteAccessor::length() const
```

## What It Does

Measures the character length of each value in the bound slot, replacing
each `Note` value with an `Int64` `Series`.

## Parameters

`None`

## Returns

`Slot`
    A new `Series[int64]` slot of character counts.

## Examples

Example 1. Measure city name lengths.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Kumasi")});
auto out = slot.nt().length();
std::cout << std::get<pluss::Series>(out.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
5 6
```

## Notes

- Changes each value's kind from `Note` to `Series` (`Int64`).
- Requires the slot to be `Note`-kind — a non-`Note` slot throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`SlotNoteAccessor::chars`](chars.md): use a length result to compute
  slice bounds relative to the end.
- [`SlotNoteAccessor::count`](count.md): the other method on this
  accessor that turns text into an `Int64` `Series`.
- [`Slot::nt`](../slot/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="indicate.html">
    <span class="label">Previous</span>
    <span class="title">indicate</span>
  </a>
  <a class="next-link" href="lower.html">
    <span class="label">Next</span>
    <span class="title">lower</span>
  </a>
</div>
