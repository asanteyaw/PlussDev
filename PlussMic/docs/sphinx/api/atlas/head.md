# Atlas::head

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::head(std::int64_t n = 5) -> Atlas
```

## What It Does

Returns the first `n` frames from the atlas depth axis.

## Parameters

**n**
    Number of rows, entries, or depth items to keep in the preview.

## Returns

`Atlas`
    Value produced by this call.

## Examples

Example 1. Inspect the first part of the current object without changing the source data.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.head(1);
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
  <a class="prev-link" href="frames.html">
    <span class="label">Previous</span>
    <span class="title">frames</span>
  </a>
  <a class="next-link" href="kindelize.html">
    <span class="label">Next</span>
    <span class="title">kindelize</span>
  </a>
</div>
