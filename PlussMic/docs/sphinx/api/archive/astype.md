# Archive::astype

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::astype(pluss::ScalarType dtype) -> Archive
```

## What It Does

This changes the semantic or numeric type so later operations route through the correct implementation path.

## Parameters

**dtype**
    Target scalar dtype used by this conversion overload.

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

auto out = archive.astype();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="aggregate_over_catalogs.html">
    <span class="label">Previous</span>
    <span class="title">aggregate_over_catalogs</span>
  </a>
  <a class="next-link" href="bfill.html">
    <span class="label">Next</span>
    <span class="title">bfill</span>
  </a>
</div>
