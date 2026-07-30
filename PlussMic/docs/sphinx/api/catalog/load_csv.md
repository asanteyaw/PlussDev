<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a40b979a96b3 -->

# Catalog::load_csv

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
static Catalog Catalog::load_csv(std::string_view filepath)
static Catalog Catalog::load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options)
```

## What It Does

**A `static` factory method** — reads a CSV file into a new `Catalog`, one stream per
column, with the column's dtype inferred from its values (numeric columns become
`Series`, everything else becomes `Note`).

## Parameters

**filepath** (`std::string_view`)
    The CSV file to read.

**options** (`const pluss::loader::ReadCsvOptions&`, optional)
    Fine-grained control over parsing — delimiter detection, quoting, parallel-read
    thresholds, and more.

## Returns

`Catalog`
    A new catalog with one stream per CSV column.

## Examples

Example 1. Load a CSV file with default options.

```cpp
auto ct = pluss::kindle::Catalog::load_csv("cities.csv");
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

Numeric columns (`id`, `population`) are inferred as `Series`; the text column (`city`)
becomes `Note`.

## Notes

- Column dtype is inferred per-column from its values, not declared up front.
- [`load()`](load.md)`("csv", filepath)` is equivalent to this method's no-options
  overload.

## See Also

- [`Catalog::load_csv_with_key`](load_csv_with_key.md): the same CSV loading, plus
  promoting one column to the case-label axis.
- [`Catalog::load`](load.md): the format-dispatching entry point this method backs.
- [`Catalog::save_csv`](save_csv.md): the write-side counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load.html">
    <span class="label">Previous</span>
    <span class="title">load</span>
  </a>
  <a class="next-link" href="load_csv_with_key.html">
    <span class="label">Next</span>
    <span class="title">load_csv_with_key</span>
  </a>
</div>
