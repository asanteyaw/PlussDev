<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ecdebcd9aca0 -->

# RegistryTimelineAccessor::month_name

[Back to RegistryTimelineAccessor API Reference](../../registry_timeline_accessor_api_reference)

## Signature

```cpp
Registry RegistryTimelineAccessor::month_name() const
```

## What It Does

Renders each bound field's timestamp as its full English month name,
replacing each `Timeline` field with a `Note` (text) `Series` field.

## Parameters

`None`

## Returns

`Registry`
    A new registry with every bound field replaced by a `Note` field holding
    each row's full month name (`"January"`-`"December"`); every unbound
    field unchanged.

## Examples

Example 1. Render the month name for a January and a February timestamp.

```cpp
auto rg = pluss::bundle::Registry({
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})},
});

auto out = rg.fields({"ts"}).tl().month_name();
auto ts = out.fields("ts");
std::cout << std::get<pluss::Note>(ts.at(0)).at(0) << " | "
          << std::get<pluss::Note>(ts.at(1)).at(0);
```

### Output

```text
January | February
```

## Notes

- Always the full English name (`"January"`, not `"Jan"`); use
  [`fmt("%b")`](fmt.md) for the abbreviated three-letter form.
- Changes each bound field's kind from `Timeline` to `Note`.
- Requires every bound field to be `Timeline`-kind — a non-`Timeline` field
  throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`RegistryTimelineAccessor::month`](month.md): the same information as a
  `1`-`12` number instead of a name.
- [`RegistryTimelineAccessor::weekday_name`](weekday_name.md): the
  equivalent for weekdays.
- [`Registry::tl`](../registry/tl.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="month_end.html">
    <span class="label">Previous</span>
    <span class="title">month_end</span>
  </a>
  <a class="next-link" href="month_start.html">
    <span class="label">Next</span>
    <span class="title">month_start</span>
  </a>
</div>
