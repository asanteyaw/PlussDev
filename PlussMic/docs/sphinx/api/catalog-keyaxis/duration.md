# CatalogKeyAxisAccessor::duration

[Back to CatalogKeyAxisAccessor API Reference](../../catalog_keyaxis_accessor_api_reference)

## Signature

```cpp
Catalog CatalogKeyAxisAccessor::duration()
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

`None`

## Returns

`Catalog`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});
auto keyaxis = ct.keyaxis();

auto out = keyaxis.duration();
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="next-link" href="like.html">
    <span class="label">Next</span>
    <span class="title">like</span>
  </a>
</div>
