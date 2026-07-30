# Atlas::align_depth

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::align_depth(const Thread& labels) -> Atlas
```

## What It Does

Reorders frames so their depth labels match the supplied label order.

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
auto out = atlas.align_depth(labels);
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

## Practical Behavior

- `align_depth(labels)` reorders frames to match the requested depth label order.
- Use it when two atlases should share the same depth layout before comparison or combination.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="aggregate_over_frames.html">
    <span class="label">Previous</span>
    <span class="title">aggregate_over_frames</span>
  </a>
  <a class="next-link" href="axis0_labels.html">
    <span class="label">Next</span>
    <span class="title">axis0_labels</span>
  </a>
</div>
