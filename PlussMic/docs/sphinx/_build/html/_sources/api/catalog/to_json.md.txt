<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f1d063783034 -->

# Catalog::to_json

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::to_json() const -> std::string
```

## What It Does

Serializes the catalog to a JSON string describing its **own structure** — keys, and one
nested object per stream (kind, dtype, size, values, and a validity mask when the stream
has one) — not a row-oriented array of records.

```{important} A different JSON shape than save_json()/load_json()/from_json()
`to_json()`'s output is a structural/metadata-style JSON document (`{"_type": "Catalog",
"keys": [...], "streams": [...]}`), **not** the row-per-object array
(`[{"id": 1, "city": "Accra"}, ...]`) that [`save_json()`](save_json.md) writes and
[`load_json()`](load_json.md)/[`from_json()`](from_json.md) read. The two are not
round-trip compatible — `to_json()`'s output cannot be fed into `from_json()` to
reconstruct the same catalog. Use `save_json()`/`load_json()` for the row-oriented
interchange format; use `to_json()` when you want a full structural dump (useful for
debugging, logging, or diffing).
```

## Parameters

`None`

## Returns

`std::string`
    A JSON document describing the catalog's keys and, for each stream, its kind, dtype,
    size, values, and (if present) validity mask.

## Examples

Example 1. Serialize a small catalog.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

std::cout << ct.to_json();
```

### Output

```text
{
  "_type": "Catalog",
  "defined": true,
  "nkeys": 2,
  "nrecords": 3,
  "keys": [
    "id",
    "city"
  ],
  "streams": [
    {
      "key": "id",
      "value": {
  "_type": "Stream",
  "defined": true,
  "kind": "Series",
  "dtype": "Series[float64]",
  "size": 3,
  "values": [
    1,
    2,
    3
  ]
}
    },
    {
      "key": "city",
      "value": {
  "_type": "Stream",
  "defined": true,
  "kind": "Note",
  "dtype": "Note",
  "size": 3,
  "values": [
    "Accra",
    "Tema",
    "Kumasi"
  ]
}
    }
  ]
}
```

Example 2. A stream with a validity mask gets an extra `"validity"` array — invalid cells
still show their underlying stored value in `"values"`, not JSON `null`; `"validity"`
is what actually marks them invalid.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}),
        pluss::make_series(std::vector<bool>{true, false, true})),
});

std::cout << ct.to_json();
```

### Output

```text
{
  "_type": "Catalog",
  "defined": true,
  "nkeys": 1,
  "nrecords": 3,
  "keys": [
    "id"
  ],
  "streams": [
    {
      "key": "id",
      "value": {
  "_type": "Stream",
  "defined": true,
  "kind": "Series",
  "dtype": "Series[float64]",
  "size": 3,
  "values": [
    1,
    2,
    3
  ],
  "validity": [
    true,
    false,
    true
  ]
}
    }
  ]
}
```

Row `1` is invalid (per `"validity"`), but `"values"` still shows `2` at that position —
check `"validity"` to know which entries actually matter.

## Notes

- Not row-oriented, and not compatible with [`from_json()`](from_json.md)'s expected
  input shape — see the important callout above.
- `"validity"` only appears on a stream when that stream actually has a validity mask;
  a stream with no `NULL`s at all omits the key entirely (see Example 1).
- Invalid cells keep their underlying stored value in `"values"` — only `"validity"`
  marks them as not meaningful.

## See Also

- [`Catalog::save_json`](save_json.md) / [`Catalog::load_json`](load_json.md) /
  [`Catalog::from_json`](from_json.md): the row-oriented JSON interchange format, a
  different shape from this method's structural dump.
- [`Catalog::to_string`](to_string.md): the plain ASCII-table rendering, a third
  distinct text representation.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tfidf.html">
    <span class="label">Previous</span>
    <span class="title">tfidf</span>
  </a>
  <a class="next-link" href="to_string.html">
    <span class="label">Next</span>
    <span class="title">to_string</span>
  </a>
</div>
