# DataFrame::load_csv

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::load_csv(std::string_view filepath) -> DataFrame

DataFrame::load_csv(std::string_view filepath, char sep, std::optional<std::size_t> ncases = std::nullopt, std::vector<std::string> usefeats = {}, std::optional<std::string> case_col = std::nullopt, pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error) -> DataFrame

DataFrame::load_csv(std::string_view filepath, char sep, std::optional<std::size_t> ncases, pluss::RangeView<std::string_view> usefeats, std::optional<std::string> case_col = std::nullopt, pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error) -> DataFrame

DataFrame::load_csv(std::string_view filepath, const pluss::loader::Config& config) -> DataFrame

DataFrame::load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options) -> DataFrame

DataFrame::load_csv(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) -> DataFrame

DataFrame::load_csv(std::string_view filepath, std::string_view header, std::size_t ncases) -> DataFrame

DataFrame::load_csv(std::string_view filepath, std::string_view header, std::string_view case_feature = "none", std::string_view engine = "auto", std::optional<std::size_t> ncases = std::nullopt) -> DataFrame
```

## What It Does

Loads the object from disk using the corresponding PlussMic loader route.

## Parameters

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

**header**
    CSV header mode.

    Common values:
    - `infer`: Read the first row as feature names.
    - `none`: Treat the file as headerless and auto-generate feature names such as `feat_0`, `feat_1`, and so on.

**case_feature**
    Name to use for the restored case-axis feature when cases are materialized back into ordinary columns.

**engine**
    Backend selector. Use `"auto"` to let PlussMic choose, or pass an explicit engine name when you need predictable routing.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Load a CSV file with the default parser settings.

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv");
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Load a CSV file with an explicit separator, row limit, and projected feature subset from a standard string container.

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", ',', 8, std::vector<std::string>{"Agency Name", "State"}, std::nullopt);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 3. Load a CSV file with an explicit separator, row limit, and projected feature subset from a lightweight string view.

```cpp
std::array<std::string_view, 2> usefeats{"Agency Name", "State"};
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", ',', 8, usefeats, std::nullopt);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 4. Load a CSV file with a reusable loader configuration object.

```cpp
pluss::loader::Config config{
    {"ncases", 8},
    {"features", {"Agency Name", "State"}},
};
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", config);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 5. Load a CSV file with a typed options object so parsing rules live in one reusable config.

```cpp
pluss::loader::ReadCsvOptions options;
options.ncases = 8;
options.case_col = "Agency Name";
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", options);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 6. Load a CSV file while supplying inline configuration entries such as row limits or projected features.

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", {
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
Example 7. Load a headerless CSV file and auto-generate feature names while limiting the number of rows read.

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", "none", static_cast<std::size_t>(8));
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 8. Load a CSV file while explicitly controlling header inference, case-axis promotion, engine selection, and row limit.

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", "infer", "Agency Name", "auto", 8);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

## Practical Behavior

- `load_csv(...)` loads one external source into typed PlussMic columns.
- Positional overloads are quick for ad-hoc work, while option/config overloads are better when you want repeatable loader settings.
- Excel multi-sheet overloads return `pluss::loader::LoadResult` because several frames may be produced at once.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load.html">
    <span class="label">Previous</span>
    <span class="title">load</span>
  </a>
  <a class="next-link" href="load_excel.html">
    <span class="label">Next</span>
    <span class="title">load_excel</span>
  </a>
</div>
