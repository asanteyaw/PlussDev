<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c271f51d1d3c -->

# SlotNoteAccessor::chars

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signature

```cpp
Slot SlotNoteAccessor::chars(std::int64_t start,
                             std::optional<std::int64_t> stop = std::nullopt,
                             std::int64_t step = 1) const
```

## What It Does

Slices every value in the bound slot by character position —
Python-style `text[start:stop:step]` semantics, including negative
indices counting from the end.

## Parameters

**start** (`std::int64_t`)
    The starting character position, inclusive. Negative values count
    from the end (`-1` is the last character).

**stop** (`std::optional<std::int64_t>`, defaults to `std::nullopt`)
    The ending character position, exclusive. `std::nullopt` (the
    default) slices through to the end.

**step** (`std::int64_t`, defaults to `1`)
    The stride between characters kept in the slice.

## Returns

`Slot`
    A new slot, every value replaced by the sliced text.

## Examples

Example 1. Take the first two characters of each value.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Kumasi")});
auto out = slot.nt().chars(0, 2);
std::cout << std::get<pluss::Note>(out.at(0)).at(0) << " "
          << std::get<pluss::Note>(out.at(1)).at(0);
```

### Output

```text
Ac Ku
```

Example 2. A negative `start` slices from the end.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_note("Accra")});
auto out = slot.nt().chars(-2);
std::cout << std::get<pluss::Note>(out.at(0)).at(0);
```

### Output

```text
ra
```

Example 3. `step` keeps every Nth character, with no `stop` bound.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_note("Accra")});
auto out = slot.nt().chars(0, std::nullopt, 2);
std::cout << std::get<pluss::Note>(out.at(0)).at(0);
```

### Output

```text
Aca
```

## Notes

- `stop` defaults to `std::nullopt` (slice to the end), not to `start`'s
  value.
- Requires the slot to be `Note`-kind — a non-`Note` slot throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`SlotNoteAccessor::length`](length.md): each value's character count.
- [`SlotNoteAccessor::strip`](strip.md): trim edges instead of slicing.
- [`Slot::nt`](../slot/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="casefold.html">
    <span class="label">Previous</span>
    <span class="title">casefold</span>
  </a>
  <a class="next-link" href="count.html">
    <span class="label">Next</span>
    <span class="title">count</span>
  </a>
</div>
