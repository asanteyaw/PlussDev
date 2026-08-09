<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: da4299c9fa38 -->

# AxelTimelineAccessor::day

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
Axel AxelTimelineAccessor::day() const
```

## What It Does

Extracts the day-of-month (`1`-`31`) of each bound field's timestamp,
in every registry frame, replacing each `Timeline` field with an
`Int64` `Series` field. Applies
[`RegistryTimelineAccessor::day`](../registry-timeline/day.md) to every
frame via [`Axel::map`](../axel/map.md).

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field, in every frame, replaced by an
    `Int64` `Series` field of day-of-month numbers; every unbound field
    unchanged.

## Examples

Example 1. Extract the day-of-month from two end-of-month timestamps.

```cpp
auto r0 = pluss::bundle::Registry({{"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fields({"ts"}).tl().day();
auto ts = out.registry(0).fields("ts");
std::cout << std::get<pluss::Series>(ts.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(ts.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
31 29
```

`2024-02-29` is valid — 2024 is a leap year, so February has 29 days
that year; see [`leap()`](leap.md).

## Notes

- Reads local calendar fields relative to whatever timezone the field's
  `Timeline` currently carries (`"UTC"` unless changed via
  [`tz()`](tz.md)/[`localize()`](localize.md)).
- Requires every bound field, in every frame, to be `Timeline`-kind — a
  non-`Timeline` field throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`AxelTimelineAccessor::year`](year.md) / [`month()`](month.md): the
  other calendar-date components.
- [`AxelTimelineAccessor::yearday`](yearday.md): the day-of-*year*
  number instead of day-of-month.
- [`AxelTimelineAccessor::month_end`](month_end.md): whether this day
  is the last day of its month.
- [`Axel::tl`](../axel/tl.md): the entry point this accessor is reached
  from.

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
