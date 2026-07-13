# DataFrame::load

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::load(std::string_view source_type, std::string_view filepath) -> DataFrame

DataFrame::load(std::string_view source_type, std::string_view filepath, char sep, std::optional<std::size_t> ncases = std::nullopt, std::vector<std::string> usefeats = {}, std::optional<std::string> case_col = std::nullopt, pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error) -> DataFrame

DataFrame::load(std::string_view source_type, std::string_view filepath, char sep, std::optional<std::size_t> ncases, pluss::RangeView<std::string_view> usefeats, std::optional<std::string> case_col = std::nullopt, pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error) -> DataFrame

DataFrame::load(std::string_view source_type, std::string_view filepath, const pluss::loader::Config& config) -> DataFrame

DataFrame::load(std::string_view source_type, std::string_view filepath, const pluss::loader::ReadCsvOptions& options) -> DataFrame

DataFrame::load(std::string_view source_type, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) -> DataFrame
```

## What It Does

Loads the object from disk using the corresponding PlussMic loader route.

## Parameters

**source_type**
    Storage format selector such as `"csv"`, `"xlsx"`, `"json"`, or `"parquet"`.

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**sep**
    Column separator used for delimited text files such as CSV.

**ncases**
    Optional row limit applied while loading. Leave unset to read the whole source.

**usefeats**
    Feature names to load from the source. Leave empty to load every available feature.

**case_col**
    Optional source column that should become the DataFrame case axis while loading.

**on_bad_lines**
    Malformed-line policy for delimited input.

    Supported values:
    - `Error`: Stop immediately when a malformed row is encountered.
    - `Warn`: Continue while reporting the malformed row.
    - `Skip`: Ignore malformed rows silently.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**config_entries**
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Load a runtime-selected source type with the default reader for that format.

```cpp
auto out = pluss::table::DataFrame::load("csv", "path/to/data.csv");
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Load a runtime-selected delimited source while overriding the separator, row limit, and projected feature list.

```cpp
auto out = pluss::table::DataFrame::load("csv", "path/to/data.csv", ',', 8, std::vector<std::string>{"Agency Name", "State"}, std::nullopt);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 3. Load a runtime-selected delimited source while projecting only a selected feature subset from a lightweight string view.

```cpp
std::array<std::string_view, 2> usefeats{"Agency Name", "State"};
auto out = pluss::table::DataFrame::load("csv", "path/to/data.csv", ',', 8, usefeats, std::nullopt);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 4. Load a runtime-selected source type with a reusable loader configuration object.

```cpp
pluss::loader::Config config{
    {"ncases", 8},
    {"features", {"Agency Name", "State"}},
};
auto out = pluss::table::DataFrame::load("csv", "path/to/data.csv", config);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 5. Load a delimited file through the format-switched entry point while controlling parsing with a typed CSV options object.

```cpp
pluss::loader::ReadCsvOptions options;
options.ncases = 8;
options.case_col = "Agency Name";
auto out = pluss::table::DataFrame::load("csv", "path/to/data.csv", options);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 6. Load a runtime-selected source type while passing inline loader configuration entries.

```cpp
auto out = pluss::table::DataFrame::load("csv", "path/to/data.csv", {
    {"ncases", 8},
    {"features", {"Agency Name", "State"}},
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

- `load(...)` is the format-switched entry point behind the explicit file loaders.
- Use it when the source format is chosen at runtime; otherwise, the format-specific loader is clearer.
- CSV-style overloads let you control separator, row limits, feature projection, case-axis promotion, and bad-line policy.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="levels.html">
    <span class="label">Previous</span>
    <span class="title">levels</span>
  </a>
  <a class="next-link" href="load_csv.html">
    <span class="label">Next</span>
    <span class="title">load_csv</span>
  </a>
</div>
