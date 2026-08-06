<!-- pluss-hand-authored -->

# SlotTimelineAccessor::iso

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::iso() const
```

## What It Does

Renders each value in the bound slot as a fixed ISO-8601-with-offset
text representation, replacing each `Timeline` value with a `Note`
(text) `Series`. Equivalent to [`fmt("%Y-%m-%dT%H:%M:%S.%f%z")`](fmt.md).

## Parameters

`None`

## Returns

`Slot`
    A new `Note` slot holding each value as
    `"YYYY-MM-DDTHH:MM:SS.fffffffff+HH:MM"`.

## Examples

Example 1. Render a timestamp as ISO-8601 text.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z")});
auto out = slot.tl().iso();
std::cout << std::get<pluss::Note>(out.at(0)).at(0);
```

### Output

```text
2024-01-31T09:15:30.000000000+00:00
```

## Notes

- Always includes the full 9-digit nanosecond fraction and a signed
  `%z` offset.
- Changes each value's kind from `Timeline` to `Note`.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::fmt`](fmt.md): the general form this method is
  a fixed pattern of.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnat.html">
    <span class="label">Previous</span>
    <span class="title">isnat</span>
  </a>
  <a class="next-link" href="leap.html">
    <span class="label">Next</span>
    <span class="title">leap</span>
  </a>
</div>
