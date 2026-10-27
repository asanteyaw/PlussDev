<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4cd1c5e5f5da -->

# SlotTimelineAccessor::epoch

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::epoch(pluss::table::TimeUnit unit = pluss::table::TimeUnit::Nanos) const
```

## What It Does

Extracts each value in the bound slot as a Unix epoch value in `unit`,
replacing each `Timeline` value with an `Int64` `Series`.

```{important} The default unit is Nanos, not Seconds
A bare `epoch()` call reports nanoseconds since the Unix epoch, not
seconds. Pass `TimeUnit::Seconds` explicitly for the more commonly
expected scale.
```

## Parameters

**unit** (`pluss::table::TimeUnit`, defaults to `pluss::table::TimeUnit::Nanos`)
    One of the fixed-duration units: `Nanos`, `Micros`, `Millis`,
    `Seconds`, or `Days`. Passing a span-only unit (`Weeks`, `Months`,
    `Years`, `BusinessDays`) throws.

## Returns

`Slot`
    A new `Series[int64]` slot of epoch counts in `unit`.

## Examples

Example 1. Extract the epoch second from two timestamps.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")});
auto out = slot.tl().epoch(pluss::table::TimeUnit::Seconds);
std::cout << std::get<pluss::Series>(out.at(0)).host<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).host<std::int64_t>()[0];
```

### Output

```text
1706692530 1709250300
```

Example 2. A span-only unit throws.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_timeline("2024-01-31T09:15:30Z")});
try {
  auto out = slot.tl().epoch(pluss::table::TimeUnit::Weeks);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Slate::tl().epoch: unit is span-only; use Slate::tl().span
```

## Notes

- See the important callout above — the default unit is `Nanos`.
- Only fixed-duration units are accepted; `Weeks`, `Months`, `Years`, or
  `BusinessDays` throw.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::fmt`](fmt.md) / [`iso()`](iso.md): render as
  text instead of a raw epoch count.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="day.html">
    <span class="label">Previous</span>
    <span class="title">day</span>
  </a>
  <a class="next-link" href="floor.html">
    <span class="label">Next</span>
    <span class="title">floor</span>
  </a>
</div>
