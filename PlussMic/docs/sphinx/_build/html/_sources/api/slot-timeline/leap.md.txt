<!-- pluss-hand-authored -->

# SlotTimelineAccessor::leap

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::leap() const
```

## What It Does

Flags whether each value's calendar year in the bound slot is a leap
year, replacing each `Timeline` value with a `Bool` `Series`.

```{important} Reflects the value's YEAR, not whether the date is February 29th
`leap()` answers "is this value's *year* a leap year," not "is this
value February 29th." Any value dated anywhere in 2024 flags `true`,
since 2024 as a whole is a leap year — see Example 1, where a January
date and a February-29th date both flag `true` for the same reason.
```

## Parameters

`None`

## Returns

`Slot`
    A new `Series[bool]` slot: `true` when the value's calendar year is a
    leap year, `false` otherwise.

## Examples

Example 1. Two timestamps from the same leap year both flag `true`,
even though only one falls on February 29th.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")});
auto out = slot.tl().leap();
std::cout << std::boolalpha
          << std::get<pluss::Series>(out.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<bool>()[0];
```

### Output

```text
true true
```

Example 2. A 2024 timestamp flags `true`; a 2023 timestamp flags
`false`.

```cpp
auto slot = pluss::bundle::Slot({pluss::make_timeline("2024-07-06T14:00:00Z"), pluss::make_timeline("2023-03-01T00:00:00Z")});
auto out = slot.tl().leap();
std::cout << std::boolalpha
          << std::get<pluss::Series>(out.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<bool>()[0];
```

### Output

```text
true false
```

## Notes

- See the important callout above.
- Reads local calendar fields relative to whatever timezone the value
  currently carries.
- Requires the slot to be `Timeline`-kind — a non-`Timeline` slot throws
  `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`SlotTimelineAccessor::month_end`](month_end.md): where leap-year
  status actually changes February's last valid day.
- [`SlotTimelineAccessor::yearday`](yearday.md): where leap years shift
  every day-of-year number from March onward.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="iso.html">
    <span class="label">Previous</span>
    <span class="title">iso</span>
  </a>
  <a class="next-link" href="localize.html">
    <span class="label">Next</span>
    <span class="title">localize</span>
  </a>
</div>
