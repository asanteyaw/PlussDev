<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6d3266b01092 -->

# RegistryTimelineAccessor::week

[Back to RegistryTimelineAccessor API Reference](../../registry_timeline_accessor_api_reference)

## Signature

```cpp
Registry RegistryTimelineAccessor::week() const
```

## What It Does

Extracts the ISO week-of-year (`1`-`53`) of each bound field's timestamp,
replacing each `Timeline` field with an `Int64` `Series` field.

## Parameters

`None`

## Returns

`Registry`
    A new registry with every bound field replaced by an `Int64` `Series`
    field of ISO week numbers; every unbound field unchanged.

## Examples

Example 1. Extract the ISO week from a January and a February timestamp.

```cpp
auto rg = pluss::bundle::Registry({
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})},
});

auto out = rg.fields({"ts"}).tl().week();
auto ts = out.fields("ts");
std::cout << std::get<pluss::Series>(ts.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(ts.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
5 9
```

Example 2. A second pair of timestamps, showing the ISO week wrapping across
a year boundary.

```cpp
auto rg = pluss::bundle::Registry({
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-07-06T14:00:00Z"), pluss::make_timeline("2023-03-01T00:00:00Z")})},
});

auto out = rg.fields({"ts"}).tl().week();
auto ts = out.fields("ts");
std::cout << std::get<pluss::Series>(ts.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(ts.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
27 9
```

## Notes

- Uses ISO week numbering — weeks start on Monday, and week `1` is the week
  containing the year's first Thursday, so early-January dates can land in
  the *previous* year's final week under ISO rules.
- Reads local calendar fields relative to whatever timezone the field's
  `Timeline` currently carries (`"UTC"` unless changed via
  [`tz()`](tz.md)/[`localize()`](localize.md)).
- Requires every bound field to be `Timeline`-kind — a non-`Timeline` field
  throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`RegistryTimelineAccessor::yearday`](yearday.md): the finer-grained
  day-of-year component.
- [`RegistryTimelineAccessor::weekday`](weekday.md): the day-of-week within
  that week.
- [`Registry::tl`](../registry/tl.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="valid.html">
    <span class="label">Previous</span>
    <span class="title">valid</span>
  </a>
  <a class="next-link" href="weekday.html">
    <span class="label">Next</span>
    <span class="title">weekday</span>
  </a>
</div>
