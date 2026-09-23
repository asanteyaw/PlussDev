<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a2fd1b0dc827 -->

# SlotTimelineAccessor::week

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::week() const
```

## What It Does

Extracts the ISO week-of-year (`1`-`53`) of each value in the bound
slot, replacing each `Timeline` value with an `Int64` `Series`.

## Parameters

`None`

## Returns

`Slot`
    A new `Series[int64]` slot of ISO week numbers.

## Examples

Example 1. Extract the ISO week from a January and a February timestamp.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")});
auto out = slot.tl().week();
std::cout << std::get<pluss::Series>(out.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
5 9
```

Example 2. A second pair of timestamps, showing the ISO week for
mid-year dates.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_timeline("2024-07-06T14:00:00Z"), pluss::make_timeline("2023-03-01T00:00:00Z")});
auto out = slot.tl().week();
std::cout << std::get<pluss::Series>(out.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
27 9
```

## Notes

- Uses ISO week numbering — weeks start on Monday, and week `1` is the
  week containing the year's first Thursday, so early-January dates can
  land in the *previous* year's final week under ISO rules.
- Reads local calendar fields relative to whatever timezone the value
  currently carries.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::yearday`](yearday.md): the finer-grained
  day-of-year component.
- [`SlotTimelineAccessor::weekday`](weekday.md): the day-of-week within
  that week.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="valid.html">
    <span class="label">Previous</span>
    <span class="title">valid</span>
  </a>
  <a class="next-link" href="weekday.html">
    <span class="label">Next</span>
    <span class="title">weekday</span>
  </a>
</div>
