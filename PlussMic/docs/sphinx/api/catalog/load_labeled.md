# Catalog::load_labeled

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog Catalog::load_labeled(std::string_view filepath, std::string_view label_field, const pluss::loader::ReadCsvOptions& options = {})
```

## What It Does

Loads a CSV catalog and uses one field as stream case labels.

## Parameters

**filepath**
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

**label_field**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`Catalog`
    Value produced by this call.

## Examples

Example 1. Pass a typed reader-options object to control file loading details.

```cpp
auto out = pluss::kindle::Catalog::load_labeled("path/to/data.csv", "id");
```

### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_json.html">
    <span class="label">Previous</span>
    <span class="title">load_json</span>
  </a>
  <a class="next-link" href="load_parquet.html">
    <span class="label">Next</span>
    <span class="title">load_parquet</span>
  </a>
</div>
