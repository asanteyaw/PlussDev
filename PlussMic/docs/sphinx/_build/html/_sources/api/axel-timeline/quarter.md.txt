<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0ed6c9093634 -->

# AxelTimelineAccessor::quarter

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
Axel AxelTimelineAccessor::quarter() const
```

## What It Does

Extracts the calendar quarter (`1`-`4`) of each bound field's
timestamp, in every registry frame, replacing each `Timeline` field
with an `Int64` `Series` field. Applies
[`RegistryTimelineAccessor::quarter`](../registry-timeline/quarter.md)
to every frame via [`Axel::map`](../axel/map.md).

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field, in every frame, replaced by an
    `Int64` `Series` field of quarter numbers; every unbound field
    unchanged.

## Examples

Example 1. Extract the quarter from a July and a March timestamp.

```cpp
auto r0 = pluss::scribble::Registry({{"ts", pluss::scribble::Slot({pluss::make_timeline("2024-07-06T14:00:00Z"), pluss::make_timeline("2023-03-01T00:00:00Z")})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"ts"}).tl().quarter();
auto ts = out.registry(0).fields("ts");
std::cout << std::get<pluss::Series>(ts.at(0)).host<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(ts.at(1)).host<std::int64_t>()[0];
```

### Output

```text
3 1
```

July falls in Q3 (months 7-9); March falls in Q1 (months 1-3).

## Notes

- `1` covers January-March, `2` April-June, `3` July-September, `4`
  October-December.
- Reads local calendar fields relative to whatever timezone the field's
  `Timeline` currently carries (`"UTC"` unless changed via
  [`tz()`](tz.md)/[`localize()`](localize.md)).
- Requires every bound field, in every frame, to be `Timeline`-kind — a
  non-`Timeline` field throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`AxelTimelineAccessor::month`](month.md): the finer-grained
  component `quarter()` is derived from.
- [`AxelTimelineAccessor::period`](period.md): group timestamps into
  arbitrary frequency-based periods, not just quarters.
- [`Axel::tl`](../axel/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="period.html">
    <span class="label">Previous</span>
    <span class="title">period</span>
  </a>
  <a class="next-link" href="round.html">
    <span class="label">Next</span>
    <span class="title">round</span>
  </a>
</div>
