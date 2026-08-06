<!-- pluss-hand-authored -->

# SlotTimelineAccessor::month_name

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::month_name() const
```

## What It Does

Renders each value in the bound slot as its full English month name,
replacing each `Timeline` value with a `Note` (text) `Series`.

## Parameters

`None`

## Returns

`Slot`
    A new `Note` slot holding each value's full month name
    (`"January"`-`"December"`).

## Examples

Example 1. Render the month name for a January and a February timestamp.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")});
auto out = slot.tl().month_name();
std::cout << std::get<pluss::Note>(out.at(0)).at(0) << " | "
          << std::get<pluss::Note>(out.at(1)).at(0);
```

### Output

```text
January | February
```

## Notes

- Always the full English name; use [`fmt("%b")`](fmt.md) for the
  abbreviated three-letter form.
- Changes each value's kind from `Timeline` to `Note`.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::month`](month.md): the same information as a
  `1`-`12` number instead of a name.
- [`SlotTimelineAccessor::weekday_name`](weekday_name.md): the
  equivalent for weekdays.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month_end.html">
    <span class="label">Previous</span>
    <span class="title">month_end</span>
  </a>
  <a class="next-link" href="month_start.html">
    <span class="label">Next</span>
    <span class="title">month_start</span>
  </a>
</div>
