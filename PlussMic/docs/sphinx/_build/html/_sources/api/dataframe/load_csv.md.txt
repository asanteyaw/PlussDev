<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 74584aa20988 -->

# DataFrame::load_csv

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
static DataFrame load_csv(std::string_view filepath) -> DataFrame

static DataFrame load_csv(std::string_view filepath,
                           char sep,
                           std::optional<std::size_t> ncases = std::nullopt,
                           std::vector<std::string> usefeats = {},
                           std::optional<std::string> case_col = std::nullopt,
                           pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error) -> DataFrame

static DataFrame load_csv(std::string_view filepath,
                           char sep,
                           std::optional<std::size_t> ncases,
                           pluss::RangeView<std::string_view> usefeats,
                           std::optional<std::string> case_col = std::nullopt,
                           pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error) -> DataFrame

static DataFrame load_csv(std::string_view filepath, const pluss::loader::Config& config) -> DataFrame

static DataFrame load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options) -> DataFrame

static DataFrame load_csv(std::string_view filepath,
                           std::initializer_list<pluss::loader::ConfigEntry> config_entries) -> DataFrame

static DataFrame load_csv(std::string_view filepath, std::string_view header, std::size_t ncases) -> DataFrame

static DataFrame load_csv(std::string_view filepath,
                           std::string_view header,
                           std::string_view case_feature = "none",
                           std::string_view engine = "auto",
                           std::optional<std::size_t> ncases = std::nullopt) -> DataFrame
```

## What It Does

`load_csv()` is `DataFrame`'s static constructor for delimited text files. It parses a
CSV (or any single-character-delimited text file) and returns a fully typed `DataFrame`:
numeric columns are parsed into `Series` with an inferred scalar type, text columns
become `Note`, and empty or malformed cells become missing values rather than empty
strings.

The overload set trades off convenience against control:

- `load_csv(filepath)` alone is the "just read it" overload — comma-separated, header
  row present, every column and row loaded, malformed rows treated as an error.
- The positional overload (`sep`, `ncases`, `usefeats`, `case_col`, `on_bad_lines`) is
  for one-off loads where a handful of settings need to change but a whole config object
  would be overkill.
- The `Config` / `ReadCsvOptions` overloads are for reusable, named option scribbles —
  build the options once and pass the same object to several `load_csv` calls, or store
  it alongside a data pipeline definition.
- The `header` overloads are for headerless files or files where the source column that
  should become the case axis is known up front.

All overloads perform the same parse-and-type-infer work under the hood; picking an
overload is about how you want to *express* the loader settings, not about different
parsing behavior.

## Parameters

**filepath** (`std::string_view`)

Path to the delimited text file to read.

**sep** (`char`)

Field delimiter. `','` for standard CSV; pass `'\t'` for TSV, `'|'` for pipe-delimited
files, and so on.

**ncases** (`std::optional<std::size_t>`, default unset)

Maximum number of rows to read, useful for peeking at a large file without loading it in
full. Leave unset to read every row.

**usefeats** (`std::vector<std::string>` / `pluss::RangeView<std::string_view>`, default
empty)

Restricts the load to a subset of columns by name, matched against the file's header
row. Leave empty to load every column present in the file.

**case_col** (`std::optional<std::string>`, default unset)

Names a source column that should become the frame's case axis instead of an
auto-generated integer index. The column is consumed from the regular feature set and
becomes the case label thread.

**on_bad_lines** (`pluss::loader::BadLinePolicy`, default `Error`)

What to do when a row does not parse cleanly (wrong field count, for example):

- `Error`: stop loading and raise immediately.
- `Warn`: emit a warning and continue, skipping the malformed row.
- `Skip`: silently skip the malformed row.

**config** (`const pluss::loader::Config&`) / **config_entries**
(`std::initializer_list<pluss::loader::ConfigEntry>`)

A reusable or inline key/value settings scribble, for callers who prefer a single options
object over a long positional argument list.

**options** (`const pluss::loader::ReadCsvOptions&`)

A typed options struct covering the same settings as the positional overload
(`sep`, `ncases`, `case_col`, `usefeats`, `on_bad_lines`, and more), useful when the same
parsing configuration is reused across several files.

**header** (`std::string_view`)

CSV header mode: `"infer"` reads the first row as feature names; `"none"` treats the
file as headerless and auto-generates feature names (`feat_0`, `feat_1`, ...).

**case_feature** (`std::string_view`, default `"none"`)

Name given to the restored case-axis column, for the `header`-based overload.

**engine** (`std::string_view`, default `"auto"`)

Backend selector for the parser. `"auto"` lets PlussMic choose the fastest available
engine for the file; pass an explicit engine name only when you need predictable,
non-auto-detected routing.

## Returns

`DataFrame`
    A new, fully typed frame built from the file's contents. Numeric columns are typed
    as `Series`; text columns are typed as `Note`; malformed or empty cells become
    missing values rather than empty strings or `"nan"` text.

## Examples

Example 1. Load a plain, comma-separated CSV with a header row.

Given `orders.csv`:

```text
order_id,city,product,units,unit_price
101,Accra,Laptop,2,1200
102,Accra,Headphones,5,45.5
103,Tema,Laptop,1,1150
104,Tema,Monitor,3,320
```

```cpp
using namespace pluss::table;

