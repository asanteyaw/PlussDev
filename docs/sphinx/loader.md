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
#include <pluss/tils.h>
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
- `filepath`: source CSV/TXT path or `http(s)` URL.
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
- `filepath` URL mode:
  - `load_csv` and `load_txt` accept `http://...` and `https://...` and download to a temporary local file before parsing.

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
out.writeln("feature_names", df.features());
```

Expected output:

```text
ncases 2
nfeatures 3
feature_names [id, name, score]
```

### Example B: load CSV from URL

```cpp
auto df = pluss::loader::load_csv(
  "https://raw.githubusercontent.com/DanikaLipman/Stat217/refs/heads/main/example5.csv"
);
```

Expected output behavior:
- remote CSV is downloaded and parsed as a regular DataFrame.

### Example B.1: space-separated file

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
- `filepath`: Excel file path or `http(s)` URL.
- `options`: sheet selection, header policy, row/column window.
- `sheet_name` / `sheet_index`: convenience single-sheet selectors.
Output:
- single-sheet `DataFrame` or multi-sheet `Workbook`.

### Arguments

- `sheet_name` / `sheet_index`: choose one sheet.
- `sheet_names` / `sheet_indices` / `all_sheets`: multi-sheet selection.
- `header_rows + header_joiner`: flatten multi-row headers.
- `start_*` / `end_*`: bounded region read.
- `filepath` URL mode:
  - `load_excel` and `load_excel_workbook` accept `http://...` and `https://...` and download to a temporary local file before parsing.

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

These APIs provide a consistent pathing system for both local files and internet-hosted datasets.

### 6.1 Mental model

Use one of three root styles depending on workflow:

- Global local root (`set_root_path(...)`): simplest for single-project scripts/notebooks.
- Scoped local roots (`PathContext::from_root(...)`): best for multi-root programs.
- Scoped internet roots (`UrlContext::from_root("https://...")`): best for repeatable remote dataset layouts.

### 6.2 API surface

Global local root:

`set_root_path(root, resolve_mode = RootResolve::WorkingDirectory) -> void`  
`root_path() -> path`  
`reset_root_path() -> void`  
`set_path_policy(policy) -> void`  
`path_policy() -> PathPolicy`  
`file_path(subpath, options = {}) -> PathRef`  
`file_paths({...}, options = {}) -> std::vector<PathRef>`  
`file_path_map({{"name","subpath"}, ...}, options = {}) -> std::unordered_map<std::string, PathRef>`  
`file_dir(subpath, options = {}) -> DirRef`  
`absolute_path(path, options = {}) -> PathRef`

Scoped local root:

`PathContext::from_root(root, resolve_mode = RootResolve::WorkingDirectory) -> PathContext`  
`ctx.file_path(...)`, `ctx.file_paths(...)`, `ctx.file_path_map(...)`, `ctx.file_dir(...)`, `ctx.absolute_path(...)`

Scoped internet root:

`UrlContext::from_root("https://...") -> UrlContext`  
`u.root() -> std::string`  
`u.file_path(subpath) -> UrlRef`  
`u.file_paths({...}) -> std::vector<UrlRef>`  
`u.file_path_map({...}) -> std::unordered_map<std::string, UrlRef>`

String accessors:

`PathRef::string() -> std::string`  
`DirRef::string() -> std::string`  
`DirRef::file(subpath, options = {}) -> PathRef`  
`UrlRef::string() -> std::string`

### 6.3 Global root workflows (`set_root_path`)

```cpp
using namespace pluss::loader;

set_root_path("../"); // project root relative to current working directory

auto raw_csv  = file_path("all_data/housing.csv");
auto out_dir  = file_dir("main_outputs/exp01", {.create_dirs = true});
auto pred_csv = out_dir.file("predictions.csv");

auto df = load_csv(raw_csv.string());
save_csv(df, pred_csv.string());
```

Expected behavior:

```text
all paths are anchored under the configured project root
output directory is created automatically for this call
```

### 6.4 Batch path helpers (many files)

```cpp
using namespace pluss::loader;

set_root_path("/abs/path/to/proj");

auto files = file_paths({
  "all_data/housing.csv",
  "all_data/prices.csv",
  "all_data/fossils.xlsx"
});

auto named = file_path_map({
  {"housing", "all_data/housing.csv"},
  {"prices",  "all_data/prices.csv"},
  {"fossils", "all_data/fossils.xlsx"}
});

auto housing = load_csv(named.at("housing").string());
auto prices  = load_csv(named.at("prices").string());
auto fossil  = load_excel(named.at("fossils").string());
```

