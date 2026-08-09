<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 260907c913b7 -->

# SlotTimelineAccessor::floor

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::floor(std::string_view unit) const
```

## What It Does

Rounds each value in the bound slot *down* to the nearest `unit`
boundary, replacing each `Timeline` value's values in place (stays
`Timeline`-kind).

## Parameters

**unit** (`std::string_view`)
    One of `"S"`/`"s"`/`"sec"`/`"second"`, `"min"`/`"m"`/`"minute"`,
    `"H"`/`"h"`/`"hour"`, or `"D"`/`"d"`/`"day"`. Cannot be empty.

## Returns

`Slot`
    A new slot with each value floored to `unit`.

## Examples

Example 1. Floor two timestamps to the nearest hour.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")});
auto out = slot.tl().floor("H");
auto fmted = out.tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << std::get<pluss::Note>(fmted.at(0)).at(0) << " | "
          << std::get<pluss::Note>(fmted.at(1)).at(0);
```

### Output

```text
2024-01-31 09:00:00 | 2024-02-29 23:00:00
```

Example 2. An unsupported unit throws.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z")});
try {
  auto out = slot.tl().floor("bogus");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Timeline: unsupported unit (expected S/min/H/D)
```

## Notes

- `unit` cannot be empty, and only accepts the fixed clock units listed
  above.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::ceil`](ceil.md) / [`round()`](round.md): round
  up, or to the nearest boundary, instead of always down.
- [`SlotTimelineAccessor::date`](date.md): shorthand for `floor("D")`.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="epoch.html">
    <span class="label">Previous</span>
    <span class="title">epoch</span>
  </a>
  <a class="next-link" href="fmt.html">
    <span class="label">Next</span>
    <span class="title">fmt</span>
  </a>
</div>
