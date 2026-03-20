# Loader Guide

`pluss::loader` in this codebase has two practical families:

- table/file I/O loaders (CSV, TXT, Excel, JSON, Parquet) from PlussMic
- runtime path utilities and tensor data-loader helpers re-exported from aux

For an exhaustive onboarding flow (including pathing + DataLoader end-to-end patterns), see [Aux Tutorial](aux_tutorial.md).

## Includes

For table/DataFrame I/O:

```cpp
#include <pluss/mic.h>
```

For aux-backed loader/path/data-loader utilities:

```cpp
#include <pluss/pluss.h>
```

## 1. Config-Driven Loader Surface

### API Signature

`pluss::loader::ConfigValue(value) -> ConfigValue`  
Inputs:
- `value`: one of `bool`, `int64`, `double`, `string`, `vector<string>`.
Output:
- typed config value.

`pluss::loader::config_from(entries) -> Config`  
Inputs:
- `entries`: key/value list.
Output:
- `Config` map for loader APIs.

### Arguments

- `Config` is convenient for dynamic parameter wiring.
- For most workflows, typed options (`ReadCsvOptions`, `ReadExcelOptions`) are clearer and safer.

### Example

```cpp
pluss::loader::Config cfg = pluss::loader::config_from({
  {"header", true},
  {"sep", ","},
  {"ncases", static_cast<std::int64_t>(1000)},
  {"usefeats", std::vector<std::string>{"Open", "Close"}}
});
```

Expected output behavior:

```text
cfg contains 4 keys with typed values
```

## 2. CSV/TXT Loading

### API Signature

`pluss::loader::load_csv(options) -> DataFrame`  
`pluss::loader::load_csv(filepath, options = {}) -> DataFrame`  
`pluss::loader::load_txt(filepath, config = {}) -> DataFrame`  
`pluss::loader::load_csv_records(records, options = {}) -> DataFrame`

Inputs:
- `filepath`: source CSV/TXT path.
- `options`: typed CSV options (`sep`, header, selected features, row bounds, NA behavior, bad-line policy, engine).
- `records`: in-memory table rows.
Output:
- loaded `DataFrame`.

### Arguments

- `sep`: if explicitly assigned (for example `opts.sep = ' '`), that separator is used directly.
- `auto_detect_sep`: used only when separator is not explicitly set.
- `usefeats`: select a subset of feature names.
- `case_col`: choose a feature to become the case/index column.
- `on_bad_lines`: strict fail, warn, or skip malformed rows.

### Example A: deterministic in-memory records

```cpp
std::vector<std::vector<std::string>> records = {
  {"id", "name", "score"},
  {"1", "Ada", "10"},
  {"2", "Lin", "12"}
};

auto df = pluss::loader::load_csv_records(records);

pluss::OutputEngine out;
out.writeln("ncases", df.ncases());
out.writeln("nfeatures", df.nfeatures());
out.writeln("feature_names", df.feature_names());
```

Expected output:

```text
ncases 2
nfeatures 3
feature_names [id, name, score]
```

### Example B: space-separated file

```cpp
pluss::loader::ReadCsvOptions opts;
opts.filepath = "data/my_space_separated.txt";
opts.sep = ' ';            // explicit separator
// no need to set opts.auto_detect_sep = false

auto df = pluss::loader::load_csv(opts);
```

Expected output behavior:

```text
Loader uses ' ' as separator directly
Auto detection is bypassed because sep was explicitly set
```

### Example C: feature subset and case selection

```cpp
pluss::loader::ReadCsvOptions opts;
opts.filepath = "data/bank-full.csv";
opts.usefeats = {"age", "job", "balance"};
opts.case_col = "age";
opts.ncases = 500;

auto df = pluss::loader::load_csv(opts.filepath, opts);
```

Expected output behavior:

```text
Loaded up to 500 rows
Only selected features present
Case/index derived from "age"
```

## 3. Excel Loading

### API Signature

`pluss::loader::load_excel(filepath, options = {}) -> DataFrame`  
`pluss::loader::load_excel_workbook(filepath, options = {}) -> Workbook`  
`pluss::loader::load_excel(filepath, sheet_name, header = true) -> DataFrame`  
`pluss::loader::load_excel(filepath, sheet_index, header = true) -> DataFrame`

Inputs:
- `filepath`: Excel file path.
- `options`: sheet selection, header policy, row/column window.
- `sheet_name` / `sheet_index`: convenience single-sheet selectors.
Output:
- single-sheet `DataFrame` or multi-sheet `Workbook`.

### Arguments

- `sheet_name` / `sheet_index`: choose one sheet.
- `sheet_names` / `sheet_indices` / `all_sheets`: multi-sheet selection.
- `header_rows + header_joiner`: flatten multi-row headers.
- `start_*` / `end_*`: bounded region read.

### Example

```cpp
pluss::loader::ReadExcelOptions opts;
opts.sheet_name = "Prices";
opts.header = true;

auto prices = pluss::loader::load_excel("data/market.xlsx", opts);

auto wb = pluss::loader::load_excel_workbook("data/market.xlsx", {
  .all_sheets = true
});
```

Expected output behavior:

```text
prices contains only the "Prices" sheet
wb contains every sheet as (sheet_name, DataFrame)
```

## 4. JSON/Parquet and Generic Delegating Loader

### API Signature

