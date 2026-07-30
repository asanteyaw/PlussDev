# Archive::slice_depth

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::slice_depth(std::int64_t start, std::int64_t stop) -> Archive
```

## What It Does

Returns a contiguous depth slice `[start, stop)` from the archive.

## Parameters

**start**
    Inclusive starting position of the slice.

**stop**
    Exclusive ending position of the slice.

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

auto out = archive.slice_depth(0, 1);
```

### Output

```text
train -> Catalog(metric -> Series([10.0, 20.0]))
test  -> Catalog(metric -> Series([30.0, 40.0]))
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="size.html">
    <span class="label">Previous</span>
    <span class="title">size</span>
  </a>
  <a class="next-link" href="tabelize.html">
    <span class="label">Next</span>
    <span class="title">tabelize</span>
  </a>
</div>
