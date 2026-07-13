# DataFrame::load_excel

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::load_excel(std::string_view filepath) -> DataFrame

DataFrame::load_excel(std::string_view filepath, const pluss::loader::Config& config) -> DataFrame

DataFrame::load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options) -> DataFrame

DataFrame::load_excel(std::string_view filepath, const std::vector<std::int64_t>& sheet_indices, bool header = true) -> pluss::loader::LoadResult

DataFrame::load_excel(std::string_view filepath, const std::vector<std::string>& sheet_names, bool header = true) -> pluss::loader::LoadResult

DataFrame::load_excel(std::string_view filepath, pluss::RangeView<std::int64_t> sheet_indices, bool header = true) -> pluss::loader::LoadResult

DataFrame::load_excel(std::string_view filepath, pluss::RangeView<std::string_view> sheet_names, bool header = true) -> pluss::loader::LoadResult

DataFrame::load_excel(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) -> DataFrame

DataFrame::load_excel(std::string_view filepath, std::initializer_list<std::int64_t> sheet_indices, bool header = true) -> pluss::loader::LoadResult

DataFrame::load_excel(std::string_view filepath, std::initializer_list<std::string_view> sheet_names, bool header = true) -> pluss::loader::LoadResult

DataFrame::load_excel(std::string_view filepath, std::int64_t sheet_index, bool header = true) -> DataFrame

DataFrame::load_excel(std::string_view filepath, std::nullptr_t arg2, bool header = true) -> pluss::loader::LoadResult

DataFrame::load_excel(std::string_view filepath, std::string_view sheet_name, bool header = true) -> DataFrame
```

## What It Does

Loads the object from disk using the corresponding PlussMic loader route.

## Parameters

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**sheet_indices**
    Zero-based worksheet positions to load together from one Excel workbook.

**header**
    Controls whether feature names are written to, or expected from, the file header row.

**sheet_names**
    Worksheet names to load together from one Excel workbook. This overload returns a multi-sheet load result instead of one frame.

**config_entries**
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

**sheet_index**
    Zero-based worksheet position inside an Excel workbook.

**arg2**
    Pass `nullptr` here to request a multi-sheet Excel load result that includes every sheet.

**sheet_name**
    Worksheet name to read from or write to inside an Excel workbook.

## Returns

`DataFrame`
    Value produced by this call.

`pluss::loader::LoadResult`
    Multi-sheet Excel load bundle containing one frame per requested sheet.

## Examples

Example 1. Load the default worksheet from an Excel workbook.

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx");
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Load one Excel sheet with a reusable loader configuration object.

```cpp
pluss::loader::Config config{{"sheet_name", "Sheet 1"}};
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", config);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 3. Load one Excel sheet with a typed Excel options object.

```cpp
pluss::loader::ReadExcelOptions options;
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", options);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 4. Load several worksheets by zero-based sheet index from a standard integer container.

```cpp
std::vector<std::int64_t> sheets{0, 1};
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", sheets, true);
```

### Output

```text
Sheet1 -> [3 cs x 3 feat]
Sheet2 -> [3 cs x 3 feat]
```
Example 5. Load several worksheets by name from a standard string container.

```cpp
std::vector<std::string> sheets{"Sheet 1", "Sheet 2"};
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", sheets, true);
```

### Output

```text
Sheet1 -> [3 cs x 3 feat]
Sheet2 -> [3 cs x 3 feat]
```
Example 6. Load several worksheets by zero-based sheet index from a lightweight integer view.

```cpp
std::array<std::int64_t, 2> sheets{0, 1};
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", sheets, true);
```

### Output

```text
Sheet1 -> [3 cs x 3 feat]
Sheet2 -> [3 cs x 3 feat]
```
Example 7. Load several worksheets by name from a lightweight string view.

```cpp
std::array<std::string_view, 2> sheets{"Sheet 1", "Sheet 2"};
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", sheets, true);
```

### Output

```text
Sheet1 -> [3 cs x 3 feat]
Sheet2 -> [3 cs x 3 feat]
```
Example 8. Load an Excel workbook while supplying inline configuration entries instead of a named config object.

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", {
    {"sheet_name", "Sheet 1"},
});
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 9. Load several worksheets by zero-based sheet index using an inline integer list.

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", {0, 1}, true);
```

### Output

```text
Sheet1 -> [3 cs x 3 feat]
Sheet2 -> [3 cs x 3 feat]
```
Example 10. Load several worksheets by name using an inline list of sheet labels.

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", {"Sheet 1", "Sheet 2"}, true);
```

### Output

```text
Sheet1 -> [3 cs x 3 feat]
Sheet2 -> [3 cs x 3 feat]
```
Example 11. Load one worksheet by its zero-based sheet index.

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", 0, true);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 12. Load every worksheet from an Excel workbook into one multi-sheet load bundle.

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", nullptr, true);
```

### Output

```text
Sheet1 -> [3 cs x 3 feat]
Sheet2 -> [3 cs x 3 feat]
```
Example 13. Load one worksheet by its sheet name.

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", "Sheet 1", true);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

## Practical Behavior

- `load_excel(...)` loads one external source into typed PlussMic columns.
- Positional overloads are quick for ad-hoc work, while option/config overloads are better when you want repeatable loader settings.
- Excel multi-sheet overloads return `pluss::loader::LoadResult` because several frames may be produced at once.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_csv.html">
    <span class="label">Previous</span>
    <span class="title">load_csv</span>
  </a>
  <a class="next-link" href="load_json.html">
    <span class="label">Next</span>
    <span class="title">load_json</span>
  </a>
</div>
