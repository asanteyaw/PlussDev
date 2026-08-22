<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 070abe13d356 -->

# Slot::nt

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
SlotNoteAccessor Slot::nt() const
```

## What It Does

The entry point into **text (`Note`) operations** — binds the slot and
hands back a `SlotNoteAccessor`, documented in full on its own pages (see
[`SlotNoteAccessor` API Reference](../../slot_note_accessor_api_reference)):
case folding, trimming, pattern matching, and more.

## Parameters

`None`

## Returns

`SlotNoteAccessor`
    A binder for the current slot — call one of its methods to run a text
    operation.

## Examples

Example 1. Uppercase a `Note`-kind slot.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_note("Accra")});
auto out = slot.nt().upper();
std::cout << std::get<pluss::Note>(out.at(0)).at(0);
```

### Output

```text
ACCRA
```

## Notes

- `nt()` alone does no work — it only binds the slot; call one of the
  accessor's methods to actually transform text.
- Every method on the resulting accessor requires the slot to be
  `Note`-kind — a non-`Note` slot throws
  `"Slot::nt: slot kind must be Note"`.
- The full accessor surface is documented separately — see
  [`SlotNoteAccessor` API Reference](../../slot_note_accessor_api_reference).

## See Also

- [`Slot::cast`](cast.md) / [`Slot::tl`](tl.md): the sibling entry points
  for type casting and datetime operations.
- [`Registry::nt`](../registry/nt.md): the equivalent entry point at the
  `Registry` level.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="notnull.html">
    <span class="label">Previous</span>
    <span class="title">notnull</span>
  </a>
  <a class="next-link" href="ptype.html">
    <span class="label">Next</span>
    <span class="title">ptype</span>
  </a>
</div>
