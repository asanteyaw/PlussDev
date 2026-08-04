# Catalog::tl

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::tl() -> CatalogTimelineAccessor
```

## What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

## Parameters

`None`

## Returns

`CatalogTimelineAccessor`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.tl();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tfidf.html">
    <span class="label">Previous</span>
    <span class="title">tfidf</span>
  </a>
  <a class="next-link" href="to_json.html">
    <span class="label">Next</span>
    <span class="title">to_json</span>
  </a>
</div>
