<!-- pluss-hand-authored -->

# DataFrame::load

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
static DataFrame load(std::string_view source_type, std::string_view filepath) -> DataFrame
static DataFrame load(std::string_view source_type, std::string_view filepath, const pluss::loader::Config& config) -> DataFrame
static DataFrame load(std::string_view source_type, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) -> DataFrame
static DataFrame load(std::string_view source_type, std::string_view filepath, const pluss::loader::ReadCsvOptions& options) -> DataFrame
static DataFrame load(std::string_view source_type, std::string_view filepath, char sep, std::optional<std::size_t> ncases = std::nullopt, std::vector<std::string> usefeats = {}, std::optional<std::string> case_col = std::nullopt, pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error) -> DataFrame
static DataFrame load(std::string_view source_type, std::string_view filepath, char sep, std::optional<std::size_t> ncases, pluss::RangeView<std::string_view> usefeats, std::optional<std::string> case_col = std::nullopt, pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error) -> DataFrame
```

## What It Does

A single, format-switched entry point that dispatches to [`load_csv()`](load_csv.md),
[`load_txt()`](load_txt.md), [`load_excel()`](load_excel.md),
[`load_json()`](load_json.md), or [`load_parquet()`](load_parquet.md) based on a
runtime string, `source_type`. Useful when the file format isn't known until runtime
(config-driven pipelines, a generic "import this file" code path) — otherwise, calling
the format-specific loader directly is clearer and avoids the string-matching layer
entirely.

```{important} Not every overload supports every format
Only the `Config`/`ConfigEntry`-based overloads dispatch across **all five** formats.
The `ReadCsvOptions` overload and the positional (`sep`, `ncases`, `usefeats`,
`case_col`, `on_bad_lines`) overload only recognize `source_type` of `"csv"` or
`"txt"`/`"text"` — passing `"excel"`, `"json"`, or `"parquet"` to either of those two
overloads throws *"unsupported source type"*, even though the same string works fine
with the `Config`-based overloads. This is a real asymmetry in the overload set, not
an oversight in this documentation.
```

## Parameters

**source_type** (`std::string_view`)

The format selector, matched case-insensitively. Recognized values:
`"csv"`, `"txt"`/`"text"`, `"excel"`/`"xlsx"`, `"json"`, `"parquet"` (the last three
only via the `Config`/`ConfigEntry` overloads — see the caution above). Anything else
throws *"unsupported source type"*.

**filepath** (`std::string_view`)

Path to the file to read.

**config** (`const pluss::loader::Config&`) / **config_entries**
(`std::initializer_list<pluss::loader::ConfigEntry>`)

A reusable or inline settings bundle. Works with all five recognized `source_type`
values — each format's own loader (for example [`load_json()`](load_json.md)'s
`Config` overload) interprets the same key/value entries its dedicated loader would.

**options** (`const pluss::loader::ReadCsvOptions&`)

A typed CSV-parsing options struct, for the `"csv"`/`"txt"`/`"text"` source types only.

**sep**, **ncases**, **usefeats** (`std::vector<std::string>` or
`pluss::RangeView<std::string_view>`), **case_col**, **on_bad_lines**

Same meaning as the equivalent [`load_csv()`](load_csv.md) positional parameters, for
the `"csv"`/`"txt"`/`"text"` source types only.

## Returns

`DataFrame`
    A new, fully typed frame, exactly as if you had called the format-specific loader
    directly with equivalent settings.

## Examples

Example 1. Dispatch to the CSV loader by runtime string.

Given `orders.csv`:

```text
id,city,units
101,Accra,2
102,Tema,1
103,Kumasi,4
```

```cpp
using namespace pluss::table;

auto orders = DataFrame::load("csv", "orders.csv");
std::cout << orders;
```

### Output

```text
==========================
     | id    city    units
--------------------------
0    | 101   Accra   2    
1    | 102   Tema    1    
2    | 103   Kumasi  4    
--------------------------
[3 cs x 3 feat]
```

This produces exactly what `DataFrame::load_csv("orders.csv")` would.

Example 2. Dispatch with a `Config` bundle — works for any of the five formats.

```cpp
pluss::loader::Config config{{"ncases", std::int64_t{2}}};
auto preview = DataFrame::load("csv", "orders.csv", config);
std::cout << preview;
```

### Output

```text
==========================
     | id    city    units
--------------------------
0    | 101   Accra   2    
1    | 102   Tema    1    
--------------------------
[2 cs x 3 feat]
```

The same `config` object (and the same call shape) would work equally well with
`"json"`, `"parquet"`, or `"excel"` as `source_type`, dispatching to that format's
loader with the same `ncases` setting applied.

## Notes

- `load()` never invents its own parsing or type-inference logic — it is purely a
  string-keyed dispatch to the format-specific static loaders. Reading the dispatched
  method's own documentation ([`load_csv()`](load_csv.md), [`load_excel()`](load_excel.md),
  and so on) tells you exactly what a given call will do.
- The `ReadCsvOptions` and positional overloads are CSV/TXT-only; use the `Config`
  overload if the `source_type` might resolve to `"excel"`, `"json"`, or `"parquet"`
  at runtime.
- `load()` can never return a multi-sheet `pluss::loader::LoadResult` — even with
  `source_type = "excel"`, it always calls the single-frame [`load_excel()`](load_excel.md)
  overload. For multi-sheet loading, call `load_excel()` directly with its
  `sheet_names`/`sheet_indices`/`nullptr` overloads.

## See Also

- [`DataFrame::load_csv`](load_csv.md): the dedicated CSV loader, and the fullest
  explanation of the shared parsing/type-inference behavior.
- [`DataFrame::load_excel`](load_excel.md): the only loader with multi-sheet support,
  unreachable through `load()`.
- [`DataFrame::save_as`](save_as.md): the write-side counterpart, with the same
  format-selector string convention.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lengthen.html">
    <span class="label">Previous</span>
    <span class="title">lengthen</span>
  </a>
  <a class="next-link" href="load_csv.html">
    <span class="label">Next</span>
    <span class="title">load_csv</span>
  </a>
</div>
