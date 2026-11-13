# DataFrameFeatureAxisAccessor::duration

[Back to DataFrameFeatureAxisAccessor API Reference](../../dataframe_feataxis_accessor_api_reference)

## Signature

```cpp
DataFrame DataFrameFeatureAxisAccessor::duration()
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

`None`

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"sales_units", pluss::table::Slate(pluss::make_series({100.0, 120.0, 90.0}))},
    {"sales_price", pluss::table::Slate(pluss::make_series({1.5, 2.0, 1.8}))},
    {"city", pluss::table::Slate(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto feataxis = df.feataxis();

auto out = feataxis.duration();
std::cout << out;
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="next-link" href="like.html">
    <span class="label">Next</span>
    <span class="title">like</span>
  </a>
</div>
