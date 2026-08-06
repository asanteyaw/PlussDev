<!-- pluss-hand-authored -->

# AxelTimelineAccessor::floor

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
Axel AxelTimelineAccessor::floor(std::string_view unit) const
```

## What It Does

Rounds each bound field's timestamp *down* to the nearest `unit`
boundary, in every registry frame, replacing each `Timeline` field's
values in place (the field stays `Timeline`-kind). Applies
[`RegistryTimelineAccessor::floor`](../registry-timeline/floor.md) to
every frame via [`Axel::map`](../axel/map.md).

## Parameters

**unit** (`std::string_view`)
    One of `"S"`/`"s"`/`"sec"`/`"second"`, `"min"`/`"m"`/`"minute"`,
    `"H"`/`"h"`/`"hour"`, or `"D"`/`"d"`/`"day"`. Cannot be empty. An
    unsupported unit throws.

## Returns

`Axel`
    A new axel with every bound field's timestamps, in every frame,
    floored to `unit`; every unbound field unchanged.

## Examples

Example 1. Floor two timestamps to the nearest hour.

```cpp
auto r0 = pluss::bundle::Registry({{"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fields({"ts"}).tl().floor("H");
auto fmted = out.fields({"ts"}).tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << std::get<pluss::Note>(fmted.registry(0).fields("ts").at(0)).at(0) << " | "
          << std::get<pluss::Note>(fmted.registry(0).fields("ts").at(1)).at(0);
```

### Output

```text
2024-01-31 09:00:00 | 2024-02-29 23:00:00
```

`09:15:30` drops to `09:00:00`; `23:45:00` was already exactly on the
hour in terms of its floor target, so it advances to `23:00:00`.

Example 2. An unsupported unit throws.

```cpp
auto r0 = pluss::bundle::Registry({{"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z")})}});
auto axel = pluss::bundle::Axel({r0});

try {
  auto out = axel.fields({"ts"}).tl().floor("bogus");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Timeline: unsupported unit (expected S/min/H/D)
```

## Notes

- `unit` cannot be empty, and only accepts the fixed clock units listed
  above — an unrecognized unit throws, per Example 2.
- Requires every bound field, in every frame, to be `Timeline`-kind — a
  non-`Timeline` field throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`AxelTimelineAccessor::ceil`](ceil.md) / [`round()`](round.md):
  round up, or to the nearest boundary, instead of always down.
- [`AxelTimelineAccessor::date`](date.md): shorthand for `floor("D")`.
- [`Axel::tl`](../axel/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="epoch.html">
    <span class="label">Previous</span>
    <span class="title">epoch</span>
  </a>
  <a class="next-link" href="fmt.html">
    <span class="label">Next</span>
    <span class="title">fmt</span>
  </a>
</div>
