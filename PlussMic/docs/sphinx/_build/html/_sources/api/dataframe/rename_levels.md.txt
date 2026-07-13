# DataFrame::rename_levels

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::rename_levels(pluss::RangeView<LevelRename> renames) -> DataFrame

DataFrame::rename_levels(std::initializer_list<LevelRename> renames) -> DataFrame
```

## What It Does

Renames one or more case-axis levels without changing row order or row values.

## Parameters

**renames**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

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

auto out = df.rename_levels();
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

auto out = df.rename_levels();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rename_.html">
    <span class="label">Previous</span>
    <span class="title">rename_</span>
  </a>
  <a class="next-link" href="reorder.html">
    <span class="label">Next</span>
    <span class="title">reorder</span>
  </a>
</div>
