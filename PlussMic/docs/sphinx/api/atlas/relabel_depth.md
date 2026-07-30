# Atlas::relabel_depth

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::relabel_depth(const Thread& labels) -> Atlas
```

## What It Does

Replaces the depth-label axis with the supplied labels while keeping frame contents unchanged.

## Parameters

**labels**
    Optional integer labels assigned to the output buckets. Leave empty to let PlussMic use `0, 1, 2, ...`.

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

auto labels = pluss::table::Thread(pluss::make_note({"baseline", "stress"}));
auto out = atlas.relabel_depth(labels);
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
  <a class="prev-link" href="plot.html">
    <span class="label">Previous</span>
    <span class="title">plot</span>
  </a>
  <a class="next-link" href="show.html">
    <span class="label">Next</span>
    <span class="title">show</span>
  </a>
</div>