Expected behavior:

```text
ordered access for file_paths
name-addressable access for file_path_map
duplicate keys throw
```

### 6.5 Directory-first workflows (`file_dir`)

```cpp
using namespace pluss::loader;

set_root_path("/abs/path/to/proj");

auto model_dir = file_dir("trained_models/ngarch", {.create_dirs = true});
auto model_bin = model_dir.file("model.bin");
auto metrics   = model_dir.file("metrics.json");
```

Expected behavior:

```text
model_dir resolves as a directory target under root
model_bin/metrics are guaranteed to stay inside model_dir
```

### 6.6 Path policies and call overrides

```cpp
using namespace pluss::loader;

set_root_path("/abs/path/to/proj");
set_path_policy({
  .canonicalize_root = true,
  .auto_create_dirs = true,
  .force_replace_dirs = false
});

auto logs = file_dir("logs"); // inherits global auto_create_dirs=true
auto one_off = file_dir("scratch", {.create_dirs = false}); // explicit per-call override
```

Expected behavior:

```text
global policy sets defaults
PathOptions can override defaults for a single call
```

### 6.7 Absolute escape hatch (`absolute_path`)

```cpp
using namespace pluss::loader;

set_root_path("/abs/path/to/proj");
auto tool = absolute_path("/usr/local/bin/tool.exe");
```

Expected behavior:

```text
absolute_path does not prepend root
it is explicit opt-in and requires rooted input
```

### 6.8 Multi-root local workflows (`PathContext`)

```cpp
using namespace pluss::loader;

auto proj = PathContext::from_root("/abs/path/to/project_a");
auto ext  = PathContext::from_root("/mnt/shared_data");

auto train_csv = proj.file_path("all_data/train.csv");
auto vendor    = ext.file_path("vendor/source2.csv");
auto report    = proj.file_dir("reports", {.create_dirs = true}).file("summary.txt");
```

Expected behavior:

```text
each context has independent root and policy state
no mutation of global set_root_path state
```

### 6.9 Internet-root workflows (`UrlContext`)

`UrlContext` is the remote equivalent of `PathContext`, but for HTTP/HTTPS roots.

```cpp
using namespace pluss::loader;

auto remote = UrlContext::from_root(
  "https://raw.githubusercontent.com/DanikaLipman/Stat217/refs/heads/main/"
);

auto csv_url = remote.file_path("example5.csv");
auto xls_url = remote.file_path("unit1/example.xlsx");

auto df1 = load_csv(csv_url.string());
auto df2 = load_excel(xls_url.string());
```

Expected behavior:

```text
root trailing slash is normalized
subpaths are joined safely under the URL root
loaders receive plain string URLs
```

Batch URL addressing:

```cpp
auto urls = remote.file_paths({
  "example5.csv",
  "unit1/example6.csv"
});

auto named_urls = remote.file_path_map({
  {"housing", "example5.csv"},
  {"other", "unit1/example6.csv"}
});
```

### 6.10 Hybrid local + remote recipe

```cpp
using namespace pluss::loader;

set_root_path("/abs/path/to/proj");
auto out_dir = file_dir("outputs/session_01", {.create_dirs = true});

auto gh = UrlContext::from_root(
  "https://raw.githubusercontent.com/DanikaLipman/Stat217/refs/heads/main"
);
auto remote_csv = gh.file_path("example5.csv");

auto df = load_csv(remote_csv.string());
save_csv(df, out_dir.file("example5_copy.csv").string());
```

Expected behavior:

```text
remote read via UrlContext
local write via file_dir/file_path
single coherent pathing style across both sources
```

### 6.11 Safety and diagnostics

- Root escape prevention:
  - local helpers reject paths that attempt traversal outside root.
  - `UrlContext` rejects `..` and full URL subpath injection.
- Rooted subpath behavior:
  - local `file_path("/x.csv")` is rewritten as root-relative (`<root>/x.csv`) with warning.
- String handoff:
  - loaders accept string paths/URLs, so pass `.string()` from `PathRef`/`UrlRef`.

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
out.writeln("features", df.features());

pluss::loader::save_json(df, "outputs/bank_subset.json");
out.writeln("saved", "outputs/bank_subset.json");
```

Expected output (example):

```text
shape 1000 3
features [age, job, balance]
saved outputs/bank_subset.json
```
