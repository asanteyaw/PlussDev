<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2ee165b44870 -->

# SlotTimelineAccessor::quarter

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::quarter() const
```

## What It Does

Extracts the calendar quarter (`1`-`4`) of each value in the bound slot,
replacing each `Timeline` value with an `Int64` `Series`.

## Parameters

`None`

## Returns

`Slot`
    A new `Series[int64]` slot of quarter numbers.

## Examples

Example 1. Extract the quarter from a July and a March timestamp.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-07-06T14:00:00Z"), pluss::make_timeline("2023-03-01T00:00:00Z")});
auto out = slot.tl().quarter();
std::cout << std::get<pluss::Series>(out.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
3 1
```

July falls in Q3 (months 7-9); March falls in Q1 (months 1-3).

## Notes

- `1` covers January-March, `2` April-June, `3` July-September, `4`
  October-December.
- Reads local calendar fields relative to whatever timezone the value
  currently carries.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::month`](month.md): the finer-grained component
  `quarter()` is derived from.
- [`SlotTimelineAccessor::period`](period.md): group timestamps into
  arbitrary frequency-based periods, not just quarters.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="period.html">
    <span class="label">Previous</span>
    <span class="title">period</span>
  </a>
  <a class="next-link" href="round.html">
    <span class="label">Next</span>
    <span class="title">round</span>
  </a>
</div>
