# DataFrame::isnull

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::isnull(pluss::RangeView<std::string_view> features = {}) -> DataFrame
```

## What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

## Parameters

**features**
    Feature name(s) that select the target DataFrame columns for the operation.

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

auto out = df.isnull({"sales"});
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="interp.html">
    <span class="label">Previous</span>
    <span class="title">interp</span>
  </a>
  <a class="next-link" href="join.html">
    <span class="label">Next</span>
    <span class="title">join</span>
  </a>
</div>
