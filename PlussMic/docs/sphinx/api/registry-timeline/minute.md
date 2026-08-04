<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 975c78ba51a8 -->

# RegistryTimelineAccessor::minute

[Back to RegistryTimelineAccessor API Reference](../../registry_timeline_accessor_api_reference)

## Signature

```cpp
Registry RegistryTimelineAccessor::minute() const
```

## What It Does

Extracts the minute-of-hour (`0`-`59`) of each bound field's timestamp,
replacing each `Timeline` field with an `Int64` `Series` field.

## Parameters

`None`

## Returns

`Registry`
    A new registry with every bound field replaced by an `Int64` `Series`
    field of minute numbers; every unbound field unchanged.

## Examples

Example 1. Extract the minute component from two timestamps.

```cpp
auto rg = pluss::bundle::Registry({
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})},
});

auto out = rg.fields({"ts"}).tl().minute();
auto ts = out.fields("ts");
std::cout << std::get<pluss::Series>(ts.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(ts.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
15 45
```

## Notes

- Reads local calendar fields relative to whatever timezone the field's
  `Timeline` currently carries (`"UTC"` unless changed via
  [`tz()`](tz.md)/[`localize()`](localize.md)).
- Requires every bound field to be `Timeline`-kind — a non-`Timeline` field
  throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`RegistryTimelineAccessor::hour`](hour.md) / [`second()`](second.md): the
  neighboring time-of-day components.
- [`RegistryTimelineAccessor::time`](time.md): the whole time-of-day portion
  at once.
- [`Registry::tl`](../registry/tl.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="localize.html">
    <span class="label">Previous</span>
    <span class="title">localize</span>
  </a>
  <a class="next-link" href="month.html">
    <span class="label">Next</span>
    <span class="title">month</span>
  </a>
</div>
