<!-- pluss-hand-authored -->

# AxelTimelineAccessor::weekday_name

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
Axel AxelTimelineAccessor::weekday_name() const
```

## What It Does

Renders each bound field's timestamp as its full English weekday name,
in every registry frame, replacing each `Timeline` field with a `Note`
(text) `Series` field. Applies
[`RegistryTimelineAccessor::weekday_name`](../registry-timeline/weekday_name.md)
to every frame via [`Axel::map`](../axel/map.md).

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field, in every frame, replaced by a
    `Note` field holding each row's full weekday name
    (`"Monday"`-`"Sunday"`); every unbound field unchanged.

## Examples

Example 1. Render the weekday name for a Wednesday and a Thursday.

```cpp
auto r0 = pluss::bundle::Registry({{"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fields({"ts"}).tl().weekday_name();
auto ts = out.registry(0).fields("ts");
std::cout << std::get<pluss::Note>(ts.at(0)).at(0) << " | "
          << std::get<pluss::Note>(ts.at(1)).at(0);
```

### Output

```text
Wednesday | Thursday
```

## Notes

- Always the full English name (`"Monday"`, not `"Mon"`); use
  [`fmt("%a")`](fmt.md) for the abbreviated three-letter form.
- Changes each bound field's kind from `Timeline` to `Note`.
- Requires every bound field, in every frame, to be `Timeline`-kind — a
  non-`Timeline` field throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`AxelTimelineAccessor::weekday`](weekday.md): the same information
  as a `0`-`6` number instead of a name.
- [`AxelTimelineAccessor::month_name`](month_name.md): the equivalent
  for months.
- [`Axel::tl`](../axel/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weekday.html">
    <span class="label">Previous</span>
    <span class="title">weekday</span>
  </a>
  <a class="next-link" href="weekend.html">
    <span class="label">Next</span>
    <span class="title">weekend</span>
  </a>
</div>
