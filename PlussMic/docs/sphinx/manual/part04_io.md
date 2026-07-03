# Part IV: Loading and Saving Data

PlussMic has two public I/O styles:

- `DataFrame::load_*` and `df.save_*` convenience methods.
- `pluss::loader::*` functions and option structs for lower-level control.

Most users should start with `DataFrame` convenience methods.

## CSV

Minimal CSV load:

```cpp
auto df = pluss::table::DataFrame::load_csv("data.csv");
```

This is the correct first call when the file is a conventional comma-separated
file with a header row. PlussMic reads the header, infers feature types, creates
default numeric case labels, and returns a `DataFrame`.

The loader has two public configuration styles:

- initializer-list config entries, which are compact and convenient;
- typed option structs, which are explicit and better for production code.

Limit cases and select features with config entries:

```cpp
auto df = pluss::table::DataFrame::load_csv("bank-full.csv", {
    {"ncases", 1000},
    {"features", {"age", "job", "balance", "duration"}},
});
```

Use `ReadCsvOptions` for explicit control:

```cpp
pluss::loader::ReadCsvOptions options;
options.sep = ';';
options.ncases = 1000;
options.usefeats = {"age", "job", "balance", "duration"};
options.case_col = "job";
options.on_bad_lines = pluss::loader::BadLinePolicy::Warn;

auto df = pluss::table::DataFrame::load_csv("bank-full.csv", options);
```

Prefer `ReadCsvOptions` when the ingestion behavior matters. It is easier to
read in code review, safer to extend, and maps directly to the source-level CSV
loader API.

## CSV mental model

CSV loading has five stages:

1. Read records from the source.
2. Decide the header and feature names.
3. Decide which input columns become features.
4. Decide whether one input column becomes the case axis.
5. Convert selected columns into typed `Thread` values.

Most surprising loading behavior comes from mixing up stages 3 and 4. If a
column is selected as `case_col`, it becomes the DataFrame case axis and is not
also emitted as a normal feature.

For example:

```cpp
pluss::loader::ReadCsvOptions options;
options.case_col = "id";
options.usefeats = {"id", "city", "units"};
```

The `id` column is still used, but it is used as the case axis. The output
features are `city` and `units`.

## Header handling

For ordinary files:

```cpp
options.header = true;
options.header_row = 0;
```

For files without a header:

```cpp
options.header = false;
options.names = {"id", "city", "units"};
```

When `header` is false and you do not provide names, PlussMic generates feature
names from column positions. For durable code, provide `names` explicitly.

Use `header_row` when the actual header is not the first physical row:

```cpp
options.header = true;
options.header_row = 2;
```

Rows before the header row are skipped from data interpretation.

## Feature projection

Use names when the file has stable headers:

```cpp
options.usefeats = {"city", "units", "price"};
```

Use indices when working with headerless files or unstable external labels:

```cpp
options.usefeats_idx = {0, 2, 4};
```

If both `usefeats` and `usefeats_idx` are empty, all non-case columns are
loaded. If a requested name is missing, loading fails. If a requested index is
out of range, loading fails.

## Case columns

The case axis is PlussMic's row-label axis. It is similar to an index in other
dataframe libraries, but the public method names use `cases`.

Use a named case column:

```cpp
options.case_col = "id";
```

Use a positional case column:

```cpp
options.case_col_idx = 0;
```

After loading, inspect the case axis:

```cpp
std::cout << df.cases().dtype_name() << '\n';
```

Move a case axis back into the feature axis later with:

```cpp
auto with_id_feature = df.reset_cases("id");
```

### Verified example: semicolon CSV with case column

Assume the program and this `sales.csv` file are in the same directory:

```text
id;city;region;units;price
A001;Accra;south;10;3.0
A002;Tema;south;7;4.0
A003;Kumasi;middle;12;2.5
A004;Tamale;north;5;5.0
```

The example loads only selected features, uses `id` as the case axis, and
limits loading to three cases:

```cpp
pluss::loader::ReadCsvOptions options;
options.sep = ';';
options.case_col = "id";
options.usefeats = {"city", "units", "price"};
options.ncases = 3;

auto df = DataFrame::load_csv("sales.csv", options);
std::cout << df.to_string() << '\n';
std::cout << "case dtype=" << df.cases().dtype_name() << '\n';
```

Output:

