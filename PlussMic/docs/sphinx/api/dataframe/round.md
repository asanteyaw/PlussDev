# DataFrame::round

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::round(pluss::RangeView<std::string_view> features, std::string_view unit) -> DataFrame
```

## What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

## Parameters

**features**
    Feature name(s) that select the target DataFrame columns for the operation.

**unit**
    Temporal rounding unit such as `"hour"`, `"day"`, or `"month"`.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.round();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rolling.html">
    <span class="label">Previous</span>
    <span class="title">rolling</span>
  </a>
  <a class="next-link" href="rpow.html">
    <span class="label">Next</span>
    <span class="title">rpow</span>
  </a>
</div>
