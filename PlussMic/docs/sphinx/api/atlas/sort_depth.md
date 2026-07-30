# Atlas::sort_depth

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::sort_depth(bool ascending = true, bool nulls_last = true) -> Atlas
```

## What It Does

Sorts frames by their current depth labels.

## Parameters

**ascending**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

**nulls_last**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

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

auto labeled = pluss::table::Atlas({f0, f1}, pluss::table::Thread(pluss::make_note({"test", "train"})));
auto out = labeled.sort_depth(true, true);
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
  <a class="prev-link" href="slice_frames.html">
    <span class="label">Previous</span>
    <span class="title">slice_frames</span>
  </a>
  <a class="next-link" href="squeeze.html">
    <span class="label">Next</span>
    <span class="title">squeeze</span>
  </a>
</div>
