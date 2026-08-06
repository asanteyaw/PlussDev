<!-- pluss-hand-authored -->

# AxelTimelineAccessor::hour

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
Axel AxelTimelineAccessor::hour() const
```

## What It Does

Extracts the hour-of-day (`0`-`23`) of each bound field's timestamp, in
every registry frame, replacing each `Timeline` field with an `Int64`
`Series` field. Applies
[`RegistryTimelineAccessor::hour`](../registry-timeline/hour.md) to
every frame via [`Axel::map`](../axel/map.md).

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field, in every frame, replaced by an
    `Int64` `Series` field of hour numbers; every unbound field
    unchanged.

## Examples

Example 1. Extract the hour from a morning and a late-night timestamp.

```cpp
auto r0 = pluss::bundle::Registry({{"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fields({"ts"}).tl().hour();
auto ts = out.registry(0).fields("ts");
std::cout << std::get<pluss::Series>(ts.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(ts.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
9 23
```

## Notes

- 24-hour clock: `0` is midnight, `23` is 11 PM.
- Reads local calendar fields relative to whatever timezone the field's
  `Timeline` currently carries (`"UTC"` unless changed via
  [`tz()`](tz.md)/[`localize()`](localize.md)).
- Requires every bound field, in every frame, to be `Timeline`-kind — a
  non-`Timeline` field throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`AxelTimelineAccessor::minute`](minute.md) / [`second()`](second.md):
  the finer-grained time-of-day components.
- [`AxelTimelineAccessor::time`](time.md): the whole time-of-day
  portion at once.
- [`Axel::tl`](../axel/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fmt.html">
    <span class="label">Previous</span>
    <span class="title">fmt</span>
  </a>
  <a class="next-link" href="index.html">
    <span class="label">Next</span>
    <span class="title">index</span>
  </a>
</div>
