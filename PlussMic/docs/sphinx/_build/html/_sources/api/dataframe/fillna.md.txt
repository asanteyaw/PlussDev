# DataFrame::fillna

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::fillna(const char* value) -> DataFrame

DataFrame::fillna(const char* value, pluss::RangeView<std::string_view> features) -> DataFrame

DataFrame::fillna(const pluss::TimeIndex& value) -> DataFrame

DataFrame::fillna(const pluss::TimeIndex& value, pluss::RangeView<std::string_view> features) -> DataFrame

DataFrame::fillna(const pluss::Timeline& value) -> DataFrame

DataFrame::fillna(const pluss::Timeline& value, pluss::RangeView<std::string_view> features) -> DataFrame

DataFrame::fillna(double value) -> DataFrame

DataFrame::fillna(double value, pluss::RangeView<std::string_view> features) -> DataFrame

DataFrame::fillna(pluss::RangeView<FillSpec> specs) -> DataFrame

DataFrame::fillna(std::initializer_list<FillSpec> specs) -> DataFrame

DataFrame::fillna(std::string_view value) -> DataFrame

DataFrame::fillna(std::string_view value, pluss::RangeView<std::string_view> features) -> DataFrame
```

## What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

## Parameters

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

**features**
    Feature name(s) that select the target DataFrame columns for the operation.

**specs**
    Collection of structured operation specs. Each element describes one requested summary, derive, transform, group, or placement rule depending on the method.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Fill missing values in a returned copy with a text scalar.

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

auto out = df.fillna("Unknown");
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Fill missing values in a returned copy only within a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

auto out = df.fillna("Unknown");
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 3. Fill missing values in a returned copy with a TimeIndex scalar.

```cpp
auto df = pluss::table::DataFrame({{"idx", pluss::table::Thread(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))}});

auto out = df.fillna(pluss::make_timeindex("2024-01-01T00:00:00Z"));
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 4. Fill missing values in a returned copy only within a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

auto out = df.fillna("Unknown");
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 5. Fill missing values in a returned copy with a Timeline scalar.

```cpp
auto df = pluss::table::DataFrame({{"ts", pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))}});

auto out = df.fillna(pluss::make_timeline("2024-01-01T00:00:00Z"));
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 6. Fill missing values in a returned copy only within a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

auto out = df.fillna("Unknown");
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 7. Fill missing values in a returned copy with a numeric scalar.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.fillna(0.0, {"sales"});
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 8. Fill missing values in a returned copy only within a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

auto out = df.fillna("Unknown");
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 9. Fill missing values in a returned copy with a numeric scalar.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.fillna(0.0, {"sales"});
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 10. Fill missing values in a returned copy with a numeric scalar.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.fillna(0.0, {"sales"});
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 11. Fill missing values in a returned copy with a text scalar.

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

auto out = df.fillna("Unknown");
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 12. Fill missing values in a returned copy only within a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

auto out = df.fillna("Unknown");
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ffill.html">
    <span class="label">Previous</span>
    <span class="title">ffill</span>
  </a>
  <a class="next-link" href="fillna_.html">
    <span class="label">Next</span>
    <span class="title">fillna_</span>
  </a>
</div>
