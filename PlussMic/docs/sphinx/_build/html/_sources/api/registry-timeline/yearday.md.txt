<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 0f29404e40f7 -->

# RegistryTimelineAccessor::yearday

[Back to RegistryTimelineAccessor API Reference](../../registry_timeline_accessor_api_reference)

## Signature

```cpp
Registry RegistryTimelineAccessor::yearday() const
```

## What It Does

Extracts the day-of-year (`1`-`366`) of each bound field's timestamp,
replacing each `Timeline` field with an `Int64` `Series` field.

## Parameters

`None`

## Returns

`Registry`
    A new registry with every bound field replaced by an `Int64` `Series`
    field of day-of-year numbers; every unbound field unchanged.

## Examples

Example 1. Extract the day-of-year from a late-January and a late-February
timestamp.

```cpp
auto rg = pluss::scribble::Registry({
    {"ts", pluss::scribble::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})},
});

auto out = rg.fields({"ts"}).tl().yearday();
auto ts = out.fields("ts");
std::cout << std::get<pluss::Series>(ts.at(0)).host<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(ts.at(1)).host<std::int64_t>()[0];
```

### Output

```text
31 60
```

`"2024-01-31"` is the 31st day of the year; `"2024-02-29"` is the 60th — the
extra leap day pushes every date from March onward one day later than it
would fall in a non-leap year.

## Notes

- `1` is January 1st; the maximum is `366` in a leap year, `365` otherwise —
  see [`leap()`](leap.md).
- Reads local calendar fields relative to whatever timezone the field's
  `Timeline` currently carries (`"UTC"` unless changed via
  [`tz()`](tz.md)/[`localize()`](localize.md)).
- Requires every bound field to be `Timeline`-kind — a non-`Timeline` field
  throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`RegistryTimelineAccessor::day`](day.md): the day-of-*month* number
  instead of day-of-year.
- [`RegistryTimelineAccessor::week`](week.md): the ISO week-of-year, a
  coarser-grained position within the year.
- [`RegistryTimelineAccessor::leap`](leap.md): whether the row's year has the
  extra day that shifts this number.
- [`Registry::tl`](../registry/tl.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="year.html">
    <span class="label">Previous</span>
    <span class="title">year</span>
  </a>
</div>
