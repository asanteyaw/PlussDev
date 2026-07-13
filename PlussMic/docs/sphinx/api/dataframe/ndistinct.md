# DataFrame::ndistinct

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::ndistinct(pluss::RangeView<std::string_view> features = {}, bool dropna = true) -> Thread
```

## What It Does

Counts distinct observed values or distinct value-combinations across the selected features.

## Parameters

**features**
    Feature name(s) that select the target DataFrame columns for the operation.

**dropna**
    When `true`, null grouping keys or null values are excluded where the method supports that policy. When `false`, null participates instead of being removed.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.ndistinct();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ncases.html">
    <span class="label">Previous</span>
    <span class="title">ncases</span>
  </a>
  <a class="next-link" href="ne.html">
    <span class="label">Next</span>
    <span class="title">ne</span>
  </a>
</div>
