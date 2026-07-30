# Archive::map_catalogs

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::map_catalogs(const std::function<Catalog(const Catalog&)>& fn) -> Archive
```

## What It Does

Applies the same function to each catalog and returns the transformed archive.

## Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

## Returns

`Archive`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
using namespace pluss::kindle::literals;

auto c0 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});
auto c1 = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({30.0, 40.0})),
});
auto archive = pluss::kindle::Archive({c0, c1}, pluss::kindle::Stream(pluss::make_note({"train", "test"})));

auto out = archive.map_catalogs([](const pluss::kindle::Catalog& catalog) { return catalog.head(1); });
```

### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="loc.html">
    <span class="label">Previous</span>
    <span class="title">loc</span>
  </a>
  <a class="next-link" href="maxkeys.html">
    <span class="label">Next</span>
    <span class="title">maxkeys</span>
  </a>
</div>
