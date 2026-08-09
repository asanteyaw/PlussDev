<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9d443c4b2940 -->

# AxelTimelineAccessor::fmt

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
Axel AxelTimelineAccessor::fmt(std::string_view pattern) const
```

## What It Does

Renders each bound field's timestamp as custom text built from
`pattern`, in every registry frame, replacing each `Timeline` field
with a `Note` (text) `Series` field. Applies
[`RegistryTimelineAccessor::fmt`](../registry-timeline/fmt.md) to every
frame via [`Axel::map`](../axel/map.md).

## Parameters

**pattern** (`std::string_view`)
    A `strftime`-style format string. Recognized tokens: `%Y` (4-digit
    year), `%m`/`%d` (2-digit month/day), `%H`/`%M`/`%S` (2-digit
    hour/minute/second), `%f` (9-digit zero-padded nanoseconds), `%z`
    (UTC offset as `+HH:MM`/`-HH:MM`, never a bare `Z`), `%a`/`%A`
    (abbreviated/full weekday name), `%b`/`%B` (abbreviated/full month
    name), and `%%` (a literal `%`). Cannot be empty.

## Returns

`Axel`
    A new axel with every bound field, in every frame, replaced by a
    `Note` field holding each row's text rendered per `pattern`; every
    unbound field unchanged.

## Examples

Example 1. Format two timestamps with a custom day/month-name/year
pattern.

```cpp
auto r0 = pluss::bundle::Registry({{"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fields({"ts"}).tl().fmt("%d %b %Y");
auto ts = out.registry(0).fields("ts");
std::cout << std::get<pluss::Note>(ts.at(0)).at(0) << " | "
          << std::get<pluss::Note>(ts.at(1)).at(0);
```

### Output

```text
31 Jan 2024 | 29 Feb 2024
```

## Notes

- `pattern` cannot be empty — an empty pattern throws.
- `%f` always emits all 9 nanosecond digits, zero-padded; `%z` always
  renders as a signed `HH:MM` offset, never a bare `"Z"`.
- Changes each bound field's kind from `Timeline` to `Note`.
- Requires every bound field, in every frame, to be `Timeline`-kind — a
  non-`Timeline` field throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`AxelTimelineAccessor::iso`](iso.md): a fixed ISO-8601-with-offset
  pattern, equivalent to `fmt("%Y-%m-%dT%H:%M:%S.%f%z")`.
- [`AxelTimelineAccessor::weekday_name`](weekday_name.md) /
  [`month_name()`](month_name.md): dedicated shorthands for the
  full-name tokens.
- [`Axel::tl`](../axel/tl.md): the entry point this accessor is reached
  from.

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
