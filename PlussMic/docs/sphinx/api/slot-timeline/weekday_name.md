<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2d6bb4e0082e -->

# SlotTimelineAccessor::weekday_name

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::weekday_name() const
```

## What It Does

Renders each value in the bound slot as its full English weekday name,
replacing each `Timeline` value with a `Note` (text) `Series`.

## Parameters

`None`

## Returns

`Slot`
    A new `Note` slot holding each value's full weekday name
    (`"Monday"`-`"Sunday"`).

## Examples

Example 1. Render the weekday name for a Wednesday and a Thursday.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")});
auto out = slot.tl().weekday_name();
std::cout << std::get<pluss::Note>(out.at(0)).at(0) << " | "
          << std::get<pluss::Note>(out.at(1)).at(0);
```

### Output

```text
Wednesday | Thursday
```

## Notes

- Always the full English name; use [`fmt("%a")`](fmt.md) for the
  abbreviated three-letter form.
- Changes each value's kind from `Timeline` to `Note`.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::weekday`](weekday.md): the same information as
  a `0`-`6` number instead of a name.
- [`SlotTimelineAccessor::month_name`](month_name.md): the equivalent for
  months.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekday.html">
    <span class="label">Previous</span>
    <span class="title">weekday</span>
  </a>
  <a class="next-link" href="weekend.html">
    <span class="label">Next</span>
    <span class="title">weekend</span>
  </a>
</div>
