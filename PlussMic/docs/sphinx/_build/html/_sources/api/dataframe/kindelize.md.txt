# DataFrame::kindelize

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::kindelize() -> pluss::kindle::Catalog

DataFrame::kindelize(const pluss::kindle::KindelizePolicy& policy) -> pluss::kindle::Catalog
```

## What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

## Parameters

**policy**
    Policy object that changes the structural behavior of this overload.

## Returns

`pluss::kindle::Catalog`
    Value produced by this call.

## Examples

Example 1. Convert the frame into a Kindle catalog using the default policy.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.kindelize();
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Convert the frame into a Kindle catalog using an explicit kindelization policy.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

pluss::kindle::KindelizePolicy policy;
auto out = df.kindelize(policy);
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="join.html">
    <span class="label">Previous</span>
    <span class="title">join</span>
  </a>
  <a class="next-link" href="kurtosis.html">
    <span class="label">Next</span>
    <span class="title">kurtosis</span>
  </a>
</div>
