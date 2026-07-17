# DataFrame::save_csv

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::save_csv(std::string_view filepath) -> void

DataFrame::save_csv(std::string_view filepath, bool add_cases, bool header = true, char sep = ',', std::string null_rep = "", std::optional<std::string> case_label = std::nullopt, std::optional<std::string> float_format = std::nullopt, char quotechar = '"', bool doublequote = true, std::string line_terminator = "\n", char decimal = '.', std::string encoding = "utf-8") -> void

DataFrame::save_csv(std::string_view filepath, const pluss::loader::Config& config) -> void

DataFrame::save_csv(std::string_view filepath, const pluss::loader::WriteCsvOptions& options) -> void

DataFrame::save_csv(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) -> void
```

## What It Does

Writes the frame to CSV. Set `add_cases=true` when you want the current case axis exported as a regular column so row labels survive the round trip.

## Parameters

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**add_cases**
    When `true`, the current case axis is exported as its own column so row labels can be reconstructed when the file is loaded again.

**header**
    Controls whether feature names are written to, or expected from, the file header row.

**sep**
    Column separator used for delimited text files such as CSV.

**null_rep**
    String written anywhere a null or missing value must be serialized.

**case_label**
    Optional column label to use for the exported case axis when `add_cases=true`.

**float_format**
    Optional numeric formatting string used when floating-point values are written to text output.

**quotechar**
    Quote character used when writing delimited text output.

**doublequote**
    When `true`, embedded quote characters are escaped by doubling them in CSV-style output.

**line_terminator**
    Line ending written between serialized rows in text-based export.

**decimal**
    Decimal marker used when formatting floating-point output.

**encoding**
    Text encoding used when reading or writing text-based files.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**config_entries**
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

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

df.save_csv("path/to/output.csv");
```

### Output

```text
saved: out.*
```
Example 2. Save the frame while controlling case-axis export and file-format details explicitly.

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
Example 3. Save the frame with an explicit loader-config object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

pluss::loader::Config config{};
df.save_csv("path/to/output.csv", config);
```

### Output

```text
saved: out.*
```
Example 4. Save the frame with a typed writer-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

pluss::loader::WriteCsvOptions options;
df.save_csv("path/to/output.csv", options);
```

### Output

```text
saved: out.*
```
Example 5. Save the frame with inline configuration entries.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_csv("path/to/output.csv", {"indent", 2});
```

### Output

```text
saved: out.*
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_as.html">
    <span class="label">Previous</span>
    <span class="title">save_as</span>
  </a>
  <a class="next-link" href="save_excel.html">
    <span class="label">Next</span>
    <span class="title">save_excel</span>
  </a>
</div>
