<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9b51664f787a -->

# Registry::load_excel

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
static Registry Registry::load_excel(std::string_view filepath)
static Registry Registry::load_excel(std::string_view filepath, const pluss::loader::Config& config)
static Registry Registry::load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options)
```

## What It Does

**A `static` factory method** — loads an `.xlsx` workbook into a new `Registry`, one
field (slot) per column, following the same per-column dtype-inference rules as
[`load_csv()`](load_csv.md).

```{important} Excel support is an optional build feature (requires libzip)
`load_excel()` (and `save_excel()`) require the library to have been built with
`libzip` available. In a build without it, both throw
`"...: Excel support not built (libzip not available)"` on any call, regardless of
the file path or options passed — this is a build-configuration issue, not something
fixable by changing your call. If you hit this, rebuild PlussMic with `libzip`
available.
```

## Parameters

**filepath** (`std::string_view`)
    The `.xlsx` file to read.

**config** (`const pluss::loader::Config&`, optional)
    A generic configuration scribble, translated into `ReadExcelOptions` internally.

**options** (`const pluss::loader::ReadExcelOptions&`, optional)
    Sheet selection and header-parsing controls, same struct as
    [`Catalog::load_excel`](../catalog/load_excel.md)'s.

## Returns

`Registry`
    A new registry with one field per column of the selected sheet.

## Examples

Example 1. Attempting to load an `.xlsx` file in a build without Excel support
throws — this is the exact message this documentation environment produces, since it
was built without `libzip`.

```cpp
try {
  auto rg = pluss::scribble::Registry::load_excel("cities.xlsx");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
read_excel_raw_batch: Excel support not built (libzip not available)
```

## Notes

- Requires `libzip` at build time — see the important callout above. Only the throw
  example above could actually be verified in this documentation environment; the
  intended call shape otherwise matches [`load_csv()`](load_csv.md).
- Without an explicit `sheet_name`/`sheet_index`, the first sheet is used.
- Column dtype inference follows the same rules as [`load_csv()`](load_csv.md).

## See Also

- [`Registry::load_csv`](load_csv.md): the equivalent CSV loader, with no optional
  build dependency.
- [`Catalog::load_excel`](../catalog/load_excel.md): the equivalent loader on the
  kindle model, with the same `libzip` requirement.
- [`Registry::save_excel`](save_excel.md): the write-side counterpart.

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
