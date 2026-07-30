# Archive::aggregate_over_catalogs

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::aggregate_over_catalogs(const std::function<Catalog(const std::vector<Catalog>&)>& fn) -> Catalog
```

## What It Does

Reduces the full catalog stack to one output by applying an aggregation function across the contained catalogs.

## Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

## Returns

`Catalog`
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

auto out = archive.aggregate_over_catalogs([](const std::vector<pluss::kindle::Catalog>& catalogs) { return catalogs.front(); });
```

### Output

```text
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

## Practical Behavior

- `aggregate_over_catalogs(...)` hands the full catalog vector to your reducer and expects one output object back.
- Use it when the aggregation depends on multiple catalogs together rather than catalog-by-catalog mapping.



<div class="pluss-inline-prev-next">
  <a class="next-link" href="astype.html">
    <span class="label">Next</span>
    <span class="title">astype</span>
  </a>
</div>
