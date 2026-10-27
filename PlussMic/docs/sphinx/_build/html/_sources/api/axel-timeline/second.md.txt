<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 535b4f3931e8 -->

# AxelTimelineAccessor::second

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
Axel AxelTimelineAccessor::second() const
```

## What It Does

Extracts the second-of-minute (`0`-`59`) of each bound field's
timestamp, in every registry frame, replacing each `Timeline` field
with an `Int64` `Series` field. Applies
[`RegistryTimelineAccessor::second`](../registry-timeline/second.md) to
every frame via [`Axel::map`](../axel/map.md).

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field, in every frame, replaced by an
    `Int64` `Series` field of second numbers; every unbound field
    unchanged.

## Examples

Example 1. Extract the second component from two timestamps.

```cpp
auto r0 = pluss::scribble::Registry({{"ts", pluss::scribble::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"ts"}).tl().second();
auto ts = out.registry(0).fields("ts");
std::cout << std::get<pluss::Series>(ts.at(0)).host<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(ts.at(1)).host<std::int64_t>()[0];
```

### Output

```text
30 0
```

## Notes

- Truncates any sub-second precision the underlying timestamp carries —
  it reports whole seconds only.
- Reads local calendar fields relative to whatever timezone the field's
  `Timeline` currently carries (`"UTC"` unless changed via
  [`tz()`](tz.md)/[`localize()`](localize.md)).
- Requires every bound field, in every frame, to be `Timeline`-kind — a
  non-`Timeline` field throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`AxelTimelineAccessor::hour`](hour.md) / [`minute()`](minute.md):
  the neighboring time-of-day components.
- [`AxelTimelineAccessor::time`](time.md): the whole time-of-day
  portion at once.
- [`Axel::tl`](../axel/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="round.html">
    <span class="label">Previous</span>
    <span class="title">round</span>
  </a>
  <a class="next-link" href="shift.html">
    <span class="label">Next</span>
    <span class="title">shift</span>
  </a>
</div>
