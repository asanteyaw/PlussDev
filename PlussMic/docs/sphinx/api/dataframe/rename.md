# DataFrame::rename

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::rename(pluss::RangeView<std::pair<std::string_view, std::string_view>> renames) -> DataFrame

DataFrame::rename(pluss::RangeView<std::string_view> feature_names) -> DataFrame

DataFrame::rename(std::initializer_list<const char*> feature_names) -> DataFrame

DataFrame::rename(std::initializer_list<std::pair<std::string_view, std::string_view>> renames) -> DataFrame

DataFrame::rename(std::string_view old_name, std::string new_name) -> DataFrame
```

## What It Does

Renames one feature, several features, or the full feature axis without changing row values.

## Parameters

**renames**
    Collection of `(old_name, new_name)` pairs applied in one rename pass.

**feature_names**
    Full replacement feature-label list in final left-to-right order. The count must match the current number of features.

**old_name**
    Existing feature name to rename.

**new_name**
    Replacement feature name.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Rename several columns at once with a reusable old-name/new-name mapping.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

std::vector<std::pair<std::string_view, std::string_view>> renames{{"sales", "revenue"}, {"city", "location"}};
auto out = df.rename(renames);
```

### Output

```text
id  revenue  city
1   100.0    Accra
2   120.0    Tema
3    90.0    Kumasi
```
Example 2. Replace the full feature axis by passing every final column name in order.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

std::vector<std::string_view> names{"id_new", "revenue", "location"};
auto out = df.rename(names);
```

### Output

```text
id  revenue  city
1   100.0    Accra
2   120.0    Tema
3    90.0    Kumasi
```
Example 3. Replace the full feature axis by listing every final column name inline as C strings.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.rename({"id_new", "revenue", "location"});
```

### Output

```text
id  revenue  city
1   100.0    Accra
2   120.0    Tema
3    90.0    Kumasi
```
Example 4. Rename several columns at once with an inline old-name/new-name mapping.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.rename({{"sales", "revenue"}, {"city", "location"}});
```

### Output

```text
id  revenue  city
1   100.0    Accra
2   120.0    Tema
3    90.0    Kumasi
```
Example 5. Replace the full feature axis by passing every final column name in order.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.rename("sales", "revenue");
```

### Output

```text
id  revenue  city
1   100.0    Accra
2   120.0    Tema
3    90.0    Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rdiv.html">
    <span class="label">Previous</span>
    <span class="title">rdiv</span>
  </a>
  <a class="next-link" href="rename_.html">
    <span class="label">Next</span>
    <span class="title">rename_</span>
  </a>
</div>
