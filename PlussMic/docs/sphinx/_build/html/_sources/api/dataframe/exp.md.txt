# DataFrame::exp

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::exp() -> DataFrame
```

## What It Does

Applies an elementwise cleanup or transformation while preserving the surrounding alignment, labels, and payload family whenever possible.

## Parameters

`None`

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Apply `exp` elementwise and inspect the returned result.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.exp();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="equals.html">
    <span class="label">Previous</span>
    <span class="title">equals</span>
  </a>
  <a class="next-link" href="extend.html">
    <span class="label">Next</span>
    <span class="title">extend</span>
  </a>
</div>
