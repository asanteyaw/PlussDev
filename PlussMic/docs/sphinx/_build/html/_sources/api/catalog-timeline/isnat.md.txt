<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ede9aebf4b07 -->

# CatalogTimelineAccessor::isnat

[Back to CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimelineAccessor::isnat() const
```

## What It Does

Flags whether each bound key's timestamp is `NaT` (missing/"Not a Time"), replacing
each `Timeline`-kind key with a `Bool` `Series` key.

```{important} CatalogTimelineAccessor requires every bound key to be Timeline-kind
Every method on this accessor — not just `isnat()` — routes through `Stream::tl()`
for each bound key. `tl()` requires every bound key to be `Timeline`-kind; if any
bound key is not (`Series`, `Note`, `Mixed`, `Null`, or `Empty`), the call throws
`"Thread::tl: expected Timeline payload"`.
Since [`Catalog::tl()`](../catalog/tl.md) binds *every* key of the catalog by
default, project down to just the timestamp keys first with
[`Catalog::keys(names)`](../catalog/keys.md) or
[`Catalog::keyaxis().timeline()`](../catalog/keyaxis.md) before calling `.tl()`.
```

`isnat()`'s own result is never itself `NaT` — it always answers `true` or `false`,
even for the row it's flagging.

## Parameters

`None`

## Returns

`Catalog`
    A new catalog with every bound key replaced by a `Bool` `Series` key: `true`
    where the value is `NaT`, `false` otherwise.

## Examples

Example 1. Flag a `NaT` timestamp alongside a valid one. An empty string passed to
[`pluss::make_timeline`](../catalog/tl.md) produces `NaT` rather than throwing.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"", "2024-01-01T00:00:00Z"})),
});
auto out = ct.tl().isnat();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [ts]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
ts                       | Series       | Series[bool]     | 2       | [true, false]                                    
========================================================================================================================
```

Example 2. A non-`Timeline` key throws — see the important callout
above.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
});
try {
  auto out = ct.tl().isnat();
} catch (const pluss::Error& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Thread::tl: expected Timeline payload
```

## Notes

- See the important callout above — this precondition applies to every method on
  `CatalogTimelineAccessor`, not only `isnat()`.
- Unlike almost every other method on this accessor, `isnat()`'s output is never
  itself masked — it directly answers the missingness question.

## See Also

- [`CatalogTimelineAccessor::valid`](valid.md): the logical inverse.
- [`Catalog::tl`](../catalog/tl.md): the entry point this accessor is reached from.
- [`Catalog::keys`](../catalog/keys.md): project down to timestamp-only keys before
  calling `.tl()`, per the important callout above.
- [`RegistryTimelineAccessor::isnat`](../registry-timeline/isnat.md): the
  scribble-model equivalent.

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
