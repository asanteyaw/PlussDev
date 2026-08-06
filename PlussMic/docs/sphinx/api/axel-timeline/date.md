<!-- pluss-hand-authored -->

# AxelTimelineAccessor::date

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
Axel AxelTimelineAccessor::date() const
```

## What It Does

Strips the time-of-day from each bound field's timestamp (dropped to
midnight), in every registry frame, replacing each `Timeline` field's
values in place. Equivalent to [`floor("D")`](floor.md). Applies
[`RegistryTimelineAccessor::date`](../registry-timeline/date.md) to
every frame via [`Axel::map`](../axel/map.md).

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field's time-of-day portion, in every
    frame, zeroed out; every unbound field unchanged.

## Examples

Example 1. Strip the time-of-day from two timestamps.

```cpp
auto r0 = pluss::bundle::Registry({{"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fields({"ts"}).tl().date();
auto fmted = out.fields({"ts"}).tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << std::get<pluss::Note>(fmted.registry(0).fields("ts").at(0)).at(0) << " | "
          << std::get<pluss::Note>(fmted.registry(0).fields("ts").at(1)).at(0);
```

### Output

```text
2024-01-31 00:00:00 | 2024-02-29 00:00:00
```

## Notes

- Requires every bound field, in every frame, to be `Timeline`-kind — a
  non-`Timeline` field throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`AxelTimelineAccessor::time`](time.md): the mirror-image operation,
  keeping the time-of-day and zeroing the date.
- [`AxelTimelineAccessor::floor`](floor.md): the general form `date()`
  is a fixed `"D"` unit of.
- [`Axel::tl`](../axel/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ceil.html">
    <span class="label">Previous</span>
    <span class="title">ceil</span>
  </a>
  <a class="next-link" href="day.html">
    <span class="label">Next</span>
    <span class="title">day</span>
  </a>
</div>