`pluss::loader::load_json(filepath, config = {}) -> DataFrame`  
`pluss::loader::load_parquet(filepath, config = {}) -> DataFrame`  
`pluss::loader::load(source_type, filepath, config = {}) -> LoadResult`

Inputs:
- `source_type`: format key (for example `csv`, `excel`, `json`, `parquet`, `txt`).
- `filepath`: source location.
- `config`: format-specific dynamic options.
Output:
- typed frame/workbook wrapper (`LoadResult`) or direct `DataFrame`.

### Arguments

- `source_type` supports format dispatch (for example: `"csv"`, `"excel"`, `"json"`, `"parquet"`, `"txt"`).
- Prefer direct `load_csv/load_excel/...` when source type is known at compile-time.

### Example

```cpp
auto a = pluss::loader::load("csv", "data/bank-full.csv");
auto b = pluss::loader::load("excel", "data/report.xlsx");

if (a.is_frame()) {
  std::cout << a.dataframe().ncases() << "\n";
}
if (b.is_workbook()) {
  std::cout << b.sheet_names() << "\n";
}
```

Expected output behavior:

```text
CSV returns frame-style LoadResult
Workbook-style loads expose sheet_names()
```

## 5. Saving DataFrames

### API Signature

`pluss::loader::save_csv(frame, filepath, config = {}) -> void`  
`pluss::loader::save_excel(frame, filepath, config = {}) -> void`  
`pluss::loader::save_txt(frame, filepath, config = {}) -> void`  
`pluss::loader::save_json(frame, filepath, config = {}) -> void`  
`pluss::loader::save_parquet(frame, filepath, config = {}) -> void`  
`pluss::loader::save_as(frame, target_type, filepath, config = {}) -> void`

Inputs:
- `frame`: source `DataFrame`.
- `target_type`: format key for generic saver.
- `filepath`: output path.
- `config`: writer options.
Output:
- none (file emitted).

### Arguments

- `target_type` in `save_as` mirrors format routing (`"csv"`, `"excel"`, ...).
- format-specific options are read from `Config`.

### Example

```cpp
auto df = pluss::loader::load_csv("data/bank-full.csv", {
  {"ncases", static_cast<std::int64_t>(100)}
});

pluss::loader::save_as(df, "json", "outputs/bank_100.json");
pluss::loader::save_csv(df, "outputs/bank_100.csv");
```

Expected output behavior:

```text
Two files created in outputs/
Row/feature shape preserved across save/load round-trips
```

## 6. Path Utilities in `pluss::loader`

These come from aux pathing and are useful for stable, environment-aware path resolution.

### API Signature

`pluss::loader::project_root() -> path`  
`pluss::loader::set_project_root(root) -> void`  
`pluss::loader::reset_project_root() -> void`  
`pluss::loader::set_path_root(key, root) -> void`  
`pluss::loader::path_root(key, create_dirs = false) -> path`  
`pluss::loader::has_path_root(key) -> bool`  
`pluss::loader::clear_path_root(key) -> void`  
`pluss::loader::path_join(base, parts...) -> path`  
`pluss::loader::path_file(key, name, ensure_parent = false) -> path`  
`pluss::loader::data_file(name) -> path`  
`pluss::loader::output_dir(name = "") -> path`  
`pluss::loader::model_dir(name = "") -> path`

Inputs:
- logical `key` + path components.
Output:
- resolved path or mutating side effect.

### Arguments

- keys such as `"data"`, `"output"`, `"model"`, `"qit"`, `"mic"`, `"ma"`, `"ai"` resolve to library-aware defaults.
- env overrides are supported (for example `PLUSS_PATH_DATA`, `PLUSS_OUTPUT_DIR`).

### Example

```cpp
using namespace pluss::loader;

set_path_root("output", "./outputs");
auto run_dir = output_dir("nodej_cv");
auto pred_file = path_file("output", "nodej_cv/preds.csv", true);

std::cout << run_dir << "\n";
std::cout << pred_file << "\n";
```

Expected output:

```text
./outputs/nodej_cv
./outputs/nodej_cv/preds.csv
```

## 7. Tensor DataLoader APIs under `pluss::loader`

These are documented in detail in [Aux User Guide](aux.md).

Quick reminder signature:

`pluss::loader::SeriesDataset(fields, batch_dim = 0) -> SeriesDataset`  
`pluss::loader::DataLoader(dataset_or_fields, options = {}) -> DataLoader`  
`pluss::loader::split_fields(...) -> DataSplit`  
`pluss::loader::split_loaders(...) -> DataLoaderSplit`  
`pluss::loader::time_series_split_indices(...) -> TimeSeriesIndexSplit`  
`pluss::loader::time_series_split_fields(...) -> TimeSeriesFieldSplit`  
`pluss::loader::time_series_split_loaders(...) -> TimeSeriesLoaderSplit`

## 8. End-to-End Example (CSV -> filter -> save)

```cpp
pluss::OutputEngine out;

auto df = pluss::loader::load_csv("data/bank-full.csv", {
  {"ncases", static_cast<std::int64_t>(1000)},
  {"usefeats", std::vector<std::string>{"age", "job", "balance"}}
});

out.writeln("shape", df.ncases(), df.nfeatures());
out.writeln("features", df.feature_names());

pluss::loader::save_json(df, "outputs/bank_subset.json");
out.writeln("saved", "outputs/bank_subset.json");
```

Expected output (example):

```text
shape 1000 3
features [age, job, balance]
saved outputs/bank_subset.json
```
