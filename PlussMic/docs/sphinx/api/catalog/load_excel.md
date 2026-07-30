<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4716811d4a04 -->

# Catalog::load_excel

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
static Catalog Catalog::load_excel(std::string_view filepath)
static Catalog Catalog::load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options)
```

```cpp
struct ReadExcelOptions {
  bool header{true};
  std::int64_t header_row{0};
  std::size_t header_rows{1};
  bool header_ffill{true};
  std::string header_joiner{"_"};
  std::optional<std::string> sheet_name{};
  std::optional<std::int64_t> sheet_index{};
  std::vector<std::string> sheet_names{};
  std::vector<std::int64_t> sheet_indices{};
  std::optional<std::size_t> n_sheets{};
};
```

## What It Does

Loads an `.xlsx` workbook into a new `Catalog`, one stream per column, following the same
per-column dtype-inference rules as [`load_csv()`](load_csv.md). `sheet_name`/`sheet_index`
select a single sheet by name or position; if neither is set, the first sheet is used.
`header_rows > 1` merges a multi-row header into one column name per column, joined by
`header_joiner`.

```{important} Excel support is an optional build feature (requires libzip)
`load_excel()` (and `save_excel()`) require the library to have been built with `libzip`
available. In a build without it, both throw
`"...: Excel support not built (libzip not available)"` on any call, regardless of the
file path or options passed — this is a build-configuration issue, not something fixable
by changing your call. If you hit this, rebuild PlussMic with `libzip` available.
```

## Parameters

**filepath** (`std::string_view`)
    The `.xlsx` file to read.

**options** (`const pluss::loader::ReadExcelOptions&`, optional)
    Sheet selection and header-parsing controls — see the struct above.

## Returns

`Catalog`
    A new catalog with one stream per column of the selected sheet.

## Examples

Example 1. Load the first (or only-named) sheet with default options — the same call
shape as [`load_csv()`](load_csv.md), just for `.xlsx` files.

```cpp
auto ct = pluss::kindle::Catalog::load_excel("cities.xlsx");
std::cout << ct.nkeys();
```

Example 2. Select a specific sheet by name.

```cpp
pluss::loader::ReadExcelOptions options;
options.sheet_name = "Cities";

auto ct = pluss::kindle::Catalog::load_excel("cities.xlsx", options);
std::cout << ct.nkeys();
```

Example 3. Attempting to load an `.xlsx` file in a build without Excel support throws —
this is the exact message this documentation environment produces, since it was built
without `libzip`.

```cpp
try {
  auto ct = pluss::kindle::Catalog::load_excel("cities.xlsx");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
read_excel_raw_batch: Excel support not built (libzip not available)
```

## Notes

- Requires `libzip` at build time — see the important callout above. Examples 1 and 2 show
  the intended call shapes; only Example 3's throw could actually be verified in this
  documentation environment.
- Without an explicit `sheet_name`/`sheet_index`, the first sheet is used.
- Column dtype inference follows the same rules as [`load_csv()`](load_csv.md).

## See Also

- [`Catalog::load_csv`](load_csv.md): the equivalent CSV loader, with no optional build
  dependency.
- [`Catalog::save_excel`](save_excel.md): the write-side counterpart, with the same
  `libzip` requirement.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_csv_with_key.html">
    <span class="label">Previous</span>
    <span class="title">load_csv_with_key</span>
  </a>
  <a class="next-link" href="load_json.html">
    <span class="label">Next</span>
    <span class="title">load_json</span>
  </a>
</div>
