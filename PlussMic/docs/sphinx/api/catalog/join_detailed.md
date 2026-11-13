# Catalog::join_detailed

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
CatalogJoinResult Catalog::join_detailed(const Catalog& other, const CatalogJoinSpec& spec, const CatalogJoinOptions& options = {})
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**other**
    Second frame, slate, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

**spec**
    Structured reorder description. It lets you move a subset of features to the front, back, or relative to one anchor feature without listing the entire schema.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`CatalogJoinResult`
    Value produced by this call.

## Examples

Example 1. Pass an explicit options object to control the operation.

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.join_detailed();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="join_cache_stats.html">
    <span class="label">Previous</span>
    <span class="title">join_cache_stats</span>
  </a>
  <a class="next-link" href="keyaxis.html">
    <span class="label">Next</span>
    <span class="title">keyaxis</span>
  </a>
</div>
