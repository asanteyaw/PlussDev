<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6b02930e3f0b -->

# Catalog::save_json

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
Catalog::save_json(std::string_view filename) const -> void
Catalog::save_json(std::string_view filename, const pluss::loader::Config& config) const -> void
```

## What It Does

Writes the catalog to a JSON file — an array of one flat object per row — using the same
outer-align, positional [`tabelize()`](tabelize.md) step every other `save_*` method
(except [`save_as()`](save_as.md)) uses.

```{important} Catalog::to_json() is a different JSON shape, not this method's in-memory equivalent
[`to_json()`](to_json.md) does return real JSON, but a structural/metadata dump of the
catalog (`{"_type": "Catalog", "keys": [...], "streams": [...]}`), not the row-per-object
array (`[{"id": 1, "city": "Accra"}, ...]`) this method writes to disk. If you need the
row-oriented JSON text in memory rather than on disk, save to a temporary file with
`save_json()` and read it back — there is currently no in-memory accessor for that
specific shape.
```

## Parameters

**filename** (`std::string_view`)
    The destination file path.

**config** (`const pluss::loader::Config&`, optional)
    A generic configuration scribble, translated into the underlying JSON writer options.

## Returns

`void`
    Writes the file as a side effect; produces no return value.

## Examples

Example 1. Save a catalog to JSON and read the file back.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_json("cities.json");

std::ifstream saved("cities.json");
std::cout << saved.rdbuf();
```

### Output

```text
[
  {
    "id": 1,
    "city": "Accra"
  },
  {
    "id": 2,
    "city": "Tema"
  },
  {
    "id": 3,
    "city": "Kumasi"
  }
]
```

## Notes

- See the important callout above — [`to_json()`](to_json.md) is real JSON too, just a
  different (structural, not row-oriented) shape.
- Same ragged-catalog tolerance as [`save_csv()`](save_csv.md).

## See Also

- [`Catalog::load_json`](load_json.md) / [`Catalog::from_json`](from_json.md): the
  read-side counterparts, expecting this same row-oriented shape.
- [`Catalog::to_json`](to_json.md): a structural JSON dump, a different shape from this
  method's row-oriented output.

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
