# DataFrame::fillna_

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::fillna_(const char* value) -> DataFrame&

DataFrame::fillna_(const pluss::TimeIndex& value) -> DataFrame&

DataFrame::fillna_(const pluss::Timeline& value) -> DataFrame&

DataFrame::fillna_(double value) -> DataFrame&

DataFrame::fillna_(std::string_view value) -> DataFrame&
```

## What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

## Parameters

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

## Returns

`DataFrame&`
    Value produced by this call.

## Examples

Example 1. Fill missing values in place with a text scalar.

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

df.fillna_("Unknown");
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Fill missing values in place with a TimeIndex scalar.

```cpp
auto df = pluss::table::DataFrame({{"idx", pluss::table::Thread(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))}});

df.fillna_(pluss::make_timeindex("2024-01-01T00:00:00Z"));
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 3. Fill missing values in place with a Timeline scalar.

```cpp
auto df = pluss::table::DataFrame({{"ts", pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))}});

df.fillna_(pluss::make_timeline("2024-01-01T00:00:00Z"));
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 4. Fill missing values in place with a numeric scalar.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.fillna_(0.0);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 5. Fill missing values in place with a text scalar.

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

df.fillna_("Unknown");
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fillna.html">
    <span class="label">Previous</span>
    <span class="title">fillna</span>
  </a>
  <a class="next-link" href="filter.html">
    <span class="label">Next</span>
    <span class="title">filter</span>
  </a>
</div>
