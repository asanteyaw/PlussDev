<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 75f23c0da6bf -->

# RegistryTimelineAccessor::ceil

[Back to RegistryTimelineAccessor API Reference](../../registry_timeline_accessor_api_reference)

## Signature

```cpp
Registry RegistryTimelineAccessor::ceil(std::string_view unit) const
```

## What It Does

Rounds each bound field's timestamp *up* to the nearest `unit` boundary,
replacing each `Timeline` field's values in place (the field stays
`Timeline`-kind).

## Parameters

**unit** (`std::string_view`)
    One of `"S"`/`"s"`/`"sec"`/`"second"`, `"min"`/`"m"`/`"minute"`,
    `"H"`/`"h"`/`"hour"`, or `"D"`/`"d"`/`"day"`. Cannot be empty. An
    unsupported unit throws.

## Returns

`Registry`
    A new registry with every bound field's timestamps ceiled to `unit`;
    every unbound field unchanged.

## Examples

Example 1. Ceil two timestamps to the nearest hour.

```cpp
auto rg = pluss::bundle::Registry({
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})},
});

auto out = rg.fields({"ts"}).tl().ceil("H");
auto fmted = out.fields("ts").tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << std::get<pluss::Note>(fmted.at(0)).at(0) << " | "
          << std::get<pluss::Note>(fmted.at(1)).at(0);
```

### Output

```text
2024-01-31 10:00:00 | 2024-03-01 00:00:00
```

Both rows advance to the start of their *next* hour — `09:15:30` to
`10:00:00`, and `23:45:00` rolling over the day boundary into
`2024-03-01 00:00:00`.

## Notes

- A row already exactly on a `unit` boundary is left unchanged, not
  advanced to the next boundary.
- `unit` cannot be empty, and only accepts the fixed clock units listed
  above — an unrecognized unit throws
  `"Timeline: unsupported unit (expected S/min/H/D)"`.
- Requires every bound field to be `Timeline`-kind — a non-`Timeline` field
  throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`RegistryTimelineAccessor::floor`](floor.md) / [`round()`](round.md):
  round down, or to the nearest boundary, instead of always up.
- [`Registry::tl`](../registry/tl.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="date.html">
    <span class="label">Next</span>
    <span class="title">date</span>
  </a>
</div>
