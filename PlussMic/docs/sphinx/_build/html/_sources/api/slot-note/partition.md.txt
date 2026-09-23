<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0e0119463168 -->

# SlotNoteAccessor::partition

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signature

```cpp
pluss::table::DataFrame SlotNoteAccessor::partition(std::string_view separator,
                                                    std::string prefix = "part") const
```

## What It Does

Splits each value in the bound slot at the *first* occurrence of a
literal `separator`, always producing exactly **three** output features:
the text before, the separator itself, and the text after. Narrower and
simpler than [`split()`](split.md): always literal (no regex), always
stops at the first match, always exactly three features.

```{important} A missing separator produces empty strings, not NULL
When `separator` doesn't occur in a value at all, that value's middle and
third features are the **empty string** `""`, not `NULL` — the value
itself is still valid.
```

## Parameters

**separator** (`std::string_view`)
    The literal text to split on. Must not be empty.

**prefix** (`std::string`, defaults to `"part"`)
    Base name for the three output features: `<prefix>_0` (before),
    `<prefix>_1` (the separator, or `""` if not found), `<prefix>_2`
    (after, or `""` if not found).

## Returns

`pluss::table::DataFrame`
    A new frame with exactly three features: `<prefix>_0`, `<prefix>_1`,
    `<prefix>_2`.

## Examples

Example 1. The not-found case's empty strings.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_note("key=value"), pluss::make_note("no-equals")});
auto out = slot.nt().partition("=");
std::cout << out;
```

### Output

```text
================================
     | part_0     part_1  part_2
--------------------------------
0    | key        =       value 
1    | no-equals                
--------------------------------
[2 cs x 3 feat]
```

The first value splits cleanly. The second has no `=` — its
`part_1`/`part_2` cells are empty strings, not `NULL`.

## Notes

- Always literal (no regex) and always stops at the *first* occurrence
  — for a regex-aware or multi-way split, use [`split()`](split.md).
- Always produces exactly three output features.
- An empty `separator` throws `"Thread::partition: separator cannot be
  empty"`.
- Requires the slot to be `Note`-kind — a non-`Note` slot throws.

## See Also

- [`SlotNoteAccessor::split`](split.md): the more general, possibly-regex,
  unlimited-pieces sibling.
- [`Slot::nt`](../slot/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pad.html">
    <span class="label">Previous</span>
    <span class="title">pad</span>
  </a>
  <a class="next-link" href="replace.html">
    <span class="label">Next</span>
    <span class="title">replace</span>
  </a>
</div>
