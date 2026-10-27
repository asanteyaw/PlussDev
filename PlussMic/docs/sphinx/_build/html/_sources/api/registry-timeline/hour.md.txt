<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0969197eb298 -->

# RegistryTimelineAccessor::hour

[Back to RegistryTimelineAccessor API Reference](../../registry_timeline_accessor_api_reference)

## Signature

```cpp
Registry RegistryTimelineAccessor::hour() const
```

## What It Does

Extracts the hour-of-day (`0`-`23`) of each bound field's timestamp,
replacing each `Timeline` field with an `Int64` `Series` field.

## Parameters

`None`

## Returns

`Registry`
    A new registry with every bound field replaced by an `Int64` `Series`
    field of hour numbers; every unbound field unchanged.

## Examples

Example 1. Extract the hour from a morning and a late-night timestamp.

```cpp
auto rg = pluss::scribble::Registry({
    {"ts", pluss::scribble::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})},
});

auto out = rg.fields({"ts"}).tl().hour();
auto ts = out.fields("ts");
std::cout << std::get<pluss::Series>(ts.at(0)).host<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(ts.at(1)).host<std::int64_t>()[0];
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
- Requires every bound field to be `Timeline`-kind — a non-`Timeline` field
  throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`RegistryTimelineAccessor::minute`](minute.md) / [`second()`](second.md):
  the finer-grained time-of-day components.
- [`RegistryTimelineAccessor::time`](time.md): the whole time-of-day portion
  at once.
- [`Registry::tl`](../registry/tl.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fmt.html">
    <span class="label">Previous</span>
    <span class="title">fmt</span>
  </a>
  <a class="next-link" href="isnat.html">
    <span class="label">Next</span>
    <span class="title">isnat</span>
  </a>
</div>
