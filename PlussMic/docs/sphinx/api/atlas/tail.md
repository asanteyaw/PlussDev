# Atlas::tail

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::tail(std::int64_t n = 5) -> Atlas
```

## What It Does

Returns the last `n` frames from the atlas depth axis.

## Parameters

**n**
    Number of rows, entries, or depth items to keep in the preview.

## Returns

`Atlas`
    Value produced by this call.

## Examples

Example 1. Inspect the last part of the current object without changing the source data.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.tail(1);
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
  <a class="prev-link" href="tabelize.html">
    <span class="label">Previous</span>
    <span class="title">tabelize</span>
  </a>
  <a class="next-link" href="tl.html">
    <span class="label">Next</span>
    <span class="title">tl</span>
  </a>
</div>
