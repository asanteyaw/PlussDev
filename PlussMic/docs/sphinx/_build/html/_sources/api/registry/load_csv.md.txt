<!-- pluss-hand-authored -->

# Registry::load_csv

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
static Registry Registry::load_csv(std::string_view filepath)
static Registry Registry::load_csv(std::string_view filepath, const pluss::loader::Config& config)
static Registry Registry::load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options)
```

## What It Does

**A `static` factory method** — reads a CSV file into a new `Registry`, one field
(slot) per column, with the column's dtype inferred from its values (numeric columns
become `Series`-kind, everything else becomes `Note`-kind).

## Parameters

**filepath** (`std::string_view`)
    The CSV file to read.

**config** (`const pluss::loader::Config&`, optional)
    A generic configuration bundle, translated into `ReadCsvOptions` internally. The
    row-limiting key is `"ncases"`, not `"nrows"`.

**options** (`const pluss::loader::ReadCsvOptions&`, optional)
    Fine-grained control over parsing — delimiter detection, quoting, parallel-read
    thresholds, and more.

## Returns

`Registry`
    A new registry with one field per CSV column.

## Examples

Example 1. Load a CSV file with default options.

```cpp
auto rg = pluss::bundle::Registry::load_csv("cities.csv");
std::cout << rg;
```

### Output

```text
{
  "_type": "Registry",
  "defined": true,
  "nfields": 3,
  "fields": {
    "id": {
      "size": 3,
      "types": {
        "Series": 3
      }
    },
    "city": {
      "size": 3,
      "types": {
        "Note": 3
      }
    },
    "population": {
      "size": 3,
      "types": {
        "Series": 3
      }
    }
  }
}
```

Numeric columns (`id`, `population`) are inferred as `Series`-kind; the text column
(`city`) becomes `Note`-kind.

Example 2. Pass a `Config` bundle to limit the number of rows read, via the
`"ncases"` key.

```cpp
auto rg = pluss::bundle::Registry::load_csv("cities.csv", pluss::loader::Config{{"ncases", 2}});
std::cout << rg;
```

### Output

```text
{
  "_type": "Registry",
  "defined": true,
  "nfields": 3,
  "fields": {
    "id": {
      "size": 2,
      "types": {
        "Series": 2
      }
    },
    "city": {
      "size": 2,
      "types": {
        "Note": 2
      }
    },
    "population": {
      "size": 2,
      "types": {
        "Series": 2
      }
    }
  }
}
```

```{important} Config's row-limit key is "ncases", not "nrows"
Every field's `size` drops from `3` to `2` — confirming `Config{{"ncases", 2}}` is the
correct key. A key named `"nrows"` is silently ignored (unrecognized `Config` keys
never throw or warn), leaving every row loaded — verified directly against this
build's `pluss::loader::csv_read_options_from_config` translation logic.
```

## Notes

- Column dtype is inferred per-column from its values.
- [`load()`](load.md)`("csv", filepath)` is equivalent to this method's no-options
  overload.
- See the important callout above before reaching for a `Config`-based row limit —
  the recognized key is `"ncases"`.
- Unlike [`Catalog::load_csv`](../catalog/load_csv.md), `Registry`'s printed form is a
  JSON summary (field sizes and per-type counts), not a box table.

## See Also

- [`Registry::load`](load.md): the format-dispatching entry point this method backs.
- [`Catalog::load_csv`](../catalog/load_csv.md): the equivalent loader on the kindle
  model.
- [`Registry::save_csv`](save_csv.md): the write-side counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load.html">
    <span class="label">Previous</span>
    <span class="title">load</span>
  </a>
  <a class="next-link" href="load_excel.html">
    <span class="label">Next</span>
    <span class="title">load_excel</span>
  </a>
</div>
