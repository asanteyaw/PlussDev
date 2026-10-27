<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: cb74d4fc3d15 -->

# AxelTimelineAccessor::weekend

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
Axel AxelTimelineAccessor::weekend() const
```

## What It Does

Flags whether each bound field's timestamp falls on a Saturday or
Sunday, in every registry frame, replacing each `Timeline` field with a
`Bool` `Series` field. Applies
[`RegistryTimelineAccessor::weekend`](../registry-timeline/weekend.md)
to every frame via [`Axel::map`](../axel/map.md).

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field, in every frame, replaced by a
    `Bool` `Series` field: `true` where the row falls on a weekend,
    `false` otherwise.

## Examples

Example 1. Flag a Saturday and a Wednesday.

```cpp
auto r0 = pluss::scribble::Registry({{"ts", pluss::scribble::Slot({pluss::make_timeline("2024-07-06T14:00:00Z"), pluss::make_timeline("2023-03-01T00:00:00Z")})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"ts"}).tl().weekend();
auto ts = out.registry(0).fields("ts");
std::cout << std::boolalpha
          << std::get<pluss::Series>(ts.at(0)).host<bool>()[0] << " "
          << std::get<pluss::Series>(ts.at(1)).host<bool>()[0];
```

### Output

```text
true false
```

`"2024-07-06"` is a Saturday; `"2023-03-01"` is a Wednesday.

## Notes

- Saturday and Sunday only — there's no configurable weekend definition
  on this method.
- Reads local calendar fields relative to whatever timezone the field's
  `Timeline` currently carries (`"UTC"` unless changed via
  [`tz()`](tz.md)/[`localize()`](localize.md)).
- Requires every bound field, in every frame, to be `Timeline`-kind — a
  non-`Timeline` field throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`AxelTimelineAccessor::weekday`](weekday.md): the underlying weekday
  number this method is derived from.
- [`AxelTimelineAccessor::weekday_name`](weekday_name.md): the weekday
  spelled out as a full English name.
- [`Axel::tl`](../axel/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekday_name.html">
    <span class="label">Previous</span>
    <span class="title">weekday_name</span>
  </a>
  <a class="next-link" href="year.html">
    <span class="label">Next</span>
    <span class="title">year</span>
  </a>
</div>
