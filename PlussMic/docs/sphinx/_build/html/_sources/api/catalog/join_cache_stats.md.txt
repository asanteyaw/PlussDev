# Catalog::join_cache_stats

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::join_cache_stats() -> std::pair<std::int64_t, std::int64_t>
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

`None`

## Returns

`std::pair<std::int64_t, std::int64_t>`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.join_cache_stats();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="join.html">
    <span class="label">Previous</span>
    <span class="title">join</span>
  </a>
  <a class="next-link" href="join_detailed.html">
    <span class="label">Next</span>
    <span class="title">join_detailed</span>
  </a>
</div>
