<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 68bdbe18ac4f -->

# RegistryTimelineAccessor::month_end

[Back to RegistryTimelineAccessor API Reference](../../registry_timeline_accessor_api_reference)

## Signature

```cpp
Registry RegistryTimelineAccessor::month_end() const
```

## What It Does

Flags whether each bound field's timestamp falls on the last day of its
calendar month, replacing each `Timeline` field with a `Bool` `Series` field.

## Parameters

`None`

## Returns

`Registry`
    A new registry with every bound field replaced by a `Bool` `Series`
    field: `true` where the row's day-of-month is that month's last valid
    day, `false` otherwise.

## Examples

Example 1. Both sample timestamps are the last day of their respective
month.

```cpp
auto rg = pluss::bundle::Registry({
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})},
});

auto out = rg.fields({"ts"}).tl().month_end();
auto ts = out.fields("ts");
std::cout << std::boolalpha
          << std::get<pluss::Series>(ts.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(ts.at(1)).to_vec<bool>()[0];
```

### Output

```text
true true
```

`"2024-01-31"` is January's last day; `"2024-02-29"` is February's last day
in this leap year — see [`leap()`](leap.md), since the correct "last day" for
February depends on whether the year is a leap year.

## Notes

- Correctly accounts for each month's actual length, including leap-year
  February — see the important callout on [`leap()`](leap.md).
- Only checks the date component; the time-of-day portion doesn't affect the
  result.
- Reads local calendar fields relative to whatever timezone the field's
  `Timeline` currently carries (`"UTC"` unless changed via
  [`tz()`](tz.md)/[`localize()`](localize.md)).
- Requires every bound field to be `Timeline`-kind — a non-`Timeline` field
  throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`RegistryTimelineAccessor::month_start`](month_start.md): the mirror check
  at the beginning of the month.
- [`RegistryTimelineAccessor::leap`](leap.md): where leap-year status
  actually changes February's last valid day.
- [`Registry::tl`](../registry/tl.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month.html">
    <span class="label">Previous</span>
    <span class="title">month</span>
  </a>
  <a class="next-link" href="month_name.html">
    <span class="label">Next</span>
    <span class="title">month_name</span>
  </a>
</div>
