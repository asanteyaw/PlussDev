<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fbe2eae18ee2 -->

# Catalog::load

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
static Catalog Catalog::load(std::string_view source_type, std::string_view filepath)
static Catalog Catalog::load(std::string_view source_type, std::string_view filepath, const pluss::loader::Config& config)
```

## What It Does

**A `static` factory method** that dispatches by name to one of the format-specific
loaders — `"csv"` to [`load_csv()`](load_csv.md), `"txt"`/`"text"` to
[`load_txt()`](load_txt.md), `"excel"`/`"xlsx"` to [`load_excel()`](load_excel.md),
`"parquet"` to [`load_parquet()`](load_parquet.md), `"json"` to
[`load_json()`](load_json.md). Matching is case-insensitive. Use `load()` when the format
is only known at runtime (e.g. from a user-supplied string); reach for the specific
`load_*()` method directly otherwise.

## Parameters

**source_type** (`std::string_view`)
    The format name — `"csv"`, `"txt"`/`"text"`, `"excel"`/`"xlsx"`, `"parquet"`, or
    `"json"` (case-insensitive). Anything else throws.

**filepath** (`std::string_view`)
    The file to read.

**config** (`const pluss::loader::Config&`, optional)
    A generic, format-agnostic configuration scribble, translated internally into the
    format-specific options struct (`ReadCsvOptions`, `ReadExcelOptions`, etc.).

## Returns

`Catalog`
    A new catalog with one stream per detected column.

## Examples

Example 1. Load a CSV file by naming the format explicitly.

```cpp
auto ct = pluss::kindle::Catalog::load("csv", "cities.csv");
std::cout << ct;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=3
keys: [id, city, population]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[int64]    | 3       | [1, 2, 3]                                        
city                     | Note         | Note             | 3       | [Accra, Tema, Kumasi]                            
population               | Series       | Series[int64]    | 3       | [2388000, 300000, 3348000]                       
========================================================================================================================
```

Example 2. An unrecognized format name throws.

```cpp
try {
  auto ct = pluss::kindle::Catalog::load("xml", "cities.csv");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Catalog::load: unsupported source type 'xml'
```

## Notes

- `source_type` matching is case-insensitive.
- The two-argument overload dispatches directly to the matching no-config `load_*()`
  method; the three-argument overload additionally accepts a generic `Config` scribble.
- Every underlying `load_*()` method's own throw conditions (missing file, malformed
  content, unsupported build configuration for Excel/Parquet) apply once dispatched.

## See Also

- [`Catalog::load_csv`](load_csv.md) / [`Catalog::load_txt`](load_txt.md) /
  [`Catalog::load_excel`](load_excel.md) / [`Catalog::load_parquet`](load_parquet.md) /
  [`Catalog::load_json`](load_json.md): the format-specific methods this one dispatches to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kindelize.html">
    <span class="label">Previous</span>
    <span class="title">kindelize</span>
  </a>
  <a class="next-link" href="load_csv.html">
    <span class="label">Next</span>
    <span class="title">load_csv</span>
  </a>
</div>
