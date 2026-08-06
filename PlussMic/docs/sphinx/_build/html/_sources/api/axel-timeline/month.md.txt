<!-- pluss-hand-authored -->

# AxelTimelineAccessor::month

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
Axel AxelTimelineAccessor::month() const
```

## What It Does

Extracts the calendar month (`1`-`12`) of each bound field's timestamp,
in every registry frame, replacing each `Timeline` field with an
`Int64` `Series` field. Applies
[`RegistryTimelineAccessor::month`](../registry-timeline/month.md) to
every frame via [`Axel::map`](../axel/map.md).

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field, in every frame, replaced by an
    `Int64` `Series` field of month numbers; every unbound field
    unchanged.

## Examples

Example 1. Extract the month from a January and a February timestamp.

```cpp
auto r0 = pluss::bundle::Registry({{"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fields({"ts"}).tl().month();
auto ts = out.registry(0).fields("ts");
std::cout << std::get<pluss::Series>(ts.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(ts.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
1 2
```

## Notes

- `1` is January, `12` is December.
- Reads local calendar fields relative to whatever timezone the field's
  `Timeline` currently carries (`"UTC"` unless changed via
  [`tz()`](tz.md)/[`localize()`](localize.md)).
- Requires every bound field, in every frame, to be `Timeline`-kind — a
  non-`Timeline` field throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`AxelTimelineAccessor::month_name`](month_name.md): the same
  information spelled out as a full English name.
- [`AxelTimelineAccessor::year`](year.md) / [`day()`](day.md): the
  other calendar-date components.
- [`AxelTimelineAccessor::month_start`](month_start.md) /
  [`month_end()`](month_end.md): whether the day is the first or last
  day of this month.
- [`Axel::tl`](../axel/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="minute.html">
    <span class="label">Previous</span>
    <span class="title">minute</span>
  </a>
  <a class="next-link" href="month_end.html">
    <span class="label">Next</span>
    <span class="title">month_end</span>
  </a>
</div>
