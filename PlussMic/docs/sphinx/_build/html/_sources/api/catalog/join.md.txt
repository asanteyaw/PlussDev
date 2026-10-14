# Catalog::join

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::join(const Catalog& other, const CatalogJoinSpec& spec, const CatalogJoinOptions& options = {}) -> Catalog
```

## What It Does

Use this when you need to merge compatible objects or extend the current one with another input.

## Parameters

**other**
    Second frame, slate, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

**spec**
    Structured reorder description. It lets you move a subset of features to the front, back, or relative to one anchor feature without listing the entire schema.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`Catalog`
    Value produced by this call.

## Examples

Example 1. Pass an explicit options object to control the operation.

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.join();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnull.html">
    <span class="label">Previous</span>
    <span class="title">isnull</span>
  </a>
  <a class="next-link" href="join_cache_stats.html">
    <span class="label">Next</span>
    <span class="title">join_cache_stats</span>
  </a>
</div>
