<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8efeeca41281 -->

# RegistryTimelineAccessor::round

[Back to RegistryTimelineAccessor API Reference](../../registry_timeline_accessor_api_reference)

## Signature

```cpp
Registry RegistryTimelineAccessor::round(std::string_view unit) const
```

## What It Does

Rounds each bound field's timestamp to the *nearest* `unit` boundary,
replacing each `Timeline` field's values in place (the field stays
`Timeline`-kind).

## Parameters

**unit** (`std::string_view`)
    One of `"S"`/`"s"`/`"sec"`/`"second"`, `"min"`/`"m"`/`"minute"`,
    `"H"`/`"h"`/`"hour"`, or `"D"`/`"d"`/`"day"`. Cannot be empty. An
    unsupported unit throws.

## Returns

`Registry`
    A new registry with every bound field's timestamps rounded to the
    nearest `unit`; every unbound field unchanged.

## Examples

Example 1. Round two timestamps to the nearest hour.

```cpp
auto rg = pluss::scribble::Registry({
    {"ts", pluss::scribble::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})},
});

auto out = rg.fields({"ts"}).tl().round("H");
auto fmted = out.fields("ts").tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << std::get<pluss::Note>(fmted.at(0)).at(0) << " | "
          << std::get<pluss::Note>(fmted.at(1)).at(0);
```

### Output

```text
2024-01-31 09:00:00 | 2024-03-01 00:00:00
```

`09:15:30` is closer to `09:00:00` than `10:00:00`, so it rounds down;
`23:45:00` is closer to the next day's `00:00:00` than back to `23:00:00`,
so it rounds up and rolls into `2024-03-01`.

## Notes

- A value exactly halfway between two boundaries resolves **up**, never
  down.
- `unit` cannot be empty, and only accepts the fixed clock units listed
  above — an unrecognized unit throws
  `"Timeline: unsupported unit (expected S/min/H/D)"`.
- Requires every bound field to be `Timeline`-kind — a non-`Timeline` field
  throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`RegistryTimelineAccessor::floor`](floor.md), [`ceil()`](ceil.md): always
  down, or always up, instead of nearest.
- [`Registry::tl`](../registry/tl.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="quarter.html">
    <span class="label">Previous</span>
    <span class="title">quarter</span>
  </a>
  <a class="next-link" href="second.html">
    <span class="label">Next</span>
    <span class="title">second</span>
  </a>
</div>
