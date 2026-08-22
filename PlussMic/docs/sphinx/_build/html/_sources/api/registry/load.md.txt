<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: aa55b0b46a8c -->

# Registry::load

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
static Registry Registry::load(std::string_view source_type, std::string_view filepath)
static Registry Registry::load(std::string_view source_type, std::string_view filepath, const pluss::loader::Config& config)
```

## What It Does

**A `static` factory method** that dispatches by name to one of the format-specific
loaders — `"csv"` to [`load_csv()`](load_csv.md), `"txt"`/`"text"` to
[`load_txt()`](load_txt.md), `"excel"`/`"xlsx"` to [`load_excel()`](load_excel.md),
`"parquet"` to [`load_parquet()`](load_parquet.md), `"json"` to
[`load_json()`](load_json.md). Matching is case-insensitive. Use `load()` when the
format is only known at runtime; reach for the specific `load_*()` method directly
otherwise.

## Parameters

**source_type** (`std::string_view`)
    The format name — `"csv"`, `"txt"`/`"text"`, `"excel"`/`"xlsx"`, `"parquet"`, or
    `"json"` (case-insensitive). Anything else throws.

**filepath** (`std::string_view`)
    The file to read.

**config** (`const pluss::loader::Config&`, optional)
    A generic, format-agnostic configuration scribble, translated internally into the
    format-specific options struct.

## Returns

`Registry`
    A new registry with one field (slot) per detected column.

## Examples

Example 1. Load a CSV file by naming the format explicitly.

```cpp
auto rg = pluss::scribble::Registry::load("csv", "cities.csv");
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

Example 2. An unrecognized format name throws.

```cpp
try {
  auto rg = pluss::scribble::Registry::load("xml", "cities.csv");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Registry::load: unsupported source type 'xml'
```

## Notes

- `source_type` matching is case-insensitive.
- The two-argument overload dispatches directly to the matching no-config `load_*()`
  method; the three-argument overload additionally accepts a generic `Config` scribble.
- Every underlying `load_*()` method's own throw conditions (missing file, malformed
  content, unsupported build configuration for Excel/Parquet) apply once dispatched.
- Unlike [`Catalog`](../catalog/load.md), `Registry`'s printed form is a JSON summary
  (field names, sizes, per-type counts), not a box table — see any example's output
  above.

## See Also

- [`Registry::load_csv`](load_csv.md) / [`Registry::load_txt`](load_txt.md) /
  [`Registry::load_excel`](load_excel.md) / [`Registry::load_parquet`](load_parquet.md) /
  [`Registry::load_json`](load_json.md): the format-specific methods this one
  dispatches to.
- [`Catalog::load`](../catalog/load.md): the equivalent factory on the kindle model.

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
