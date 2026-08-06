<!-- pluss-hand-authored -->

# SlotCastAccessor::timeindex

[Back to SlotCastAccessor API Reference](../../slot_cast_accessor_api_reference)

## Signature

```cpp
Slot SlotCastAccessor::timeindex() const
```

## What It Does

Casts every value in the bound slot to `TimeIndex`-kind and returns a new
slot. Behaves the same as [`timeline()`](timeline.md), including throwing
(rather than nulling) on unparseable text, regardless of slot size — see
Example 2.

## Parameters

`None`

## Returns

`Slot`
    A new slot, every value cast to `TimeIndex`-kind.

## Examples

Example 1. Cast a two-value `Note` field of date-looking text to
`TimeIndex`.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_note("2024-01-01"), pluss::make_note("2024-01-02")});
auto out = slot.cast().timeindex();
std::cout << out.to_json();
```

### Output

```text
{
  "_type": "Slot",
  "defined": true,
  "size": 2,
  "values": [
    "TimeIndex[size=1]",
    "TimeIndex[size=1]"
  ]
}
```

Example 2. A single-value slot converts the same way a multi-value one
does.

```cpp
auto single = pluss::bundle::Slot({pluss::make_note("2024-01-01")});
auto out = single.cast().timeindex();
std::cout << out.to_json();
```

### Output

```text
{
  "_type": "Slot",
  "defined": true,
  "size": 1,
  "values": [
    "TimeIndex[size=1]"
  ]
}
```

## Notes

- `TimeIndex`'s `to_json()` type tag reports `"TimeIndex[size=N]"`,
  distinct from `Timeline`'s `"Timeline[numel=N]"`.
- Bound via [`Slot::cast()`](../slot/cast.md).

## See Also

- [`SlotCastAccessor::timeline`](timeline.md): the related cast target,
  with the same behavior.
- [`SlotCastAccessor::series`](series.md): the numeric cast target, with a
  different (silent-null) failure mode.
- [`Slot::cast`](../slot/cast.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="series.html">
    <span class="label">Previous</span>
    <span class="title">series</span>
  </a>
  <a class="next-link" href="timeline.html">
    <span class="label">Next</span>
    <span class="title">timeline</span>
  </a>
</div>
