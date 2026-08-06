<!-- pluss-hand-authored -->

# SlotNoteAccessor::pad

[Back to SlotNoteAccessor API Reference](../../slot_note_accessor_api_reference)

## Signature

```cpp
Slot SlotNoteAccessor::pad(std::int64_t width,
                           pluss::table::TextAlign align = pluss::table::TextAlign::Left,
                           char fill = ' ') const
```

## What It Does

Pads every value in the bound slot out to at least `width` characters,
using `fill` and the alignment given by `align`.

```cpp
enum class TextAlign : std::uint8_t {
  Left = 0,
  Right = 1,
  Center = 2,
};
```

`align` describes where the *original text* ends up, not where the
padding goes.

## Parameters

**width** (`std::int64_t`)
    The minimum total length of the result.

**align** (`pluss::table::TextAlign`, defaults to `pluss::table::TextAlign::Left`)
    Where the original text sits relative to the added padding.

**fill** (`char`, defaults to `' '`)
    The character used to pad.

## Returns

`Slot`
    A new slot, every value padded to `width`.

## Examples

Example 1. Default call: left-align, pad with spaces on the right.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("7")});
auto out = slot.nt().pad(4);
std::cout << "'" << std::get<pluss::Note>(out.at(0)).at(0) << "'";
```

### Output

```text
'7   '
```

Example 2. Right-align with a `'0'` fill.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("7")});
auto out = slot.nt().pad(4, pluss::table::TextAlign::Right, '0');
std::cout << std::get<pluss::Note>(out.at(0)).at(0);
```

### Output

```text
0007
```

Example 3. Center alignment splits the padding across both sides.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("7")});
auto out = slot.nt().pad(5, pluss::table::TextAlign::Center);
std::cout << "'" << std::get<pluss::Note>(out.at(0)).at(0) << "'";
```

### Output

```text
'  7  '
```

## Notes

- Values already at or beyond `width` characters pass through unchanged.
- Requires the slot to be `Note`-kind — a non-`Note` slot throws
  `"Slot::nt: slot kind must be Note"`.

## See Also

- [`SlotNoteAccessor::strip`](strip.md) / [`lstrip()`](lstrip.md) /
  [`rstrip()`](rstrip.md): the inverse operation family.
- [`Slot::nt`](../slot/nt.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="normalize.html">
    <span class="label">Previous</span>
    <span class="title">normalize</span>
  </a>
  <a class="next-link" href="partition.html">
    <span class="label">Next</span>
    <span class="title">partition</span>
  </a>
</div>
