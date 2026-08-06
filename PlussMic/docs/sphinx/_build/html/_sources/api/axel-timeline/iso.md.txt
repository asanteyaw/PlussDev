<!-- pluss-hand-authored -->

# AxelTimelineAccessor::iso

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
Axel AxelTimelineAccessor::iso() const
```

## What It Does

Renders each bound field's timestamp as a fixed ISO-8601-with-offset
text representation, in every registry frame, replacing each
`Timeline` field with a `Note` (text) `Series` field. Equivalent to
[`fmt("%Y-%m-%dT%H:%M:%S.%f%z")`](fmt.md). Applies
[`RegistryTimelineAccessor::iso`](../registry-timeline/iso.md) to every
frame via [`Axel::map`](../axel/map.md).

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field, in every frame, replaced by a
    `Note` field holding each row as
    `"YYYY-MM-DDTHH:MM:SS.fffffffff+HH:MM"`; every unbound field
    unchanged.

## Examples

Example 1. Render a timestamp as ISO-8601 text.

```cpp
auto r0 = pluss::bundle::Registry({{"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z")})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fields({"ts"}).tl().iso();
std::cout << std::get<pluss::Note>(out.registry(0).fields("ts").at(0)).at(0);
```

### Output

```text
2024-01-31T09:15:30.000000000+00:00
```

## Notes

- Always includes the full 9-digit nanosecond fraction and a signed
  `%z` offset — use [`fmt()`](fmt.md) with your own pattern for a more
  compact string.
- Changes each bound field's kind from `Timeline` to `Note`.
- Requires every bound field, in every frame, to be `Timeline`-kind — a
  non-`Timeline` field throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`AxelTimelineAccessor::fmt`](fmt.md): the general form this method
  is a fixed pattern of.
- [`Axel::tl`](../axel/tl.md): the entry point this accessor is reached
  from.

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
