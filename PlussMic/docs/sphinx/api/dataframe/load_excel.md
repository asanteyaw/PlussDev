<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1b13f6de74c4 -->

# DataFrame::load_excel

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
static DataFrame load_excel(std::string_view filepath) -> DataFrame
static DataFrame load_excel(std::string_view filepath, const pluss::loader::Config& config) -> DataFrame
static DataFrame load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options) -> DataFrame
static DataFrame load_excel(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) -> DataFrame
static DataFrame load_excel(std::string_view filepath, std::string_view sheet_name, bool header = true) -> DataFrame
static DataFrame load_excel(std::string_view filepath, std::int64_t sheet_index, bool header = true) -> DataFrame
static pluss::loader::LoadResult load_excel(std::string_view filepath, std::nullptr_t, bool header = true) -> pluss::loader::LoadResult
static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::string>& sheet_names, bool header = true) -> pluss::loader::LoadResult
static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::string_view> sheet_names, bool header = true) -> pluss::loader::LoadResult
static pluss::loader::LoadResult load_excel(std::string_view filepath, pluss::RangeView<std::string_view> sheet_names, bool header = true) -> pluss::loader::LoadResult
static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::int64_t>& sheet_indices, bool header = true) -> pluss::loader::LoadResult
static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::int64_t> sheet_indices, bool header = true) -> pluss::loader::LoadResult
static pluss::loader::LoadResult load_excel(std::string_view filepath, pluss::RangeView<std::int64_t> sheet_indices, bool header = true) -> pluss::loader::LoadResult
```

## What It Does

`DataFrame`'s static constructor for Excel workbooks — the one loader in this family
with genuinely different *return types* depending on the overload, because a workbook
can hold more than one sheet:

- **Single sheet** (no sheet argument, or `sheet_name`/`sheet_index` named explicitly)
  — returns a plain `DataFrame`, exactly like every other loader.
- **Several sheets at once** (`nullptr` for "all sheets", or an explicit list of sheet
  names/indices) — returns a `pluss::loader::LoadResult`, a tagged scribble holding
  either a single frame or a whole `Workbook` (`std::vector<std::pair<std::string,
  DataFrame>>`) depending on which overload produced it.

## Parameters

**filepath** (`std::string_view`)

Path to the `.xlsx` workbook to read.

**sheet_name** (`std::string_view`, single-sheet overload)

The worksheet to load, by name. Passing the literal string `"none"` here is rejected
with a message redirecting you to the `nullptr` (all-sheets) overload instead — it's
not a valid way to request every sheet.

**sheet_index** (`std::int64_t`, single-sheet overload)

The worksheet to load, by zero-based position.

**header** (`bool`, default `true`)

Whether the sheet's first row holds feature names.

**sheet_names** (`const std::vector<std::string>&` / `std::initializer_list<std::string_view>` /
`pluss::RangeView<std::string_view>`, multi-sheet overload)

Worksheets to load together, by name. An **empty** list is equivalent to passing
`nullptr` — both load every sheet in the workbook.

**sheet_indices** (`const std::vector<std::int64_t>&` / `std::initializer_list<std::int64_t>` /
`pluss::RangeView<std::int64_t>`, multi-sheet overload)

Worksheets to load together, by zero-based position. Same empty-list-means-all-sheets
behavior as `sheet_names`.

**config** (`const pluss::loader::Config&`) / **config_entries**
(`std::initializer_list<pluss::loader::ConfigEntry>`) / **options**
(`const pluss::loader::ReadExcelOptions&`)

Equivalent settings expressed as a reusable config object, inline entries, or a typed
options struct — covering sheet selection, header row handling, and multi-row header
merging, for callers who want to configure everything in one object rather than a
chain of positional parameters.

## Returns

`DataFrame` (single-sheet overloads)
    One fully typed frame for the requested sheet.

`pluss::loader::LoadResult` (multi-sheet overloads)
    A tagged scribble: `.is_frame()`/`.is_workbook()` report which case applies,
    `.dataframe()` unwraps a single-frame result, `.workbook()` returns the full
    `std::vector<std::pair<std::string, DataFrame>>` for a multi-sheet result, and
    `.sheets("Name")` / `.sheet_by_index(i)` reach one sheet directly without
    unwrapping the whole vector yourself.

## Examples

Example 1. Load the default (first) sheet as a plain frame.

```cpp
using namespace pluss::table;

auto orders = DataFrame::load_excel("workbook.xlsx");
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

Example 2. Load one named sheet explicitly.

```cpp
auto regions = DataFrame::load_excel("workbook.xlsx", "Regions");
std::cout << regions;
```

### Output

```text
===================
     | id    region
-------------------
0    | 201   North 
1    | 202   South 
-------------------
[2 cs x 2 feat]
```

Example 3. Load every sheet in the workbook at once.

```cpp
auto all_sheets = DataFrame::load_excel("workbook.xlsx", nullptr);
std::cout << std::boolalpha << all_sheets.is_workbook() << "\n";

for (const auto& [name, frame] : all_sheets.workbook()) {
  std::cout << name << " -> [" << frame.ncases() << " cs x " << frame.nfeatures() << " feat]\n";
}
```

### Output

```text
true
Sheet1 -> [3 cs x 3 feat]
Regions -> [2 cs x 2 feat]
```

`all_sheets.is_workbook()` is `true` because more than one sheet came back — reach one
sheet directly with `all_sheets.sheets("Regions")` instead of iterating the whole
`.workbook()` vector, if you only need one.

Example 4. Load a specific named subset of sheets — an empty list would have the same
effect as Example 3's `nullptr`.

```cpp
auto subset = DataFrame::load_excel("workbook.xlsx", std::vector<std::string>{"Regions"});
std::cout << subset.sheets("Regions");
```

### Output

```text
===================
     | id    region
-------------------
0    | 201   North 
1    | 202   South 
-------------------
[2 cs x 2 feat]
```

## Notes

- Requesting `sheet_name = "none"` on the single-sheet overload throws, redirecting
  you to the `nullptr` overload — `"none"` is not itself a magic value meaning "all
  sheets" for that particular overload.
- An empty `sheet_names`/`sheet_indices` list on the multi-sheet overloads behaves
  identically to the explicit `nullptr` (all-sheets) overload.
- `header` applies uniformly to every sheet loaded in a single call — there is no way
  to set a different header mode per sheet within one multi-sheet call.
- Only `load_excel()` (among all the loaders in this family) can return
  `pluss::loader::LoadResult` — every other loader always returns a plain `DataFrame`.

## See Also

- [`DataFrame::save_excel`](save_excel.md): the write-side counterpart.
- [`DataFrame::load_csv`](load_csv.md): the fuller explanation of the shared
  type-inference engine underlying every loader in this family.
- [`DataFrame::load`](load.md): the format-switched entry point — note it can only
  reach `load_excel()`'s single-frame overload, never the multi-sheet ones.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_csv.html">
    <span class="label">Previous</span>
    <span class="title">load_csv</span>
  </a>
  <a class="next-link" href="load_json.html">
    <span class="label">Next</span>
    <span class="title">load_json</span>
  </a>
</div>
