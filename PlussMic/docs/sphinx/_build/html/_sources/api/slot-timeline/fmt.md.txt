<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 425e1f548cfa -->

# SlotTimelineAccessor::fmt

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::fmt(std::string_view pattern) const
```

## What It Does

Renders each value in the bound slot as custom text built from
`pattern`, replacing each `Timeline` value with a `Note` (text) `Series`.

## Parameters

**pattern** (`std::string_view`)
    A `strftime`-style format string. Recognized tokens: `%Y` (4-digit
    year), `%m`/`%d` (2-digit month/day), `%H`/`%M`/`%S` (2-digit
    hour/minute/second), `%f` (9-digit zero-padded nanoseconds), `%z`
    (UTC offset as `+HH:MM`/`-HH:MM`), `%a`/`%A` (abbreviated/full
    weekday name), `%b`/`%B` (abbreviated/full month name), and `%%` (a
    literal `%`). Cannot be empty.

## Returns

`Slot`
    A new `Note` slot holding each value's text rendered per `pattern`.

## Examples

Example 1. Format two timestamps with a custom day/month-name/year
pattern.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")});
auto out = slot.tl().fmt("%d %b %Y");
std::cout << std::get<pluss::Note>(out.at(0)).at(0) << " | "
          << std::get<pluss::Note>(out.at(1)).at(0);
```

### Output

```text
31 Jan 2024 | 29 Feb 2024
```

## Notes

- `pattern` cannot be empty — an empty pattern throws.
- `%f` always emits all 9 nanosecond digits; `%z` always renders as a
  signed `HH:MM` offset, never a bare `"Z"`.
- Changes each value's kind from `Timeline` to `Note`.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::iso`](iso.md): a fixed ISO-8601-with-offset
  pattern, equivalent to `fmt("%Y-%m-%dT%H:%M:%S.%f%z")`.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="floor.html">
    <span class="label">Previous</span>
    <span class="title">floor</span>
  </a>
  <a class="next-link" href="hour.html">
    <span class="label">Next</span>
    <span class="title">hour</span>
  </a>
</div>
