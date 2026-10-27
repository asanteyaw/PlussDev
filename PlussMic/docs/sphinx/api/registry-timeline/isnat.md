<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 59edba876299 -->

# RegistryTimelineAccessor::isnat

[Back to RegistryTimelineAccessor API Reference](../../registry_timeline_accessor_api_reference)

## Signature

```cpp
Registry RegistryTimelineAccessor::isnat() const
```

## What It Does

Flags whether each bound field's timestamp is `NaT` (missing/"Not a Time"),
replacing each `Timeline` field with a `Bool` `Series` field.

```{important} RegistryTimelineAccessor requires every bound field to be Timeline-kind
Every method on this accessor — not just `isnat()` — routes through
`Slot::tl()` for each bound field. If any bound field isn't `Timeline`-kind
(`Series`, `Note`, `Mixed`, `Null`, or `Empty`), the call throws
`"Slot::tl: slot kind must be Timeline"`. Since [`Registry::tl()`](../registry/tl.md)
binds *every* field of the registry by default, project down to just the
timestamp fields first with [`fields(keys)`](../registry/fields.md) before
calling `.tl()`.
```

`isnat()`'s own result is never itself `NaT` — it always answers `true` or
`false`, even for the row it's flagging.

## Parameters

`None`

## Returns

`Registry`
    A new registry with every bound field replaced by a `Bool` `Series` field:
    `true` where the value is `NaT`, `false` otherwise.

## Examples

Example 1. Flag a `NaT` timestamp alongside a valid one. An empty string
passed to [`pluss::make_timeline`](../registry/tl.md) produces `NaT` rather
than throwing.

```cpp
auto rg = pluss::scribble::Registry({
    {"ts", pluss::scribble::Slot({pluss::make_timeline(""), pluss::make_timeline("2024-01-01T00:00:00Z")})},
});

auto out = rg.fields({"ts"}).tl().isnat();
auto ts = out.fields("ts");
std::cout << std::boolalpha
          << std::get<pluss::Series>(ts.at(0)).host<bool>()[0] << " "
          << std::get<pluss::Series>(ts.at(1)).host<bool>()[0];
```

### Output

```text
true false
```

Example 2. A non-`Timeline` field bound to `.tl()` throws — see the important
callout above.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1)})},
});

try {
  auto out = rg.fields({"id"}).tl().isnat();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Slot::tl: slot kind must be Timeline
```

## Notes

- See the important callout above — this precondition applies to every
  method on `RegistryTimelineAccessor`, not only `isnat()`.
- Unlike almost every other method on this accessor, `isnat()`'s output is
  never itself masked — it directly answers the missingness question.

## See Also

- [`RegistryTimelineAccessor::valid`](valid.md): the logical inverse.
- [`Registry::tl`](../registry/tl.md): the entry point this accessor is
  reached from.
- [`Registry::fields`](../registry/fields.md): project down to timestamp-only
  fields before calling `.tl()`, per the important callout above.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hour.html">
    <span class="label">Previous</span>
    <span class="title">hour</span>
  </a>
  <a class="next-link" href="iso.html">
    <span class="label">Next</span>
    <span class="title">iso</span>
  </a>
</div>
