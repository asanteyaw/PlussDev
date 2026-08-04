# Catalog::nt

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::nt() -> CatalogNoteAccessor
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

`None`

## Returns

`CatalogNoteAccessor`
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

auto out = ct.nt();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nrows.html">
    <span class="label">Previous</span>
    <span class="title">nrows</span>
  </a>
  <a class="next-link" href="query.html">
    <span class="label">Next</span>
    <span class="title">query</span>
  </a>
</div>
