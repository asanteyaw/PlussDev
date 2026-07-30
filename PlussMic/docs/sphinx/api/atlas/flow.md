# Atlas::flow

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::flow(const std::function<Atlas(const Atlas&)>& fn) -> Atlas
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

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

auto out = atlas.flow();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="features.html">
    <span class="label">Previous</span>
    <span class="title">features</span>
  </a>
  <a class="next-link" href="frame.html">
    <span class="label">Next</span>
    <span class="title">frame</span>
  </a>
</div>
