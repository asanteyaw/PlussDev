<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4f8207e6de15 -->

# CatalogTimelineAccessor::index

[Back to CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimelineAccessor::index(bool require_sorted = true) const
```

## What It Does

Converts each bound key from `Timeline`-kind to `TimeIndex`-kind. Idempotent:
calling `index()` on a key that's already `TimeIndex`-kind succeeds and leaves
its values unchanged.

```{important} CatalogTimelineAccessor requires every bound key to be Timeline- or TimeIndex-kind
Same precondition as every method on this accessor — see
[`CatalogTimelineAccessor::isnat`](isnat.md)'s own important callout for the full
explanation and the exact throw message.
```

## Parameters

**require_sorted** (`bool`, defaults to `true`)
    When `true` (the default), every value in the key must be non-decreasing
    (and non-`NaT`), or the call throws. When `false`, the conversion happens
    without checking order or `NaT`.

## Returns

`Catalog`
    A new catalog with every bound key converted to `TimeIndex`-kind.

## Examples

Example 1. Convert a sorted `Timeline` key to `TimeIndex`.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z"})),
});
auto out = ct.tl().index();
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
ts                       | TimeIndex    | TimeIndex        | 2       | [2024-01-01T00:00:00.000~, 2024-01-02T00:00:00.0~
========================================================================================================================
```

Example 2. An out-of-order key throws under the default `require_sorted = true`.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-02T00:00:00Z", "2024-01-01T00:00:00Z"})),
});
try {
  auto out = ct.tl().index();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
TimeIndex must be sorted (non-decreasing)
```

## Notes

- `require_sorted = false` skips the check entirely — use it only when the
  key's order doesn't matter for what comes next.
- Idempotent on an already-`TimeIndex`-kind key — see the important callout on
  [`isnat()`](isnat.md), which explains that `tl()` accepts both `Timeline`-
  and `TimeIndex`-kind keys uniformly.
- See the important callout above.

## See Also

- [`Catalog::keyaxis`](../catalog/keyaxis.md): select keys already at
  `TimeIndex`-kind, via `.timeindex()`.
- [`Catalog::tl`](../catalog/tl.md): the entry point this accessor is reached from.
- [`RegistryTimelineAccessor::index`](../registry-timeline/index.md): the
  bundle-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hour.html">
    <span class="label">Previous</span>
    <span class="title">hour</span>
  </a>
  <a class="next-link" href="isnat.html">
    <span class="label">Next</span>
    <span class="title">isnat</span>
  </a>
</div>
