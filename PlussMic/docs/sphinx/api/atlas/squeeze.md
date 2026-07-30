# Atlas::squeeze

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::squeeze() -> DataFrame
```

## What It Does

Collapses a depth-1 atlas back into a single `DataFrame`.

## Parameters

`None`

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.squeeze();
```

### Output

```text
sales
10.0
20.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sort_depth.html">
    <span class="label">Previous</span>
    <span class="title">sort_depth</span>
  </a>
  <a class="next-link" href="tabelize.html">
    <span class="label">Next</span>
    <span class="title">tabelize</span>
  </a>
</div>
