# DataFrame::save_json

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::save_json(std::string_view filepath) -> void

DataFrame::save_json(std::string_view filepath, const pluss::loader::Config& config) -> void

DataFrame::save_json(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) -> void
```

## What It Does

Writes the frame to JSON using the table export route for later reload or interchange.

## Parameters

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**config**
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

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

df.save_json("path/to/output.json");
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
df.save_json("path/to/output.json", config);
```

### Output

```text
saved: out.*
```
Example 3. Save the frame with inline configuration entries.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_json("path/to/output.json", {"indent", 2});
```

### Output

```text
saved: out.*
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_excel.html">
    <span class="label">Previous</span>
    <span class="title">save_excel</span>
  </a>
  <a class="next-link" href="save_parquet.html">
    <span class="label">Next</span>
    <span class="title">save_parquet</span>
  </a>
</div>
