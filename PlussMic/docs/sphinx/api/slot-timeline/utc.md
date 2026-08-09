<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 94066cd9035a -->

# SlotTimelineAccessor::utc

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::utc() const
```

## What It Does

Shorthand for [`tz("UTC")`](tz.md) — relabels each value's display
timezone back to `"UTC"` without changing the underlying instant.

## Parameters

`None`

## Returns

`Slot`
    A new slot with every value relabeled to `"UTC"` display. Reading
    the result back through any further `.tl()` call reflects the
    `"UTC"` relabeling.

## Examples

Example 1. `tz("America/New_York")` relabels the display timezone
(visible on the first `fmt()` read below); a follow-up `utc()` relabels
it back to `"UTC"`, and that too is visible on the next read — the
instant itself never changes across either call.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z")});

auto ny = slot.tl().tz("America/New_York");
std::cout << "NY-relabeled: "
          << std::get<pluss::Note>(ny.tl().fmt("%Y-%m-%d %H:%M:%S").at(0)).at(0) << "\n";

auto out = ny.tl().utc();
std::cout << "UTC-relabeled: "
          << std::get<pluss::Note>(out.tl().fmt("%Y-%m-%d %H:%M:%S").at(0)).at(0);
```

### Output

```text
NY-relabeled: 2024-01-31 04:15:30
UTC-relabeled: 2024-01-31 09:15:30
```

## Notes

- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.
- Internally, a `Slot` stores each row's `Timeline` value independently
  (one scalar `Timeline` per row, not one shared column); the resulting
  per-row tz labels round-trip correctly through the `kindle::Stream`
  boundary every further `.tl()` call goes through, the same as
  [`tz()`](tz.md).

## See Also

- [`SlotTimelineAccessor::tz`](tz.md): the general form this method is a
  fixed `"UTC"` shorthand for.
- [`SlotTimelineAccessor::localize`](localize.md): the operation that
  actually changes the underlying instant.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tz.html">
    <span class="label">Previous</span>
    <span class="title">tz</span>
  </a>
  <a class="next-link" href="valid.html">
    <span class="label">Next</span>
    <span class="title">valid</span>
  </a>
</div>
