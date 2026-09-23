<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: faf736fc53a9 -->

# AxelTimelineAccessor::valid

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
Axel AxelTimelineAccessor::valid() const
```

## What It Does

Flags whether each bound field's timestamp is a real, present timestamp
(not `NaT`), in every registry frame, replacing each `Timeline` field
with a `Bool` `Series` field. Exactly the logical inverse of
[`isnat()`](isnat.md). Applies
[`RegistryTimelineAccessor::valid`](../registry-timeline/valid.md) to
every frame via [`Axel::map`](../axel/map.md).

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field, in every frame, replaced by a
    `Bool` `Series` field: `true` where the value is a real timestamp,
    `false` where it's `NaT`.

## Examples

Example 1. Flag a valid timestamp alongside a `NaT` one.

```cpp
auto r0 = pluss::scribble::Registry({{"ts", pluss::scribble::Slot({pluss::make_timeline(""), pluss::make_timeline("2024-01-01T00:00:00Z")})}});
auto axel = pluss::scribble::Axel({r0});

auto out = axel.fields({"ts"}).tl().valid();
auto ts = out.registry(0).fields("ts");
std::cout << std::boolalpha
          << std::get<pluss::Series>(ts.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(ts.at(1)).to_vec<bool>()[0];
```

### Output

```text
false true
```

## Notes

- Exactly `!isnat()`, provided for readability at call sites that want
  to filter *for* present timestamps rather than *for* missing ones.
- Requires every bound field, in every frame, to be `Timeline`-kind — a
  non-`Timeline` field throws `"Slot::tl: slot kind must be Timeline"`.
- Bound via [`Axel::tl()`](../axel/tl.md), which binds every field of
  every frame unless you project first with
  [`fields(keys)`](../axel/fields.md).

## See Also

- [`AxelTimelineAccessor::isnat`](isnat.md): the logical inverse.
- [`Axel::tl`](../axel/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="utc.html">
    <span class="label">Previous</span>
    <span class="title">utc</span>
  </a>
  <a class="next-link" href="week.html">
    <span class="label">Next</span>
    <span class="title">week</span>
  </a>
</div>
