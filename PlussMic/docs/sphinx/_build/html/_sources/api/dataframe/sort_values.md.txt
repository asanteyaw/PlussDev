# DataFrame::sort_values

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::sort_values(pluss::RangeView<ArrangeSpec> specs) -> DataFrame

DataFrame::sort_values(pluss::RangeView<std::string_view> features, bool ascending = true, bool nulls_last = true) -> DataFrame
```

## What It Does

Reorders rows using one or more feature values as sort keys.

## Parameters

**specs**
    Collection of structured operation specs. Each element describes one requested summary, derive, transform, group, or placement rule depending on the method.

**features**
    Feature names used as sort keys when you want the same ascending/null policy for every key.

**ascending**
    When you use the simple feature-list overload, this one switch applies to every sort key. `true` sorts ascending and `false` sorts descending.

**nulls_last**
    When `true`, null values are placed after observed values for every sort key in the simple overload. When `false`, nulls are placed first.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Sort rows using explicit per-column sort specifications.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sort_values({{.feature = "sales", .ascending = false}});
```

### Output

```text
id  sales  city
2   120.0  Tema
1   100.0  Accra
3    90.0  Kumasi
```
Example 2. Sort rows using one shared ascending and null-order policy for every selected sort key.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sort_values({"sales"}, false, true);
```

### Output

```text
id  sales  city
2   120.0  Tema
1   100.0  Accra
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="sql.html">
    <span class="label">Next</span>
    <span class="title">sql</span>
  </a>
</div>
