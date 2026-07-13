# DataFrame::map

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::map(const std::function<ThreadValue(const ThreadValue&)>& fn, pluss::RangeView<std::string_view> features = {}) -> DataFrame
```

## What It Does

Adds, replaces, filters, or rewrites part of the current object while preserving the surrounding alignment model. Methods ending with `_` perform the change in place.

## Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

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

auto out = df.map();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lt.html">
    <span class="label">Previous</span>
    <span class="title">lt</span>
  </a>
  <a class="next-link" href="mask.html">
    <span class="label">Next</span>
    <span class="title">mask</span>
  </a>
</div>
