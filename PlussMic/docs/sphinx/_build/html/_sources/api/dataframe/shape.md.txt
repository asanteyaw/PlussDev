# DataFrame::shape

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::shape() -> std::pair<std::int64_t, std::int64_t>
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

`None`

## Returns

`std::pair<std::int64_t, std::int64_t>`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.shape();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set_features_.html">
    <span class="label">Previous</span>
    <span class="title">set_features_</span>
  </a>
  <a class="next-link" href="shift.html">
    <span class="label">Next</span>
    <span class="title">shift</span>
  </a>
</div>
