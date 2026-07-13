# DataFrame::interp

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::interp(const FillOptions& options) -> DataFrame

DataFrame::interp(pluss::RangeView<std::string_view> features = {}, std::int64_t limit = -1) -> DataFrame
```

## What It Does

Interpolates missing numeric values across selected features using the requested interpolation strategy.

## Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**features**
    Feature name(s) that select the target DataFrame columns for the operation.

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

auto out = df.interp();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.interp();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="insert.html">
    <span class="label">Previous</span>
    <span class="title">insert</span>
  </a>
  <a class="next-link" href="isnull.html">
    <span class="label">Next</span>
    <span class="title">isnull</span>
  </a>
</div>
