<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1e91cd1de1cd -->

# DataFrame::save_excel

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::save_excel(std::string_view filepath) const -> void
DataFrame::save_excel(std::string_view filepath, const pluss::loader::Config& config) const -> void
DataFrame::save_excel(std::string_view filepath, const pluss::loader::WriteExcelOptions& options) const -> void
DataFrame::save_excel(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const -> void
DataFrame::save_excel(std::string_view filepath, std::string sheet_name, bool add_cases = false, bool header = true, std::string null_rep = "", std::optional<std::string> case_label = std::nullopt) const -> void
```

## What It Does

Writes the frame to a single worksheet of an `.xlsx` workbook. The sheet name defaults
to `"Sheet1"` unless you supply one (via the positional `sheet_name` parameter, or the
`sheet_name` field of `WriteExcelOptions`/`Config`). As with `save_csv()`, the case axis
is *not* written as its own column unless `add_cases` is turned on.

## Parameters

**filepath** (`std::string_view`)

Destination `.xlsx` path.

**sheet_name** (`std::string`, positional overload, default `"Sheet1"`)

Name of the worksheet to write within the workbook.

**add_cases** (`bool`, default `false`)

When `true`, the case axis is exported as its own leading column, named via
`case_label` (or a default name if left unset) — the same convention
[`save_csv()`](save_csv.md) uses.

**header** (`bool`, default `true`)

Whether to write feature names as the sheet's first row.

**null_rep** (`std::string`, default `""`)

Literal text written for a missing cell. Default is an empty cell.

**case_label** (`std::optional<std::string>`, default unset)

Column name for the exported case-axis column when `add_cases=true`.

**config** (`const pluss::loader::Config&`) / **config_entries**
(`std::initializer_list<pluss::loader::ConfigEntry>`) / **options**
(`const pluss::loader::WriteExcelOptions&`)

Equivalent settings expressed as a reusable config object, inline entries, or the typed
`WriteExcelOptions` struct — recognizing the same `sheet_name`/`add_cases` (also
accepted under the legacy alias `cases`)/`header`/`case_label`/`null_rep` fields as the
positional overload.

## Returns

`void`
    Performs the write and returns nothing.

## Examples

Example 1. Save with the default sheet name (`"Sheet1"`), no case-axis column.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Slate(pluss::make_series({101, 102, 103}))},
    {"city",  Slate(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
    {"units", Slate(pluss::make_series({2, 1, 4}))},
});

orders.save_excel("orders_out.xlsx");
```

Resulting workbook `orders_out.xlsx`, sheet `Sheet1`:

```text
| id  | city   | units |
| 101 | Accra  | 2     |
| 102 | Tema   | 1     |
| 103 | Kumasi | 4     |
```

Example 2. Save to a named sheet and export the case axis as its own column.

```cpp
orders.save_excel("orders_with_case.xlsx", "Orders", /*add_cases=*/true,
                   /*header=*/true, /*null_rep=*/"", /*case_label=*/std::string{"case_id"});
```

Resulting workbook `orders_with_case.xlsx`, sheet `Orders`:

```text
| case_id | id  | city   | units |
| 0       | 101 | Accra  | 2     |
| 1       | 102 | Tema   | 1     |
| 2       | 103 | Kumasi | 4     |
```

Loading this back with
`DataFrame::load_excel("orders_with_case.xlsx", "Orders")` followed by treating
`case_id` as the case column (via a `Config`/`ReadExcelOptions` case-column setting)
restores the original case labels, the same way `save_csv()`/`load_csv()`'s
`case_label`/`case_col` pair does.

Example 3. Save with a typed `WriteExcelOptions` struct.

```cpp
pluss::loader::WriteExcelOptions options;
options.sheet_name = "Regions";
options.add_cases = true;

orders.save_excel("orders_options.xlsx", options);
```

Resulting workbook `orders_options.xlsx`, sheet `Regions`:

```text
| case | id  | city   | units |
| 0    | 101 | Accra  | 2     |
| 1    | 102 | Tema   | 1     |
| 2    | 103 | Kumasi | 4     |
```

Since `case_label` wasn't set here, the exported case column falls back to its default
name (`"case"`).

## Notes

- Only one sheet is written per call — there's no multi-sheet write overload the way
  [`load_excel()`](load_excel.md) has multi-sheet *read* overloads. Writing several
  sheets into one workbook means saving to the same `filepath` multiple times with
  different `sheet_name`s and workbook-append handling managed at the file-format
  level, not through a single `DataFrame::save_excel()` call.
- `add_cases`/`case_label` mirror [`save_csv()`](save_csv.md)'s convention exactly —
  the case axis is opt-in, not written by default.
- `WriteExcelOptions` accepts `add_cases` under the legacy field name `cases` as well,
  for compatibility with older call sites.

## See Also

- [`DataFrame::load_excel`](load_excel.md): the read-side counterpart, including the
  multi-sheet `LoadResult` return type this writer has no equivalent of.
- [`DataFrame::save_csv`](save_csv.md): the analogous CSV writer and the fuller
  explanation of the `add_cases`/`case_label` convention shared across all writers.
- [`DataFrame::save_as`](save_as.md): the format-switched entry point that can
  dispatch to `save_excel()` via `target_type = "excel"`/`"xlsx"`.

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
