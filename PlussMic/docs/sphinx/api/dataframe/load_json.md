<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d94571dd355b -->

# DataFrame::load_json

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
static DataFrame load_json(std::string_view filepath) -> DataFrame
static DataFrame load_json(std::string_view filepath, const pluss::loader::Config& config) -> DataFrame
static DataFrame load_json(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) -> DataFrame
```

## What It Does

`DataFrame`'s static constructor for JSON files. The parser auto-detects one of two
layouts based on the JSON root's shape:

- **Records** (a JSON array of objects, one object per row) — the more common shape,
  and the one [`save_json()`](save_json.md) writes by default.
- **Split** (a single JSON object with a `"columns"` array and a parallel `"data"`
  array of row arrays) — a more compact alternative layout, also produced by
  [`save_json()`](save_json.md) when asked for `orient = "split"`.

```{important} Values are re-inferred the same way load_csv() infers them
Even though JSON natively distinguishes numbers, strings, and booleans, `load_json()`
converts every cell to its string form internally and re-runs it through the exact
same numeric/text type-inference engine [`load_csv()`](load_csv.md) uses — it does
not trust JSON's own value tags directly. In practice this rarely matters (a JSON
number reads back as a `Series`, a JSON string as a `Note`), but it means the same
edge cases that affect `load_csv()`'s inference (a column that's "mostly numbers plus
one stray string" falling back to `Note`) apply here too, even though the source file
had unambiguous per-value JSON types.
```

## Parameters

**filepath** (`std::string_view`)

Path to the JSON file to read.

**config** (`const pluss::loader::Config&`) / **config_entries**
(`std::initializer_list<pluss::loader::ConfigEntry>`)

A reusable or inline settings bundle, recognizing the same general-purpose keys
(`ncases`, `usefeats`, `case`, and so on) as [`load_csv()`](load_csv.md)'s `Config`
overload — the layout (records vs. split) is detected automatically from the file
itself and is not a setting you choose.

## Returns

`DataFrame`
    A new, fully typed frame, with the same numeric/text type inference behavior as
    [`load_csv()`](load_csv.md).

## Examples

Example 1. Load a records-style JSON file (an array of row objects).

Given `orders.json`:

```json
[
  {"id": 101, "city": "Accra", "units": 2},
  {"id": 102, "city": "Tema", "units": 1},
  {"id": 103, "city": "Kumasi", "units": 4}
]
```

```cpp
using namespace pluss::table;

auto orders = DataFrame::load_json("orders.json");
std::cout << orders;
```

### Output

```text
==========================
     | id    city    units
--------------------------
0    | 101   Accra   2    
1    | 102   Tema    1    
2    | 103   Kumasi  4    
--------------------------
[3 cs x 3 feat]
```

Example 2. Load a split-style JSON file (columns and row-data kept separately).

Given `orders_split.json`:

```json
{
  "columns": ["id", "city", "units"],
  "data": [
    [101, "Accra", 2],
    [102, "Tema", 1],
    [103, "Kumasi", 4]
  ]
}
```

```cpp
auto orders_split = DataFrame::load_json("orders_split.json");
std::cout << orders_split;
```

### Output

```text
==========================
     | id    city    units
--------------------------
0    | 101   Accra   2    
1    | 102   Tema    1    
2    | 103   Kumasi  4    
--------------------------
[3 cs x 3 feat]
```

Both files produce the identical frame — `load_json()` detects which of the two
layouts it's looking at from the JSON root's shape (an array means records; an object
with `"columns"`/`"data"` means split) rather than requiring you to specify which one
the file uses.

## Notes

- Type inference happens the same way it does for [`load_csv()`](load_csv.md) — from
  each column's string-converted values, not from JSON's own type tags. See the
  caution above.
- A JSON root that is neither an array of objects nor a `{"columns", "data"}` object
  fails to parse.
- Empty/`null` JSON values become missing cells, matching `load_csv()`'s empty-cell
  behavior.

## See Also

- [`DataFrame::save_json`](save_json.md): the write-side counterpart, including the
  `orient` setting that controls which of these two layouts (plus a third,
  column-oriented layout) gets written.
- [`DataFrame::load_csv`](load_csv.md): the fuller explanation of the shared
  type-inference engine.
- [`DataFrame::load`](load.md): the format-switched entry point that can dispatch to
  `load_json()` via `source_type = "json"`.

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
