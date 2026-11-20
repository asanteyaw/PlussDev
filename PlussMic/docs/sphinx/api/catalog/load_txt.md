<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 81ca301a4808 -->

# Catalog::load_txt

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
static Catalog Catalog::load_txt(std::string_view filepath)
static Catalog Catalog::load_txt(std::string_view filepath, const pluss::loader::Config& config)
```

## What It Does

Loads a delimited text file (tab-separated by default, with the same auto-detecting
parser [`load_csv()`](load_csv.md) uses under the hood) into a new `Catalog`, one stream
per column, with per-column dtype inference.

## Parameters

**filepath** (`std::string_view`)
    The text file to read.

**config** (`const pluss::loader::Config&`, optional)
    A generic configuration scribble, translated into the underlying CSV-style reader
    options.

## Returns

`Catalog`
    A new catalog with one stream per column.

## Examples

Example 1. Load a tab-separated text file.

Given `cities.txt`:

```text
city	population
Accra	2500000
Tema	161000
Kumasi	3000000
```

```cpp
auto ct = pluss::kindle::Catalog::load_txt("cities.txt");
std::cout << ct;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [city, population]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
city                     | Note         | Note             | 3       | [Accra, Tema, Kumasi]                            
population               | Series       | Series[int64]    | 3       | [2500000, 161000, 3000000]                       
========================================================================================================================
```

## Notes

- Shares its parsing engine with [`load_csv()`](load_csv.md), just with a text-file-style
  default separator rather than comma.
- Column dtype is inferred per-column from its values.
- [`load()`](load.md)`("txt", filepath)` (or `"text"`) is equivalent to this method's
  no-config overload.

## See Also

- [`Catalog::load_csv`](load_csv.md): the comma-delimited counterpart, sharing the same
  underlying parser.
- [`Catalog::save_txt`](save_txt.md): the write-side counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_streams.html">
    <span class="label">Previous</span>
    <span class="title">load_streams</span>
  </a>
  <a class="next-link" href="map.html">
    <span class="label">Next</span>
    <span class="title">map</span>
  </a>
</div>
