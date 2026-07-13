# DataFrame::save_as

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::save_as(std::string_view target_type, std::string_view filepath, const pluss::loader::Config& config = {}) -> void

DataFrame::save_as(std::string_view target_type, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) -> void
```

## What It Does

Writes the frame using the explicitly supplied output format selector rather than a dedicated saver overload.

## Parameters

**target_type**
    Format or target selector string used by this overload.

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

Example 1. Save the frame with an explicit loader-config object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

pluss::loader::Config config{};
df.save_as("json", "path/to/output.json", config);
```

### Output

```text
saved: out.*
```
Example 2. Save the frame with inline configuration entries.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_as("json", "path/to/output.json", {"indent", 2});
```

### Output

```text
saved: out.*
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sample.html">
    <span class="label">Previous</span>
    <span class="title">sample</span>
  </a>
  <a class="next-link" href="save_csv.html">
    <span class="label">Next</span>
    <span class="title">save_csv</span>
  </a>
</div>
