<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a369ff2c23e1 -->

# SlotTimelineAccessor::valid

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::valid() const
```

## What It Does

Flags whether each value in the bound slot is a real, present timestamp
(not `NaT`), replacing each `Timeline` value with a `Bool` `Series`.
Exactly the logical inverse of [`isnat()`](isnat.md).

## Parameters

`None`

## Returns

`Slot`
    A new `Series[bool]` slot: `true` where the value is a real
    timestamp, `false` where it's `NaT`.

## Examples

Example 1. Flag a valid timestamp alongside a `NaT` one.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline(""), pluss::make_timeline("2024-01-01T00:00:00Z")});
auto out = slot.tl().valid();
std::cout << std::boolalpha
          << std::get<pluss::Series>(out.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<bool>()[0];
```

### Output

```text
false true
```

## Notes

- Exactly `!isnat()`.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::isnat`](isnat.md): the logical inverse.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="utc.html">
    <span class="label">Previous</span>
    <span class="title">utc</span>
  </a>
  <a class="next-link" href="week.html">
    <span class="label">Next</span>
    <span class="title">week</span>
  </a>
</div>
