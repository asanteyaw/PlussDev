# Archive::tabelize

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
Archive::tabelize(const TabelizePolicy& policy = {}) -> pluss::table::Atlas
```

## What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

## Parameters

**policy**
    Policy object that changes the structural behavior of this overload.

## Returns

`pluss::table::Atlas`
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

pluss::kindle::TabelizePolicy policy;
policy.align = pluss::kindle::AlignPolicy::Inner;
auto out = archive.tabelize(policy);
```

### Output

```text
frame0:
sales
10.0
20.0

frame1:
sales
30.0
40.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice_depth.html">
    <span class="label">Previous</span>
    <span class="title">slice_depth</span>
  </a>
  <a class="next-link" href="tail.html">
    <span class="label">Next</span>
    <span class="title">tail</span>
  </a>
</div>
