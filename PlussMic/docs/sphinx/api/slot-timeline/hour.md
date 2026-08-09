<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4c724ae67728 -->

# SlotTimelineAccessor::hour

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::hour() const
```

## What It Does

Extracts the hour-of-day (`0`-`23`) of each value in the bound slot,
replacing each `Timeline` value with an `Int64` `Series`.

## Parameters

`None`

## Returns

`Slot`
    A new `Series[int64]` slot of hour numbers.

## Examples

Example 1. Extract the hour from a morning and a late-night timestamp.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")});
auto out = slot.tl().hour();
std::cout << std::get<pluss::Series>(out.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
9 23
```

## Notes

- 24-hour clock: `0` is midnight, `23` is 11 PM.
- Reads local calendar fields relative to whatever timezone the value
  currently carries.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::minute`](minute.md) / [`second()`](second.md):
  the finer-grained time-of-day components.
- [`SlotTimelineAccessor::time`](time.md): the whole time-of-day portion
  at once.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fmt.html">
    <span class="label">Previous</span>
    <span class="title">fmt</span>
  </a>
  <a class="next-link" href="index.html">
    <span class="label">Next</span>
    <span class="title">index</span>
  </a>
</div>
