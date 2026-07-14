# DataFrame::abs

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::abs() -> DataFrame
```

## What It Does

Applies an elementwise cleanup or transformation while preserving the surrounding alignment, labels, and payload family whenever possible.

## Parameters

`None`

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Apply `abs` elementwise and inspect the returned result.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.abs();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="next-link" href="add.html">
    <span class="label">Next</span>
    <span class="title">add</span>
  </a>
</div>
