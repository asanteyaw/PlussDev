<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8d4387d20075 -->

# SlotTimelineAccessor::weekend

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::weekend() const
```

## What It Does

Flags whether each value in the bound slot falls on a Saturday or
Sunday, replacing each `Timeline` value with a `Bool` `Series`.

## Parameters

`None`

## Returns

`Slot`
    A new `Series[bool]` slot: `true` where the value falls on a
    weekend, `false` otherwise.

## Examples

Example 1. Flag a Saturday and a Wednesday.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_timeline("2024-07-06T14:00:00Z"), pluss::make_timeline("2023-03-01T00:00:00Z")});
auto out = slot.tl().weekend();
std::cout << std::boolalpha
          << std::get<pluss::Series>(out.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<bool>()[0];
```

### Output

```text
true false
```

`"2024-07-06"` is a Saturday; `"2023-03-01"` is a Wednesday.

## Notes

- Saturday and Sunday only — there's no configurable weekend definition.
- Reads local calendar fields relative to whatever timezone the value
  currently carries.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::weekday`](weekday.md): the underlying weekday
  number this method is derived from.
- [`SlotTimelineAccessor::weekday_name`](weekday_name.md): the weekday
  spelled out as a full English name.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekday_name.html">
    <span class="label">Previous</span>
    <span class="title">weekday_name</span>
  </a>
  <a class="next-link" href="year.html">
    <span class="label">Next</span>
    <span class="title">year</span>
  </a>
</div>
