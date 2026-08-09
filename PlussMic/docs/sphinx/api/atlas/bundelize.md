# Atlas::bundelize

[Back to Atlas API Reference](../../atlas_api_reference)

## Signatures

```cpp
Atlas::bundelize() -> pluss::bundle::Axel

Atlas::bundelize(const pluss::kindle::KindelizePolicy& policy) -> pluss::bundle::Axel
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**policy**
    Policy object that changes the structural behavior of this overload.

## Returns

`pluss::bundle::Axel`
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

auto out = atlas.bundelize();
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto f0 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({30.0, 40.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1});

auto out = atlas.bundelize();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="axis0_labels.html">
    <span class="label">Previous</span>
    <span class="title">axis0_labels</span>
  </a>
  <a class="next-link" href="column_names.html">
    <span class="label">Next</span>
    <span class="title">column_names</span>
  </a>
</div>
