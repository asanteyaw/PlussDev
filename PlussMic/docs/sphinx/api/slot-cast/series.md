<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: c6b57fd0dd92 -->

# SlotCastAccessor::series

[Back to SlotCastAccessor API Reference](../../slot_cast_accessor_api_reference)

## Signature

```cpp
Slot SlotCastAccessor::series() const
```

## What It Does

Casts every value in the bound slot to `Series`-kind and returns a new
slot. This behaves identically regardless of how many values the slot
holds — including exactly one — see Example 2.

## Parameters

`None`

## Returns

`Slot`
    A new slot, every value cast to `Series`-kind.

## Examples

Example 1. Cast a two-value `Note` slot of numeric-looking text to
`Series`.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_note("1"), pluss::make_note("2")});
auto out = slot.cast().series();
std::cout << std::get<pluss::Series>(out.at(0)).host<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).host<std::int64_t>()[0];
```

### Output

```text
1 2
```

Example 2. A single-value slot behaves the same way as a multi-value one —
unparseable text becomes null rather than throwing.

```cpp
auto single = pluss::scribble::Slot({pluss::make_note("Accra")});
auto out = single.cast().series();
std::cout << out.to_json();
```

### Output

```text
{
  "_type": "Slot",
  "defined": true,
  "size": 1,
  "values": [
    null
  ]
}
```

Example 3. On a two-or-more-value slot, unparseable text becomes null
rather than throwing — the same behavior as Example 2, at a larger size.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_note("Accra"), pluss::make_note("Tema")});
auto out = slot.cast().series();
std::cout << out.to_json();
```

### Output

```text
{
  "_type": "Slot",
  "defined": true,
  "size": 2,
  "values": [
    null,
    null
  ]
}
```

## Notes

- Unparseable text becomes a null value rather than throwing, regardless of
  slot size — see Examples 2 and 3.
- Bound via [`Slot::cast()`](../slot/cast.md).

## See Also

- [`SlotCastAccessor::note`](note.md): the inverse direction.
- [`Slot::cast`](../slot/cast.md): the entry point this accessor is
  reached from.
- [`RegistryCastAccessor::series`](../registry-cast/series.md): the
  field-scoped equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="note.html">
    <span class="label">Previous</span>
    <span class="title">note</span>
  </a>
</div>
