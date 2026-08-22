<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fc9814eca14e -->

# AxelTimelineAccessor::time

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
Axel AxelTimelineAccessor::time() const
```

## What It Does

Keeps only the time-of-day portion of each bound field's timestamp —
the date is reset to the Unix epoch day (`1970-01-01`) — in every
registry frame, replacing each `Timeline` field's values in place.
Applies [`RegistryTimelineAccessor::time`](../registry-timeline/time.md)
to every frame via [`Axel::map`](../axel/map.md).

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field's date portion, in every frame,
    reset to `1970-01-01`, keeping only the time-of-day; every unbound
    field unchanged.

## Examples

Example 1. Keep only the time-of-day from two timestamps.

```cpp
auto r0 = pluss::scribble::Registry({{"ts", pluss::scribble::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"ts"}).tl().time();
auto fmted = out.fields({"ts"}).tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << std::get<pluss::Note>(fmted.registry(0).fields("ts").at(0)).at(0) << " | "
          << std::get<pluss::Note>(fmted.registry(0).fields("ts").at(1)).at(0);
```

### Output

```text
1970-01-01 09:15:30 | 1970-01-01 23:45:00
```

Every row's date is now `1970-01-01`; each original time-of-day is
preserved exactly.

## Notes

- The result is still `Timeline`-kind with a fixed date component —
  it's meant for comparing or grouping by time-of-day, not as a display
  format. Use [`fmt("%H:%M:%S")`](fmt.md) for a bare text time.
- Requires every bound field, in every frame, to be `Timeline`-kind — a
  non-`Timeline` field throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`AxelTimelineAccessor::date`](date.md): the mirror-image operation,
  keeping the date and zeroing the time-of-day.
- [`Axel::tl`](../axel/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shift.html">
    <span class="label">Previous</span>
    <span class="title">shift</span>
  </a>
  <a class="next-link" href="tz.html">
    <span class="label">Next</span>
    <span class="title">tz</span>
  </a>
</div>
