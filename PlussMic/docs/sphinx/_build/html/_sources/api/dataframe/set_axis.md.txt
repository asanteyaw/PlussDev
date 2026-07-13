# DataFrame::set_axis

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::set_axis(pluss::RangeView<std::string_view> feature_names) -> DataFrame
```

## What It Does

Replaces the full feature label set with the supplied names in positional order.

## Parameters

**feature_names**
    Full replacement feature-label list in final left-to-right order. The count must match the current number of features.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.set_axis({"revenue", "location", "id_new"});
```

### Output

```text
revenue  location  id_new
100.0    Accra     1
120.0    Tema      2
90.0     Kumasi    3
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set.html">
    <span class="label">Previous</span>
    <span class="title">set</span>
  </a>
  <a class="next-link" href="set_cases.html">
    <span class="label">Next</span>
    <span class="title">set_cases</span>
  </a>
</div>
