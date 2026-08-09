<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7dfa7252bad3 -->

# Slot::tl

[Back to Slot API Reference](../../slot_api_reference)

## Signature

```cpp
SlotTimelineAccessor Slot::tl() const
```

## What It Does

The entry point into **datetime operations** — binds the slot and hands
back a `SlotTimelineAccessor`, documented in full on its own pages (see
[`SlotTimelineAccessor` API Reference](../../slot_timeline_accessor_api_reference)):
calendar-field extraction, formatting, timezone handling, and more.

## Parameters

`None`

## Returns

`SlotTimelineAccessor`
    A binder for the current slot — call one of its methods to run a
    datetime operation.

## Examples

Example 1. Extract the calendar year from a `Timeline`-kind slot.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z")});
auto out = slot.tl().year();
std::cout << std::get<pluss::Series>(out.at(0)).to_vec<std::int64_t>()[0];
```

### Output

```text
2024
```

## Notes

- `tl()` alone does no work — it only binds the slot; call one of the
  accessor's methods to actually run a datetime operation.
- Every method on the resulting accessor requires the slot to be
  `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.
- The full accessor surface is documented separately — see
  [`SlotTimelineAccessor` API Reference](../../slot_timeline_accessor_api_reference).

## See Also

- [`Slot::cast`](cast.md) / [`Slot::nt`](nt.md): the sibling entry points
  for type casting and text operations.
- [`Registry::tl`](../registry/tl.md): the equivalent entry point at the
  `Registry` level.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="take.html">
    <span class="label">Previous</span>
    <span class="title">take</span>
  </a>
  <a class="next-link" href="to_json.html">
    <span class="label">Next</span>
    <span class="title">to_json</span>
  </a>
</div>
