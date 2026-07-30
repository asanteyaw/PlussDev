<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5bdb10ad1975 -->

# DataFrame::save_json

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::save_json(std::string_view filepath) const -> void
DataFrame::save_json(std::string_view filepath, const pluss::loader::Config& config) const -> void
DataFrame::save_json(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const -> void
```

## What It Does

Writes the frame to a JSON file, in one of three layouts controlled by the `orient`
config key:

- **`"records"`** (the default) — a JSON array of one object per case, each keyed by
  feature name. This is also the layout [`load_json()`](load_json.md) reads most
  naturally.
- **`"columns"`** — a single JSON object keyed by feature name, each value a JSON
  array of that feature's values across every case.
- **`"split"`** — a single JSON object with a `"columns"` array of feature names and a
  parallel `"data"` array of row arrays. This is the other layout
  [`load_json()`](load_json.md) recognizes on read.

Output is pretty-printed with a 2-space indent by default; set `pretty=false` for
compact, single-line output.

## Parameters

**filepath** (`std::string_view`)

Destination file path.

**config** (`const pluss::loader::Config&`) / **config_entries**
(`std::initializer_list<pluss::loader::ConfigEntry>`)

Recognized keys:

- **`orient`** (`std::string`, default `"records"`): `"records"`, `"columns"`, or
  `"split"` — see "What It Does" above.
- **`pretty`** (`bool`, default `true`): whether to add newlines and indentation.
  Passing an integer instead of a bool also works — any value greater than `0` is
  treated as `true`.
- **`indent`** / **`json_indent`** (integer, `1`-`8`, default `2`): number of spaces
  per indent level, when `pretty` is on.
- **`addcases`** / **`cases`** / **`case`** / **`index`** (`bool`, default `false`):
  export the case axis as its own field/column, the same convention
  [`save_csv()`](save_csv.md)'s `add_cases` uses.
- **`caselabel`** / **`indexlabel`** / **`case_feature`** (`std::string`): field name
  for the exported case-axis column when `addcases` is enabled.

## Returns

`void`
    Performs the write and returns nothing.

## Examples

Example 1. The default `"records"` orientation.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",   pluss::make_series({101, 102})},
    {"city", pluss::make_note({"Accra", "Tema"})},
});

orders.save_json("orders_records.json");
```

Resulting `orders_records.json`:

```json
[
  {
    "id": 101,
    "city": "Accra"
  },
  {
    "id": 102,
    "city": "Tema"
  }
]
```

Example 2. The `"columns"` orientation.

```cpp
orders.save_json("orders_columns.json", {{"orient", std::string("columns")}});
```

Resulting `orders_columns.json`:

```json
{
  "id": [101, 102],
  "city": ["Accra", "Tema"]
}
```

Example 3. The `"split"` orientation — the other layout [`load_json()`](load_json.md)
can read back directly.

```cpp
orders.save_json("orders_split.json", {{"orient", std::string("split")}});
```

Resulting `orders_split.json`:

```json
{
  "columns": ["id", "city"],
  "data": [
    [101, "Accra"],
    [102, "Tema"]
  ]
}
```

## Notes

- `"records"` is both the default write orientation and one of the two layouts
  [`load_json()`](load_json.md) auto-detects on read; `"split"` is the other.
  `"columns"` is write-only — `load_json()` does not currently parse a bare
  column-keyed object back into a frame.
- Set `pretty=false` for compact output with no added whitespace, useful when file
  size matters more than human readability.
- `addcases`/`caselabel` follow the same convention as [`save_csv()`](save_csv.md)'s
  `add_cases`/`case_label` — export the case axis as its own field so a later
  reload can restore it.

## See Also

- [`DataFrame::load_json`](load_json.md): the read-side counterpart, including which
  of these layouts it can parse back.
- [`DataFrame::save_csv`](save_csv.md): the analogous CSV writer, sharing the
  `add_cases`/`case_label` convention.
- [`DataFrame::save_as`](save_as.md): the format-switched entry point that can
  dispatch to `save_json()` via `target_type = "json"`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_excel.html">
    <span class="label">Previous</span>
    <span class="title">save_excel</span>
  </a>
  <a class="next-link" href="save_parquet.html">
    <span class="label">Next</span>
    <span class="title">save_parquet</span>
  </a>
</div>
