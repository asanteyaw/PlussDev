# DataFrame::load_parquet

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::load_parquet(std::string_view filepath) -> DataFrame

DataFrame::load_parquet(std::string_view filepath, const pluss::loader::Config& config) -> DataFrame

DataFrame::load_parquet(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) -> DataFrame
```

## What It Does

Loads the object from disk using the corresponding PlussMic loader route.

## Parameters

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

**config_entries**
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Load a PARQUET source with the default reader for that format.

```cpp
auto out = pluss::table::DataFrame::load_parquet("path/to/data.parquet");
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Load a PARQUET source with a reusable loader configuration object.

```cpp
pluss::loader::Config config{{"ncases", 8}};
auto out = pluss::table::DataFrame::load_parquet("path/to/data.parquet", config);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 3. Load a PARQUET source while supplying inline configuration entries.

```cpp
auto out = pluss::table::DataFrame::load_parquet("path/to/data.parquet", {
    {"ncases", 8},
});
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

## Practical Behavior

- `load_parquet(...)` loads one external source into typed PlussMic columns.
- Positional overloads are quick for ad-hoc work, while option/config overloads are better when you want repeatable loader settings.
- Excel multi-sheet overloads return `pluss::loader::LoadResult` because several frames may be produced at once.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_json.html">
    <span class="label">Previous</span>
    <span class="title">load_json</span>
  </a>
  <a class="next-link" href="load_txt.html">
    <span class="label">Next</span>
    <span class="title">load_txt</span>
  </a>
</div>
