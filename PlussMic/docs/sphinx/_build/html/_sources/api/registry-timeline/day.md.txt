<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 707c8aa3c241 -->

# RegistryTimelineAccessor::day

[Back to RegistryTimelineAccessor API Reference](../../registry_timeline_accessor_api_reference)

## Signature

```cpp
Registry RegistryTimelineAccessor::day() const
```

## What It Does

Extracts the day-of-month (`1`-`31`) of each bound field's timestamp,
replacing each `Timeline` field with an `Int64` `Series` field.

## Parameters

`None`

## Returns

`Registry`
    A new registry with every bound field replaced by an `Int64` `Series`
    field of day-of-month numbers; every unbound field unchanged.

## Examples

Example 1. Extract the day-of-month from two end-of-month timestamps.

```cpp
auto rg = pluss::scribble::Registry({
    {"ts", pluss::scribble::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})},
});

auto out = rg.fields({"ts"}).tl().day();
auto ts = out.fields("ts");
std::cout << std::get<pluss::Series>(ts.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(ts.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
31 29
```

`2024-02-29` is valid — 2024 is a leap year, so February has 29 days that
year; see [`leap()`](leap.md).

## Notes

- Reads local calendar fields relative to whatever timezone the field's
  `Timeline` currently carries (`"UTC"` unless changed via
  [`tz()`](tz.md)/[`localize()`](localize.md)).
- Requires every bound field to be `Timeline`-kind — a non-`Timeline` field
  throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`RegistryTimelineAccessor::year`](year.md) / [`month()`](month.md): the
  other calendar-date components.
- [`RegistryTimelineAccessor::yearday`](yearday.md): the day-of-*year* number
  instead of day-of-month.
- [`RegistryTimelineAccessor::month_end`](month_end.md): whether this day is
  the last day of its month.
- [`Registry::tl`](../registry/tl.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="date.html">
    <span class="label">Previous</span>
    <span class="title">date</span>
  </a>
  <a class="next-link" href="epoch.html">
    <span class="label">Next</span>
    <span class="title">epoch</span>
  </a>
</div>
