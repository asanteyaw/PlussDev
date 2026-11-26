<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 624d39e09431 -->

# Registry::load_txt

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
static Registry Registry::load_txt(std::string_view filepath)
static Registry Registry::load_txt(std::string_view filepath, const pluss::loader::Config& config)
```

## What It Does

**A `static` factory method** — loads a delimited text file (tab-separated by
default, with the same auto-detecting parser [`load_csv()`](load_csv.md) uses under
the hood) into a new `Registry`, one field (slot) per column, with per-column dtype
inference.

## Parameters

**filepath** (`std::string_view`)

The text file to read.

**config** (`const pluss::loader::Config&`, optional)

A generic configuration scribble, translated into the underlying CSV-style reader
options — see [`load_csv()`](load_csv.md)'s important callout for the recognized
`"ncases"` row-limit key.

## Returns

`Registry`
    A new registry with one field per column.

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
auto rg = pluss::scribble::Registry::load_txt("cities.txt");
std::cout << rg;
```

### Output

```text
========================================================================================================================
scribble::Registry  fields=2
states=3
------------------------------------------------------------------------------------------------------------------------
field      | ptype  | states | preview
------------------------------------------------------------------------------------------------------------------------
city       | Note   | 3      | [Accra, Tema, Kumasi]
population | Series | 3      | [2500000, 161000, 3000000]
========================================================================================================================
```

## Notes

- Shares its parsing engine with [`load_csv()`](load_csv.md), just with a
  text-file-style default separator rather than comma.
- Column dtype is inferred per-column from its values.
- [`load()`](load.md)`("txt", filepath)` (or `"text"`) is equivalent to this method's
  no-config overload.

## See Also

- [`Registry::load_csv`](load_csv.md): the comma-delimited counterpart, sharing the
  same underlying parser.
- [`Catalog::load_txt`](../catalog/load_txt.md): the equivalent loader on the kindle
  model.
- [`Registry::save_txt`](save_txt.md): the write-side counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_parquet.html">
    <span class="label">Previous</span>
    <span class="title">load_parquet</span>
  </a>
  <a class="next-link" href="map.html">
    <span class="label">Next</span>
    <span class="title">map</span>
  </a>
</div>
