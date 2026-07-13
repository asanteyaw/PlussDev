# DataFrame::ffill

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::ffill(const FillOptions& options) -> DataFrame

DataFrame::ffill(std::int64_t limit = -1) -> DataFrame
```

## What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

## Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**limit**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Pass an explicit options object to control the operation.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.ffill();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.ffill();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="features.html">
    <span class="label">Previous</span>
    <span class="title">features</span>
  </a>
  <a class="next-link" href="fillna.html">
    <span class="label">Next</span>
    <span class="title">fillna</span>
  </a>
</div>
