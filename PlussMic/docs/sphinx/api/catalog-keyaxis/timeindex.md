<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 11ef84e808cf -->

# CatalogKeyAxisAccessor::timeindex

[Back to CatalogKeyAxisAccessor API Reference](../../catalog_keyaxis_accessor_api_reference)

## Signature

```cpp
Catalog CatalogKeyAxisAccessor::timeindex() const
```

## What It Does

Returns a new catalog containing only the keys whose `Stream::kind()` is
`StreamKind::TimeIndex`.

```{important} TimeIndex is distinct from Timeline
A key built from [`pluss::make_timeline`](../catalog/tl.md) is `Timeline`-kind,
not `TimeIndex`-kind — `timeindex()` won't select it until it's been
converted via [`Catalog::cast().timeindex()`](../catalog-cast/timeindex.md) or
[`CatalogTimelineAccessor::index()`](../catalog-timeline/index.md). See
Example 2.
```

## Parameters

`None`

## Returns

`Catalog`
    A new catalog containing only the `TimeIndex`-kind keys.

## Examples

Example 1. A catalog with no `TimeIndex`-kind key returns an empty result —
its `Timeline`-kind key doesn't count.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0})),
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z"})),
});
auto out = ct.keyaxis().timeindex();
std::cout << "nkeys=" << out.nkeys();
```

### Output

```text
nkeys=0
```

Example 2. After casting the `Timeline`-kind key to `TimeIndex`, it's
selected.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0})),
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-01T00:00:00Z"})),
});
auto ct2 = pluss::kindle::Catalog({
    "id"_k = ct.keys("id"),
    "ts"_k = ct.keys("ts").cast().timeindex(),
});
auto out = ct2.keyaxis().timeindex();
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
ts                       | TimeIndex    | TimeIndex        | 1       | [2024-01-01T00:00:00.000~]                       
========================================================================================================================
```

## Notes

- See the important callout above.
- A `TimeIndex`-kind filter, not a cast — nothing here converts a
  `Timeline`-kind key.
- An empty result is a valid, defined, empty catalog — not a throw.

## See Also

- [`CatalogKeyAxisAccessor::timeline`](timeline.md): the sibling filter for
  `Timeline`-kind keys.
- [`Catalog::cast`](../catalog/cast.md): convert a `Timeline`-kind key to
  `TimeIndex`-kind first.
- [`Catalog::keyaxis`](../catalog/keyaxis.md): the entry point this accessor
  is reached from.
- [`RegistryFieldAxisAccessor::timeindex`](../registry-fieldaxis/timeindex.md):
  the bundle-model equivalent.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="suffix.html">
    <span class="label">Previous</span>
    <span class="title">suffix</span>
  </a>
  <a class="next-link" href="timeline.html">
    <span class="label">Next</span>
    <span class="title">timeline</span>
  </a>
</div>
