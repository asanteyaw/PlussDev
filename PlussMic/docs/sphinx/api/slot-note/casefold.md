<!-- pluss-hand-authored -->

# SlotNoteAccessor::casefold

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signature

```cpp
Slot SlotNoteAccessor::casefold() const
```

## What It Does

Applies aggressive, locale-independent case folding to every value in the
bound slot, and returns a new slot.

```{important} casefold() is not just lower() with a different name
Case folding is designed for *case-insensitive comparison*, not display —
it can normalize characters that [`lower()`](lower.md) leaves alone (for
example, the German `ß` folds toward `ss` under full Unicode case folding,
where lowercasing leaves it as `ß`). If you're about to compare two text
values for equality regardless of case, prefer `casefold()` on both sides
over `lower()`.
```

## Parameters

`None`

## Returns

`Slot`
    A new slot, every value case-folded.

## Examples

Example 1. Case-fold two values.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("Accra City"), pluss::make_note("TEMA town")});
auto out = slot.nt().casefold();
std::cout << std::get<pluss::Note>(out.at(0)).at(0) << " | "
          << std::get<pluss::Note>(out.at(1)).at(0);
```

### Output

```text
accra city | tema town
```

## Notes

- See the important callout above for how this differs from
  [`lower()`](lower.md).
- Requires the slot to be `Note`-kind — a non-`Note` slot throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`SlotNoteAccessor::lower`](lower.md): the plain, non-comparison-oriented
  lowercasing transform.
- [`SlotNoteAccessor::upper`](upper.md) / [`titlecase()`](titlecase.md):
  the other case transforms on this accessor.
- [`Slot::nt`](../slot/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="chars.html">
    <span class="label">Next</span>
    <span class="title">chars</span>
  </a>
</div>
