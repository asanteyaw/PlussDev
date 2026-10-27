<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b10a96272855 -->

# AxelTimelineAccessor::weekday

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
Axel AxelTimelineAccessor::weekday() const
```

## What It Does

Extracts the weekday number (`0` = Monday through `6` = Sunday) of each
bound field's timestamp, in every registry frame, replacing each
`Timeline` field with an `Int64` `Series` field. Applies
[`RegistryTimelineAccessor::weekday`](../registry-timeline/weekday.md)
to every frame via [`Axel::map`](../axel/map.md).

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field, in every frame, replaced by an
    `Int64` `Series` field of weekday numbers; every unbound field
    unchanged.

## Examples

Example 1. Extract the weekday number from a Wednesday and a Thursday.

```cpp
auto r0 = pluss::scribble::Registry({{"ts", pluss::scribble::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"ts"}).tl().weekday();
auto ts = out.registry(0).fields("ts");
std::cout << std::get<pluss::Series>(ts.at(0)).host<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(ts.at(1)).host<std::int64_t>()[0];
```

### Output

```text
2 3
```

`"2024-01-31"` is a Wednesday (`2`), `"2024-02-29"` is a Thursday (`3`)
— see [`weekday_name()`](weekday_name.md) for the same information
spelled out.

## Notes

- `0` is Monday, `6` is Sunday (ISO weekday numbering).
- Reads local calendar fields relative to whatever timezone the field's
  `Timeline` currently carries (`"UTC"` unless changed via
  [`tz()`](tz.md)/[`localize()`](localize.md)).
- Requires every bound field, in every frame, to be `Timeline`-kind — a
  non-`Timeline` field throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`AxelTimelineAccessor::weekday_name`](weekday_name.md): the same
  information as a full English name instead of a number.
- [`AxelTimelineAccessor::weekend`](weekend.md): whether the weekday
  falls on a weekend.
- [`AxelTimelineAccessor::week`](week.md): the ISO week-of-year this
  weekday belongs to.
- [`Axel::tl`](../axel/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="week.html">
    <span class="label">Previous</span>
    <span class="title">week</span>
  </a>
  <a class="next-link" href="weekday_name.html">
    <span class="label">Next</span>
    <span class="title">weekday_name</span>
  </a>
</div>
