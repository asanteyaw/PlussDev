# DataFrame::clip

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::clip(double min_value, double max_value) -> DataFrame
```

## What It Does

Applies an elementwise cleanup or transformation while preserving the surrounding alignment, labels, and payload family whenever possible.

## Parameters

**min_value**
    Lower/upper boundary used by this overload.

**max_value**
    Lower/upper boundary used by this overload.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Apply `clip` elementwise and inspect the returned result.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.clip();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cast.html">
    <span class="label">Previous</span>
    <span class="title">cast</span>
  </a>
  <a class="next-link" href="combine.html">
    <span class="label">Next</span>
    <span class="title">combine</span>
  </a>
</div>
