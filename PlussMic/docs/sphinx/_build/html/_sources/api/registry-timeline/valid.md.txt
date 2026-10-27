<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 55ee00c8abf4 -->

# RegistryTimelineAccessor::valid

[Back to RegistryTimelineAccessor API Reference](../../registry_timeline_accessor_api_reference)

## Signature

```cpp
Registry RegistryTimelineAccessor::valid() const
```

## What It Does

Flags whether each bound field's timestamp is a real, present timestamp (not
`NaT`), replacing each `Timeline` field with a `Bool` `Series` field. Exactly
the logical inverse of [`isnat()`](isnat.md).

## Parameters

`None`

## Returns

`Registry`
    A new registry with every bound field replaced by a `Bool` `Series` field:
    `true` where the value is a real timestamp, `false` where it's `NaT`.

## Examples

Example 1. Flag a valid timestamp alongside a `NaT` one.

```cpp
auto rg = pluss::scribble::Registry({
    {"ts", pluss::scribble::Slot({pluss::make_timeline(""), pluss::make_timeline("2024-01-01T00:00:00Z")})},
});

auto out = rg.fields({"ts"}).tl().valid();
auto ts = out.fields("ts");
std::cout << std::boolalpha
          << std::get<pluss::Series>(ts.at(0)).host<bool>()[0] << " "
          << std::get<pluss::Series>(ts.at(1)).host<bool>()[0];
```

### Output

```text
false true
```

## Notes

- Exactly `!isnat()`, provided for readability at call sites that want to
  filter *for* present timestamps rather than *for* missing ones.
- Requires every bound field to be `Timeline`-kind — a non-`Timeline` field
  throws `"Slot::tl: slot kind must be Timeline"`.
- Bound via [`Registry::tl()`](../registry/tl.md), which binds every field of
  the registry unless you project first with
  [`fields(keys)`](../registry/fields.md).

## See Also

- [`RegistryTimelineAccessor::isnat`](isnat.md): the logical inverse.
- [`Registry::tl`](../registry/tl.md): the entry point this accessor is
  reached from.

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
