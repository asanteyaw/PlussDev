<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 41a8fd55df9e -->

# AxelTimelineAccessor::isnat

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
Axel AxelTimelineAccessor::isnat() const
```

## What It Does

Flags whether each bound field's timestamp is `NaT` (missing/"Not a
Time"), in every registry frame, replacing each `Timeline` field with a
`Bool` `Series` field. Applies
[`RegistryTimelineAccessor::isnat`](../registry-timeline/isnat.md) to
every frame via [`Axel::map`](../axel/map.md).

```{important} AxelTimelineAccessor requires every bound field, in every frame, to be Timeline-kind
Every method on this accessor — not just `isnat()` — routes through
`Registry::tl()` per frame, which itself routes through `Slot::tl()`
per field. If any bound field in any frame isn't `Timeline`-kind
(`Series`, `Note`, `TimeIndex`, `Mixed`, `Null`, or `Empty`), the call
throws `"Slot::tl: slot kind must be Timeline"`. Since
[`Axel::tl()`](../axel/tl.md) binds *every* field of *every* frame by
default, project down to just the timestamp fields first with
[`fields(keys)`](../axel/fields.md) before calling `.tl()`.
```

`isnat()`'s own result is never itself `NaT` — it always answers `true`
or `false`, even for the row it's flagging.

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field, in every frame, replaced by a
    `Bool` `Series` field: `true` where the value is `NaT`, `false`
    otherwise.

## Examples

Example 1. Flag a `NaT` timestamp alongside a valid one. An empty
string passed to `pluss::make_timeline` produces `NaT` rather than
throwing.

```cpp
auto r0 = pluss::bundle::Registry({{"ts", pluss::bundle::Slot({pluss::make_timeline(""), pluss::make_timeline("2024-01-01T00:00:00Z")})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fields({"ts"}).tl().isnat();
auto ts = out.registry(0).fields("ts");
std::cout << std::boolalpha
          << std::get<pluss::Series>(ts.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(ts.at(1)).to_vec<bool>()[0];
```

### Output

```text
true false
```

Example 2. A non-`Timeline` field bound to `.tl()` throws — see the
important callout above.

```cpp
auto r0 = pluss::bundle::Registry({{"id", pluss::bundle::Slot({pluss::make_series(1)})}});
auto axel = pluss::bundle::Axel({r0});

try {
  auto out = axel.fields({"id"}).tl().isnat();
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
  method on `AxelTimelineAccessor`, not only `isnat()`.
- Unlike almost every other method on this accessor, `isnat()`'s output
  is never itself masked — it directly answers the missingness
  question.

## See Also

- [`AxelTimelineAccessor::valid`](valid.md): the logical inverse.
- [`Axel::tl`](../axel/tl.md): the entry point this accessor is reached
  from.
- [`Axel::fields`](../axel/fields.md): project down to timestamp-only
  fields before calling `.tl()`, per the important callout above.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="index.html">
    <span class="label">Previous</span>
    <span class="title">index</span>
  </a>
  <a class="next-link" href="iso.html">
    <span class="label">Next</span>
    <span class="title">iso</span>
  </a>
</div>
