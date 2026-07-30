<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7ced7ad0ba69 -->

# Catalog::load_csv_with_key

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
static Catalog Catalog::load_csv_with_key(std::string_view filepath, std::string_view key_field, const pluss::loader::ReadCsvOptions& options = {})
```

## What It Does

Loads a CSV the same way [`load_csv()`](load_csv.md) does, then **removes** `key_field`
from the regular streams and re-exposes it as the catalog's case-label axis, materialized
under the name `"_label"`. This is useful when one CSV column is a row identifier you want
attached as metadata rather than treated as an ordinary data stream.

```{important} key_field is removed from the stream list, not just relabeled
After this call, `key_field`'s original name no longer exists as a stream — [`has()`](has.md)
returns `false` for it. The values live on under the fixed name `"_label"` instead. If you
want to keep the key as an ordinary, still-named stream *and* attach it as a label, use
[`load_csv()`](load_csv.md) followed by your own labeling step instead.
```

## Parameters

**filepath** (`std::string_view`)
    The CSV file to read.

**key_field** (`std::string_view`)
    The column name to promote to the case-label axis. Must exist in the file's header.

**options** (`const pluss::loader::ReadCsvOptions&`, optional)
    Same parsing options as [`load_csv()`](load_csv.md).

## Returns

`Catalog`
    A new catalog with `key_field` removed from the regular streams and re-exposed as a
    `"_label"` stream.

## Examples

Example 1. Load a CSV, promoting its `id` column to the label axis.

```cpp
auto ct = pluss::kindle::Catalog::load_csv_with_key("cities.csv", "id");
std::cout << ct;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=3
keys: [city, population, _label]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
city                     | Note         | Note             | 3       | [Accra, Tema, Kumasi]                            
population               | Series       | Series[int64]    | 3       | [2388000, 300000, 3348000]                       
_label                   | Series       | Series[int64]    | 3       | [1, 2, 3]                                        
========================================================================================================================
```

`id` no longer appears as its own stream — its values moved to `_label`.

Example 2. Requesting a key field that doesn't exist in the file throws.

```cpp
try {
  auto ct = pluss::kindle::Catalog::load_csv_with_key("cities.csv", "zzz");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Catalog::load_csv_with_key: key field not found 'zzz'
```

## Notes

- `key_field` is removed as a named stream, not just relabeled — see the important
  callout above.
- The label stream is always named `"_label"`, regardless of the source column's original
  name.
- `key_field` must be non-empty and must exist in the file's header, or the call throws.

## See Also

- [`Catalog::load_csv`](load_csv.md): the same loading without key promotion.
- [`Catalog::kindelize`](kindelize.md): the underlying `materialize_label` mechanism this
  method uses internally.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_csv.html">
    <span class="label">Previous</span>
    <span class="title">load_csv</span>
  </a>
  <a class="next-link" href="load_excel.html">
    <span class="label">Next</span>
    <span class="title">load_excel</span>
  </a>
</div>
