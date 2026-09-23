<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6c79cde9c4d8 -->

# AxelTimelineAccessor::ceil

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
Axel AxelTimelineAccessor::ceil(std::string_view unit) const
```

## What It Does

Rounds each bound field's timestamp *up* to the nearest `unit`
boundary, in every registry frame, replacing each `Timeline` field's
values in place. Applies
[`RegistryTimelineAccessor::ceil`](../registry-timeline/ceil.md) to
every frame via [`Axel::map`](../axel/map.md).

## Parameters

**unit** (`std::string_view`)
    One of `"S"`/`"s"`/`"sec"`/`"second"`, `"min"`/`"m"`/`"minute"`,
    `"H"`/`"h"`/`"hour"`, or `"D"`/`"d"`/`"day"`. Cannot be empty. An
    unsupported unit throws.

## Returns

`Axel`
    A new axel with every bound field's timestamps, in every frame,
    ceiled to `unit`; every unbound field unchanged.

## Examples

Example 1. Ceil two timestamps to the nearest hour.

```cpp
auto r0 = pluss::scribble::Registry({{"ts", pluss::scribble::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"ts"}).tl().ceil("H");
auto fmted = out.fields({"ts"}).tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << std::get<pluss::Note>(fmted.registry(0).fields("ts").at(0)).at(0) << " | "
          << std::get<pluss::Note>(fmted.registry(0).fields("ts").at(1)).at(0);
```

### Output

```text
2024-01-31 10:00:00 | 2024-03-01 00:00:00
```

Both values advance to the start of their *next* hour — `09:15:30` to
`10:00:00`, and `23:45:00` rolling over the day boundary into
`2024-03-01 00:00:00`.

## Notes

- A value already exactly on a `unit` boundary is left unchanged, not
  advanced to the next boundary.
- `unit` cannot be empty, and only accepts the fixed clock units listed
  above — an unrecognized unit throws
  `"Timeline: unsupported unit (expected S/min/H/D)"`.
- Requires every bound field, in every frame, to be `Timeline`-kind — a
  non-`Timeline` field throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`AxelTimelineAccessor::floor`](floor.md) / [`round()`](round.md):
  round down, or to the nearest boundary, instead of always up.
- [`Axel::tl`](../axel/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="date.html">
    <span class="label">Next</span>
    <span class="title">date</span>
  </a>
</div>
