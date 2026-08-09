<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2afb0d65945d -->

# SlotTimelineAccessor::index

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::index(bool require_sorted = true) const
```

## What It Does

Converts the bound slot from `Timeline`-kind to `TimeIndex`-kind.

## Parameters

**require_sorted** (`bool`, defaults to `true`)
    When `true` (the default), every value in the slot must be
    non-decreasing (and non-`NaT`), or the call throws. When `false`, the
    conversion happens without checking order or `NaT`.

## Returns

`Slot`
    A new slot converted from `Timeline`-kind to `TimeIndex`-kind.

## Examples

Example 1. Convert a sorted `Timeline` slot to `TimeIndex`.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-01-01T00:00:00Z"), pluss::make_timeline("2024-01-02T00:00:00Z")});
auto out = slot.tl().index();
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

Example 2. An out-of-order slot throws under the default
`require_sorted = true`.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-01-02T00:00:00Z"), pluss::make_timeline("2024-01-01T00:00:00Z")});
try {
  auto out = slot.tl().index();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
TimeIndex must be sorted (non-decreasing)
```

## Notes

- `require_sorted = false` skips the check entirely — use it only when
  the slot's order doesn't matter for what comes next.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.
- Each converted value becomes its own single-element `TimeIndex`
  (`"TimeIndex[size=1]"` per row, as seen in Example 1's `to_json()`
  output) — a `Slot` stores one independent value per row rather than one
  shared column, so `index()` on an `N`-row `Timeline` slot produces `N`
  separate one-element `TimeIndex` values, not one `N`-element
  `TimeIndex`.

## See Also

- [`Slot::from_kindle`](../slot/from_kindle.md): the round-trip partner
  that reconstructs `TimeIndex` values from a `kindle::Stream`.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hour.html">
    <span class="label">Previous</span>
    <span class="title">hour</span>
  </a>
  <a class="next-link" href="isnat.html">
    <span class="label">Next</span>
    <span class="title">isnat</span>
  </a>
</div>
