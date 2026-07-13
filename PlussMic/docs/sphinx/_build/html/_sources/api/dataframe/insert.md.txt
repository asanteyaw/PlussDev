# DataFrame::insert

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::insert(std::int64_t feature_index, std::string name, const Thread& feature) -> DataFrame
```

## What It Does

Inserts a new feature at the requested position and shifts the later features to the right.

## Parameters

**feature_index**
    Zero-based output column position used by structural methods such as `insert(...)`.

**name**
    Name used to identify the target object, field, feature, or slot for this overload.

**feature**
    Name of the source feature used by the operation.

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

auto out = df.insert(1, "profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})));
```

### Output

```text
id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="info_string.html">
    <span class="label">Previous</span>
    <span class="title">info_string</span>
  </a>
  <a class="next-link" href="interp.html">
    <span class="label">Next</span>
    <span class="title">interp</span>
  </a>
</div>