auto orders = DataFrame::load_csv("orders.csv");
std::cout << orders;
```

### Output

```text
=====================================================
     | order_id  city   product     units  unit_price
-----------------------------------------------------
0    | 101       Accra  Laptop      2      1200      
1    | 102       Accra  Headphones  5      45.5      
2    | 103       Tema   Laptop      1      1150      
3    | 104       Tema   Monitor     3      320       
-----------------------------------------------------
[4 cs x 5 feat]
```

`order_id` and `units` are parsed as integer `Series`, `unit_price` as a floating-point
`Series`, and `city` / `product` as `Note` — all inferred purely from the file's
contents, with no schema declared up front.

Example 2. Load only a subset of columns and cap the number of rows.

```cpp
auto preview = DataFrame::load_csv(
    "orders.csv", ',', std::size_t{2}, std::vector<std::string>{"city", "unit_price"});
```

`preview` has only the `city` and `unit_price` features, and stops after the first `2`
data rows, without reading (or even parsing) the rest of the file — useful for a quick
peek at a source file that may be much larger than you want to load in full.

Example 3. Use `case_col` to promote an existing column to the case axis.

```cpp
auto by_order_id = DataFrame::load_csv(
    "orders.csv", ',', std::nullopt, std::vector<std::string>{}, std::string{"order_id"});
```

`order_id` is consumed from the ordinary feature set and becomes the frame's case label,
so the resulting frame has four features (`city`, `product`, `units`, `unit_price`) with
case labels `101, 102, 103, 104` instead of a plain `0..3` index.

## Notes

- Type inference happens per column, from the observed values in that column only —
  `load_csv` does not accept an explicit dtype schema. If a numeric-looking column
  contains even one non-numeric value, expect it to be inferred as `Note` rather than
  `Series`; clean the source data or post-process with [`astype()`](astype.md) /
  [`cast()`](cast.md) if that happens unintentionally.
- Empty cells become missing values (visible as `NULL` when the frame is printed and
  reflected in `info()`'s non-null counts), not the literal string `""` or `"nan"`.
- Excel loading (`load_excel()`) can return several frames at once from a multi-sheet
  workbook, which is why its return type differs (`pluss::loader::LoadResult`) from the
  single-frame `load_csv()`.

## See Also

- [`DataFrame::save_csv`](save_csv.md): the write-side counterpart.
- [`DataFrame::load_excel`](load_excel.md) / [`load_json`](load_json.md) /
  [`load_parquet`](load_parquet.md): loaders for other source formats.
- [`DataFrame::info`](info.md): the fastest way to check the dtypes and null counts
  `load_csv` actually inferred.

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
