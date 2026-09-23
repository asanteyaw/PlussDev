<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ef63ae41a81b -->

# RegistryTimelineAccessor::iso

[Back to RegistryTimelineAccessor API Reference](../../registry_timeline_accessor_api_reference)

## Signature

```cpp
Registry RegistryTimelineAccessor::iso() const
```

## What It Does

Renders each bound field's timestamp as a fixed ISO-8601-with-offset text
representation, replacing each `Timeline` field with a `Note` (text) `Series`
field. Equivalent to [`fmt("%Y-%m-%dT%H:%M:%S.%f%z")`](fmt.md).

## Parameters

`None`

## Returns

`Registry`
    A new registry with every bound field replaced by a `Note` field holding
    each row as `"YYYY-MM-DDTHH:MM:SS.fffffffff+HH:MM"`; every unbound field
    unchanged.

## Examples

Example 1. Render a timestamp as ISO-8601 text.

```cpp
auto rg = pluss::scribble::Registry({
    {"ts", pluss::scribble::Slot({pluss::make_timeline("2024-01-31T09:15:30Z")})},
});

auto out = rg.fields({"ts"}).tl().iso();
std::cout << std::get<pluss::Note>(out.fields("ts").at(0)).at(0);
```

### Output

```text
2024-01-31T09:15:30.000000000+00:00
```

## Notes

- Always includes the full 9-digit nanosecond fraction and a signed `%z`
  offset — use [`fmt()`](fmt.md) with your own pattern for a more compact
  string.
- Changes each bound field's kind from `Timeline` to `Note`.
- Requires every bound field to be `Timeline`-kind — a non-`Timeline` field
  throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`RegistryTimelineAccessor::fmt`](fmt.md): the general form this method is
  a fixed pattern of.
- [`Registry::tl`](../registry/tl.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnat.html">
    <span class="label">Previous</span>
    <span class="title">isnat</span>
  </a>
  <a class="next-link" href="leap.html">
    <span class="label">Next</span>
    <span class="title">leap</span>
  </a>
</div>
