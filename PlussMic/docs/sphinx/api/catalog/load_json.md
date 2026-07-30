<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 458082e3b6a2 -->

# Catalog::load_json

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
static Catalog Catalog::load_json(std::string_view filepath)
static Catalog Catalog::load_json(std::string_view filepath, const pluss::loader::Config& config)
```

## What It Does

Loads a JSON file (an array of flat objects) into a new `Catalog`, one stream per distinct
key across the objects, with per-column dtype inference — the same underlying inference
[`load_csv()`](load_csv.md) uses, applied to the JSON values instead of parsed CSV text.

## Parameters

**filepath** (`std::string_view`)
    The JSON file to read. Expected shape: a top-level array of flat (non-nested) objects.

**config** (`const pluss::loader::Config&`, optional)
    A generic configuration bundle, translated into the underlying reader options.

## Returns

`Catalog`
    A new catalog with one stream per distinct key found across the JSON objects.

## Examples

Example 1. Load a JSON array of objects.

```cpp
auto ct = pluss::kindle::Catalog::load_json("cities.json");
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

## Notes

- Expects a flat array-of-objects JSON shape — the same shape
  [`from_json()`](from_json.md) parses from an in-memory string instead of a file.
- Column dtype is inferred per-column from its values.
- [`load()`](load.md)`("json", filepath)` is equivalent to this method's no-config
  overload.

## See Also

- [`Catalog::from_json`](from_json.md): the same parsing, from an in-memory string
  instead of a file.
- [`Catalog::save_json`](save_json.md): the write-side counterpart (the same
  row-oriented shape this method reads).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_excel.html">
    <span class="label">Previous</span>
    <span class="title">load_excel</span>
  </a>
  <a class="next-link" href="load_parquet.html">
    <span class="label">Next</span>
    <span class="title">load_parquet</span>
  </a>
</div>