```text
===========================
     | city    units  price
---------------------------
A001 | Accra   10     3
A002 | Tema    7      4
A003 | Kumasi  12     2.5
---------------------------
[3 cs x 3 feat]
case dtype=utf8
```

The `id` column does not appear as a feature because it became the case axis.
The printed case labels are `A001`, `A002`, and `A003`, and the case dtype is
`utf8`. The fourth input row is absent because `options.ncases = 3`.

### Verified example: load records, infer dtypes, save CSV

This example uses `load_csv_records` instead of a file. It is useful for tests,
examples, generated rows, or adapters that already have records in memory.

```cpp
const std::vector<std::vector<std::string>> records = {
    {"id", "city", "units", "active"},
    {"A001", "Accra", "10", "yes"},
    {"A002", "Tema", "", "no"},
    {"A003", "Kumasi", "12", "yes"},
};

pluss::loader::ReadCsvOptions read_options;
read_options.case_col = "id";
auto df = pluss::loader::load_csv_records(records, read_options);

std::cout << df.to_string() << '\n';
std::cout << "city dtype=" << df.features("city").dtype_name() << '\n';
std::cout << "units dtype=" << df.features("units").dtype_name() << '\n';
std::cout << "active dtype=" << df.features("active").dtype_name() << '\n';

pluss::loader::WriteCsvOptions write_options;
write_options.sep = ';';
write_options.add_cases = true;
write_options.case_label = "id";
write_options.na_rep = "";

df.save_csv("roundtrip.csv", write_options);

std::ifstream in("roundtrip.csv");
std::string line;
while (std::getline(in, line)) {
  std::cout << line << '\n';
}
```

Output:

```text
============================
     | city    units  active
----------------------------
A001 | Accra   10     true
A002 | Tema    NA     false
A003 | Kumasi  12     true
----------------------------
[3 cs x 3 feat]
city dtype=utf8
units dtype=int64
active dtype=bool
id;city;units;active
A001;Accra;10;true
A002;Tema;;false
A003;Kumasi;12;true
```

The loader inferred `city` as UTF-8 text, `units` as `int64`, and `active` as
boolean because the default true/false sets include `yes` and `no`. The empty
`units` cell in row `A002` becomes `NA` in the DataFrame. When saving, the
example sets `na_rep = ""`, so that same missing value is written as an empty
field between two semicolons.

`write_options.add_cases = true` writes the case axis as the first output
column. `write_options.case_label = "id"` names that output column `id`.

Important CSV options:

| Option | Purpose |
| --- | --- |
| `sep` | explicit separator |
| `auto_detect_sep` | infer separator when `sep` is not explicit |
| `engine` | `Auto`, `Parallel`, or `Serial` |
| `parallel_threads` | requested parallel parse threads |
| `profile` | emit parser timing diagnostics |
| `header`, `header_row`, `names` | header handling |
| `usefeats`, `usefeats_idx` | select input features |
| `case_col`, `case_col_idx` | choose case/index feature |
| `skipcases`, `skipcases_first_n`, `skipfooter` | skip rows |
| `ncases` | limit number of parsed cases |
| `parse_dates`, `parse_dates_idx` | datetime parsing |
| `na_values`, `keep_default_na`, `na_filter` | missing-value handling |
| `true_values`, `false_values` | boolean parsing |
| `thousands`, `decimal` | numeric locale hints |
| `on_bad_lines` | error, warn, or skip malformed rows |

## Type inference and explicit dtypes

CSV data starts as text. PlussMic converts each selected column into a typed
Thread. Common inferred outputs are:

- integer Series for integral values;
- floating Series for decimal values;
- boolean Series for configured true/false tokens;
- Note for general text;
- Timeline or TimeIndex when date parsing is requested.

The default boolean token sets include values such as `true`, `false`, `1`,
`0`, `yes`, and `no`.

Use `dtype` when a column must have a specific interpretation:

```cpp
options.dtype["customer_id"] = "string";
options.dtype["units"] = "int64";
options.dtype["price"] = "float64";
```

Use `parse_dates` when text columns should become time payloads:

```cpp
options.parse_dates = {"created_at", "updated_at"};
options.infer_datetime_format = true;
options.dayfirst = false;
```

Use explicit dtypes for identifiers that look numeric. Postal codes, account
numbers, and product IDs should usually be loaded as text, not numbers.

## Missing values

CSV missing-value handling is controlled by:

