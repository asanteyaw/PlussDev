<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7b83ed964df2 -->

# SlotCastAccessor::note

[Back to SlotCastAccessor API Reference](../../slot_cast_accessor_api_reference)

## Signature

```cpp
Slot SlotCastAccessor::note() const
```

## What It Does

Casts every value in the bound slot to `Note`-kind (text) and returns a
new slot. Unlike casting toward `Series`, `Note`-conversion always
succeeds *logically* — every domain kind has a printable text form. This
holds regardless of how many values the slot holds — including exactly
one — see Example 2.

## Parameters

`None`

## Returns

`Slot`
    A new slot, every value cast to `Note`-kind.

## Examples

Example 1. Cast a two-value numeric `Series` slot to text.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2)});
auto out = slot.cast().note();
std::cout << std::get<pluss::Note>(out.at(0)).at(0) << " "
          << std::get<pluss::Note>(out.at(1)).at(0);
```

### Output

```text
1 2
```

Example 2. A single-value slot converts the same way a multi-value one
does.

```cpp
auto single = pluss::bundle::Slot({pluss::make_series(1)});
auto out = single.cast().note();
std::cout << std::get<pluss::Note>(out.at(0)).at(0);
```

### Output

```text
1
```

## Notes

- There's no silent-failure mode — any value can be rendered as text,
  regardless of slot size.
- Bound via [`Slot::cast()`](../slot/cast.md).

## See Also

- [`SlotCastAccessor::series`](series.md): the inverse direction, with a
  silent-null failure mode for unparseable text.
- [`Slot::cast`](../slot/cast.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="series.html">
    <span class="label">Next</span>
    <span class="title">series</span>
  </a>
</div>
