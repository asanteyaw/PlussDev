# DataFrame::save_excel

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::save_excel(std::string_view filepath) -> void

DataFrame::save_excel(std::string_view filepath, const pluss::loader::Config& config) -> void

DataFrame::save_excel(std::string_view filepath, const pluss::loader::WriteExcelOptions& options) -> void

DataFrame::save_excel(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) -> void

DataFrame::save_excel(std::string_view filepath, std::string sheet_name, bool add_cases = false, bool header = true, std::string null_rep = "", std::optional<std::string> case_label = std::nullopt) -> void
```

## What It Does

Writes the frame to an Excel sheet. Set `add_cases=true` when you want the current case axis exported as a regular column.

## Parameters

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**config_entries**
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

**sheet_name**
    Worksheet name to read from or write to inside an Excel workbook.

**add_cases**
    When `true`, the current case axis is exported as its own column so row labels can be reconstructed when the file is loaded again.

**header**
    Controls whether feature names are written to, or expected from, the file header row.

**null_rep**
    String written anywhere a null or missing value must be serialized.

**case_label**
    Optional column label to use for the exported case axis when `add_cases=true`.

## Returns

`void`
    Performs the side effect and returns no value.

## Examples

Example 1. Save the frame using the simplest filepath-only overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_excel("path/to/output.xlsx");
```

### Output

```text
saved: out.*
```
Example 2. Save the frame with an explicit loader-config object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

pluss::loader::Config config{};
df.save_excel("path/to/output.xlsx", config);
```

### Output

```text
saved: out.*
```
Example 3. Save the frame with a typed writer-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

pluss::loader::WriteExcelOptions options;
df.save_excel("path/to/output.xlsx", options);
```

### Output

```text
saved: out.*
```
Example 4. Save the frame with inline configuration entries.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_excel("path/to/output.xlsx", {"indent", 2});
```

### Output

```text
saved: out.*
```
Example 5. Save the frame while controlling case-axis export and file-format details explicitly.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_csv("path/to/output.csv", true, true, ',', "NA", "case_id");
```

### Output

```text
saved: out.*
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_csv.html">
    <span class="label">Previous</span>
    <span class="title">save_csv</span>
  </a>
  <a class="next-link" href="save_json.html">
    <span class="label">Next</span>
    <span class="title">save_json</span>
  </a>
</div>