```cpp
options.na_filter = true;
options.keep_default_na = true;
options.na_values.insert("missing");
options.na_values.insert("N/A");
```

`na_filter = false` disables missing-value detection. `keep_default_na = true`
keeps the built-in missing tokens. `na_values` adds project-specific tokens.

Missingness becomes a Thread validity mask. That means a numeric feature can
still have an integer or floating dtype even when some rows display as `NA`.

## Bad lines

Malformed rows are rows whose field count does not match the expected layout.
The policy is:

```cpp
options.on_bad_lines = pluss::loader::BadLinePolicy::Error;
options.on_bad_lines = pluss::loader::BadLinePolicy::Warn;
options.on_bad_lines = pluss::loader::BadLinePolicy::Skip;
```

Use `Error` for production ingestion when malformed data should stop the job.
Use `Warn` when you want diagnostics and can tolerate dropped rows. Use `Skip`
only when silent row loss is acceptable in that context.

## Engine selection

The CSV engine can be selected explicitly:

```cpp
options.engine = pluss::loader::CsvEngine::Auto;
options.engine = pluss::loader::CsvEngine::Parallel;
options.engine = pluss::loader::CsvEngine::Serial;
```

`Auto` is the right default for most users. Use `Parallel` for large local
files when throughput matters. Use `Serial` when debugging parser behavior or
when deterministic step-by-step ingestion is more important than speed.

Related options:

```cpp
options.parallel_threads = 8;
options.parallel_min_bytes = 4 * 1024 * 1024;
options.profile = true;
```

`profile = true` emits parser timing diagnostics through the diagnostics path.

## TXT

TXT uses the same table loader surface as CSV:

```cpp
auto df = pluss::table::DataFrame::load_txt("space_separated.txt", {
    {"sep", " "},
    {"ncases", 1000},
});
```

For strongly typed control, use CSV options with an explicit separator through
the lower-level loader path.

## Excel

Load the default sheet:

```cpp
auto df = pluss::table::DataFrame::load_excel("report.xlsx");
```

Load by sheet name or index:

```cpp
auto prices = pluss::table::DataFrame::load_excel("report.xlsx", "Prices");
auto first = pluss::table::DataFrame::load_excel("report.xlsx", 0);
```

Load a workbook:

```cpp
auto loaded = pluss::table::DataFrame::load_excel("report.xlsx", nullptr, true);
for (const auto& name : loaded.sheet_names().values()) {
  loaded.sheets(name).head();
}
```

Use `ReadExcelOptions` for advanced reading:

```cpp
pluss::loader::ReadExcelOptions options;
options.sheet_name = "Prices";
options.header = true;
options.start_row = 2;
options.end_row = 200;
options.start_col = 0;
options.end_col = 8;

auto prices = pluss::table::DataFrame::load_excel("report.xlsx", options);
```

Important Excel options:

| Option | Purpose |
| --- | --- |
| `sheet_name`, `sheet_index` | single sheet selection |
| `sheet_names`, `sheet_indices`, `all_sheets` | workbook/multi-sheet selection |
| `header`, `header_row`, `header_rows` | header interpretation |
| `header_ffill`, `header_joiner` | multi-row header flattening |
| `start_row`, `end_row`, `start_col`, `end_col` | bounded region reads |
| `trim_outer_empty` | remove empty outer rows/columns |
| `auto_table_block` | infer a table block |

## JSON and Parquet

```cpp
auto weather = pluss::table::DataFrame::load_json("weather.json");
auto flights = pluss::table::DataFrame::load_parquet("flights.parquet");
```

For dynamic format selection:

```cpp
auto loaded = pluss::table::DataFrame::load("csv", "data.csv");
auto parquet = pluss::table::DataFrame::load("parquet", "flights.parquet");
```

## Raw batches

Use raw or neutral batch APIs when you need to inspect parsed input before it
becomes a `DataFrame`:

```cpp
auto raw = pluss::loader::read_csv_raw_batch("data.csv", {{"ncases", 100}});
auto batch = pluss::loader::read_csv_batch("data.csv", {{"ncases", 100}});
```

Raw batches keep strings/notes close to file contents. Neutral batches carry
`Thread` values but do not own a frame.

## `CsvCaseReader::next`

### API Signature

```text
CsvCaseReader::next(output: DataFrame&) -> bool
```

### Notes

