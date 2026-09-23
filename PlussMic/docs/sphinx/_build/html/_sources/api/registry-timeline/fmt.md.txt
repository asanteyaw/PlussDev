<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: beca90d95733 -->

# RegistryTimelineAccessor::fmt

[Back to RegistryTimelineAccessor API Reference](../../registry_timeline_accessor_api_reference)

## Signature

```cpp
Registry RegistryTimelineAccessor::fmt(std::string_view pattern) const
```

## What It Does

Renders each bound field's timestamp as custom text built from `pattern`,
replacing each `Timeline` field with a `Note` (text) `Series` field.

## Parameters

**pattern** (`std::string_view`)
    A `strftime`-style format string. Recognized tokens: `%Y` (4-digit
    year), `%m`/`%d` (2-digit month/day), `%H`/`%M`/`%S` (2-digit
    hour/minute/second), `%f` (9-digit zero-padded nanoseconds), `%z` (UTC
    offset as `+HH:MM`/`-HH:MM`, never a bare `Z`), `%a`/`%A` (abbreviated/full
    weekday name), `%b`/`%B` (abbreviated/full month name), and `%%` (a
    literal `%`). Cannot be empty.

## Returns

`Registry`
    A new registry with every bound field replaced by a `Note` field holding
    each row's text rendered per `pattern`; every unbound field unchanged.

## Examples

Example 1. Format two timestamps with a custom day/month-name/year pattern.

```cpp
auto rg = pluss::scribble::Registry({
    {"ts", pluss::scribble::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})},
});

auto out = rg.fields({"ts"}).tl().fmt("%d %b %Y");
auto ts = out.fields("ts");
std::cout << std::get<pluss::Note>(ts.at(0)).at(0) << " | "
          << std::get<pluss::Note>(ts.at(1)).at(0);
```

### Output

```text
31 Jan 2024 | 29 Feb 2024
```

## Notes

- `pattern` cannot be empty — an empty pattern throws.
- `%f` always emits all 9 nanosecond digits, zero-padded; `%z` always renders
  as a signed `HH:MM` offset, never a bare `"Z"`.
- Changes each bound field's kind from `Timeline` to `Note`.
- Requires every bound field to be `Timeline`-kind — a non-`Timeline` field
  throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`RegistryTimelineAccessor::iso`](iso.md): a fixed ISO-8601-with-offset
  pattern, equivalent to `fmt("%Y-%m-%dT%H:%M:%S.%f%z")`.
- [`RegistryTimelineAccessor::weekday_name`](weekday_name.md) /
  [`month_name()`](month_name.md): dedicated shorthands for the full-name
  tokens.
- [`Registry::tl`](../registry/tl.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="floor.html">
    <span class="label">Previous</span>
    <span class="title">floor</span>
  </a>
  <a class="next-link" href="hour.html">
    <span class="label">Next</span>
    <span class="title">hour</span>
  </a>
</div>
