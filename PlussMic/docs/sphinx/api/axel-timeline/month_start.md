<!-- pluss-hand-authored -->

# AxelTimelineAccessor::month_start

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
Axel AxelTimelineAccessor::month_start() const
```

## What It Does

Flags whether each bound field's timestamp falls on the first day of
its calendar month, in every registry frame, replacing each `Timeline`
field with a `Bool` `Series` field. Applies
[`RegistryTimelineAccessor::month_start`](../registry-timeline/month_start.md)
to every frame via [`Axel::map`](../axel/map.md).

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field, in every frame, replaced by a
    `Bool` `Series` field: `true` where the row's day-of-month is `1`,
    `false` otherwise.

## Examples

Example 1. Neither sample timestamp falls on the 1st of its month.

```cpp
auto r0 = pluss::bundle::Registry({{"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fields({"ts"}).tl().month_start();
auto ts = out.registry(0).fields("ts");
std::cout << std::boolalpha
          << std::get<pluss::Series>(ts.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(ts.at(1)).to_vec<bool>()[0];
```

### Output

```text
false false
```

Both sample dates are the *last* day of their month, not the first —
see [`month_end()`](month_end.md), which flags `true` for exactly these
rows.

## Notes

- Only checks the day-of-month component (`day() == 1`); the
  time-of-day portion doesn't affect the result.
- Reads local calendar fields relative to whatever timezone the field's
  `Timeline` currently carries (`"UTC"` unless changed via
  [`tz()`](tz.md)/[`localize()`](localize.md)).
- Requires every bound field, in every frame, to be `Timeline`-kind — a
  non-`Timeline` field throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`AxelTimelineAccessor::month_end`](month_end.md): the mirror check
  at the end of the month.
- [`AxelTimelineAccessor::day`](day.md): the underlying day-of-month
  number this method is derived from.
- [`Axel::tl`](../axel/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month_name.html">
    <span class="label">Previous</span>
    <span class="title">month_name</span>
  </a>
  <a class="next-link" href="period.html">
    <span class="label">Next</span>
    <span class="title">period</span>
  </a>
</div>