Reads the next bounded case batch. The first emitted batch establishes the
inferred feature kinds and numeric dtypes. A later inferred schema change fails
if conversion to that schema would discard an observed value; compatible batches,
including all-null batches, are converted to the established schema. Supply
`ReadCsvOptions::dtype` when a source column contains mixed lexical forms.

### Inputs

`output: DataFrame&`
    Destination replaced with the next batch when the method returns `true`.

### Output

Returns `true` when a batch was emitted and `false` after the source is exhausted.

### Options

- Constructor `case_batch_size` controls the maximum cases per batch.
- `ReadCsvOptions::dtype` fixes feature types across every batch.
- `ReadCsvOptions::ncases` limits the total cases emitted.
- `skipcases` and `skipfooter` are rejected because they cannot be honored
  predictably by this streaming contract.

### Examples

Example 1: read every batch.

```cpp
pluss::loader::CsvCaseReader reader("data.csv", 4096);
pluss::table::DataFrame batch;
while (reader.next(batch)) {
    batch.show();
}
```

#### Expected output

```text
Each successful iteration contains at most 4096 cases; the loop ends at EOF.
```

Example 2: enforce a text schema.

```cpp
pluss::loader::ReadCsvOptions options;
options.dtype["account"] = "string";
pluss::loader::CsvCaseReader reader("data.csv", 2048, options);
```

#### Expected output

```text
account is Note-backed in every emitted batch.
```

Example 3: enforce a numeric schema.

```cpp
pluss::loader::ReadCsvOptions options;
options.dtype["amount"] = "float64";
pluss::loader::CsvCaseReader reader("data.csv", 2048, options);
```

#### Expected output

```text
amount is Float64 Series-backed in every emitted batch.
```

Example 4: stop after a global case limit.

```cpp
pluss::loader::ReadCsvOptions options;
options.ncases = 10'000;
pluss::loader::CsvCaseReader reader("data.csv", 1024, options);
```

#### Expected output

```text
reader.cases_read() never exceeds 10000.
```

Example 5: detect unsafe inference drift.

```cpp
// value contains 1, 2, then "three" in a later batch.
pluss::loader::CsvCaseReader reader("mixed.csv", 2);
pluss::table::DataFrame batch;
reader.next(batch);
reader.next(batch);
```

#### Expected output

```text
The second call fails and asks for ReadCsvOptions::dtype.
```

## URL inputs

When the SDK has URL loader support, `load_csv`, `load_txt`, and `load_excel`
can accept `http://` and `https://` sources. Treat remote reads as I/O
operations that can fail, timeout, or produce changing data.

## Save outputs

```cpp
df.save_csv("clean.csv");
df.save_excel("clean.xlsx", "Clean");
df.save_json("clean.json");
df.save_parquet("clean.parquet");
df.save_txt("clean.txt");
```

Dynamic save:

```cpp
df.save_as("csv", "clean.csv");
df.save_as("parquet", "clean.parquet");
```

CSV save options:

```cpp
pluss::loader::WriteCsvOptions options;
options.add_cases = true;
options.case_label = "id";
options.na_rep = "";
options.float_format = "{:.6f}";

df.save_csv("clean.csv", options);
```

Important CSV write options:

| Option | Purpose |
| --- | --- |
| `sep` | output delimiter |
| `header` | write feature names |
| `add_cases` | write the case axis as the first column |
| `case_label` | output name for the case column |
| `na_rep` | text written for missing values |
| `float_format` | optional floating-point format |
| `quotechar` | quote character |
| `doublequote` | escape quotes by doubling |
| `line_terminator` | row terminator |
| `decimal` | decimal character |
| `encoding` | output encoding label |

When creating files for another system, set `sep`, `add_cases`, `case_label`,
and `na_rep` deliberately. Defaults are convenient for exploration, but
production exports should make these choices visible.

Excel save options:

```cpp
pluss::loader::WriteExcelOptions options;
options.sheet_name = "Clean";
options.add_cases = true;
options.na_rep = "";

df.save_excel("clean.xlsx", options);
```

## I/O checklist

- Use typed options for production ingestion.
- Set separators explicitly for non-comma files.
- Set `case_col` deliberately when a dataset has an ID column.
- Limit with `ncases` while developing.
- Use `on_bad_lines = Warn` or `Skip` only when dropping malformed rows is acceptable.
- Save cleaned outputs with explicit format-specific filenames.
