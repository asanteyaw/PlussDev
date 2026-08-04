<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fe2b2f9df256 -->

# RegistryTimelineAccessor::year

[Back to RegistryTimelineAccessor API Reference](../../registry_timeline_accessor_api_reference)

## Signature

```cpp
Registry RegistryTimelineAccessor::year() const
```

## What It Does

Extracts the calendar year of each bound field's timestamp, replacing each
`Timeline` field with an `Int64` `Series` field.

## Parameters

`None`

## Returns

`Registry`
    A new registry with every bound field replaced by an `Int64` `Series`
    field of calendar years; every unbound field unchanged.

## Examples

Example 1. Extract the year from two timestamps in the same year.

```cpp
auto rg = pluss::bundle::Registry({
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})},
});

auto out = rg.fields({"ts"}).tl().year();
auto ts = out.fields("ts");
std::cout << std::get<pluss::Series>(ts.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(ts.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
2024 2024
```

## Notes

- Reads local calendar fields relative to whatever timezone the field's
  `Timeline` currently carries (`"UTC"` unless changed via
  [`tz()`](tz.md)/[`localize()`](localize.md)).
- Requires every bound field to be `Timeline`-kind — a non-`Timeline` field
  throws `"Slot::tl: slot kind must be Timeline"`.
- Bound via [`Registry::tl()`](../registry/tl.md), which binds every field of
  the registry unless you project first with
  [`fields(keys)`](../registry/fields.md).

## See Also

- [`RegistryTimelineAccessor::month`](month.md) / [`day()`](day.md): the
  other calendar-date components, extracted the same way.
- [`RegistryTimelineAccessor::leap`](leap.md): whether the extracted year is
  a leap year.
- [`Registry::tl`](../registry/tl.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekend.html">
    <span class="label">Previous</span>
    <span class="title">weekend</span>
  </a>
  <a class="next-link" href="yearday.html">
    <span class="label">Next</span>
    <span class="title">yearday</span>
  </a>
</div>
