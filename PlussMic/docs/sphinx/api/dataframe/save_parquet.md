<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a88cc3922cda -->

# DataFrame::save_parquet

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
void DataFrame::save_parquet(std::string_view filepath) const
void DataFrame::save_parquet(std::string_view filepath, const pluss::loader::Config& config) const
void DataFrame::save_parquet(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const
```

## What It Does

Writes the frame to a Parquet file via Apache Arrow's Parquet C++ library —
compact, columnar, typed storage, the write-side counterpart to
[`load_parquet()`](load_parquet.md).

```{important} Requires an Arrow + Parquet C++ build
Like `load_parquet()`, `save_parquet()` throws *"built without parquet-cpp support"*
if PlussMic was compiled without the optional `PLUSSMIC_HAS_PARQUET_CPP` dependency.
```

## Parameters

**filepath** (`std::string_view`)

Destination Parquet file path.

**config** (`const pluss::loader::Config&`) / **config_entries**
(`std::initializer_list<pluss::loader::ConfigEntry>`)

Recognized keys:

- **`addcases`** / **`cases`** / **`case`** / **`index`** (`bool`, default `false`):
  export the case axis as its own column, matching
  [`save_csv()`](save_csv.md)'s `add_cases` convention.
- **`caselabel`** / **`indexlabel`** / **`case_feature`** (`std::string`): column
  name for the exported case-axis column when the flag above is enabled.

## Returns

`void`
    Performs the write and returns nothing.

## Examples

Example 1. Save with the plain filepath-only overload — no case-axis column.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Slate(pluss::make_series({101, 102, 103}))},
    {"city",  Slate(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
    {"units", Slate(pluss::make_series({2, 1, 4}))},
});

orders.save_parquet("orders_out.parquet");
```

Resulting `orders_out.parquet`, shown as its logical column store:

```text
| id  | city   | units |
| 101 | Accra  | 2     |
| 102 | Tema   | 1     |
| 103 | Kumasi | 4     |
```

Example 2. Export the case axis as its own named column.

```cpp
orders.save_parquet("orders_with_case.parquet", {
    {"addcases", true},
    {"caselabel", std::string("case_id")},
});
```

Resulting `orders_with_case.parquet`:

```text
| case_id | id  | city   | units |
| 0       | 101 | Accra  | 2     |
| 1       | 102 | Tema   | 1     |
| 2       | 103 | Kumasi | 4     |
```

## Notes

- Requires PlussMic to have been built with Arrow + Parquet C++ support, exactly like
  [`load_parquet()`](load_parquet.md); otherwise every call throws immediately.
- `addcases`/`caselabel` follow the same convention as
  [`save_csv()`](save_csv.md)'s `add_cases`/`case_label` and
  [`save_excel()`](save_excel.md)'s equivalent fields.
- Values are written using Arrow's own Parquet type mapping from PlussMic's inferred
  `Series`/`Note` dtypes — but recall from [`load_parquet()`](load_parquet.md) that
  reading the file back re-infers types from stringified values rather than trusting
  the Parquet schema directly, so a save/load round trip is not guaranteed to preserve
  the exact original precision (for example, a narrower integer width).

## See Also

- [`DataFrame::load_parquet`](load_parquet.md): the read-side counterpart, including
  the schema-discarding re-inference behavior worth knowing about before round-tripping.
- [`DataFrame::save_csv`](save_csv.md): the analogous CSV writer and the fuller
  explanation of the `add_cases`/`case_label` convention shared across writers.
- [`DataFrame::save_as`](save_as.md): the format-switched entry point that can
  dispatch to `save_parquet()` via `target_type = "parquet"`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_json.html">
    <span class="label">Previous</span>
    <span class="title">save_json</span>
  </a>
  <a class="next-link" href="save_txt.html">
    <span class="label">Next</span>
    <span class="title">save_txt</span>
  </a>
</div>
