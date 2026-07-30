# Atlas::slice_frames

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::slice_frames(std::int64_t start, std::int64_t stop) -> Atlas
```

## What It Does

Returns a contiguous depth slice `[start, stop)` from the atlas.

## Parameters

**start**
    Inclusive starting position of the slice.

**stop**
    Exclusive ending position of the slice.

## Returns

`Atlas`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.slice_frames(0, 1);
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
  <a class="prev-link" href="show.html">
    <span class="label">Previous</span>
    <span class="title">show</span>
  </a>
  <a class="next-link" href="sort_depth.html">
    <span class="label">Next</span>
    <span class="title">sort_depth</span>
  </a>
</div>
