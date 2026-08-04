<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5ed48f7e6bb0 -->

# Registry::save_json

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
void Registry::save_json(std::string_view filepath) const
void Registry::save_json(std::string_view filepath, const pluss::loader::Config& config) const
```

## What It Does

Writes the registry to a JSON file. Unlike every other `save_*` method,
`save_json()` does **not** unconditionally require the registry to bridge cleanly
into the kindle model:

- **Uniform field kinds**: converts via
  [`to_kindle()`](to_kindle.md) and writes a row-major array-of-objects JSON, same
  shape as `Catalog::save_json()`.
- **Mixed field kinds**: falls back to
  writing this registry's own [`to_json()`](to_json.md) output directly — the `config`
  parameter is ignored in this path.

```{important} save_json is the one saver that accepts heterogeneous registries
Every other `save_*` method (`save_csv`, `save_txt`, `save_excel`, `save_parquet`,
and `save_as` for any target other than `"json"`) throws on a registry with a
genuinely mixed-type field. `save_json()` is the only one that can round-trip such a
registry at all, via its `to_json()` fallback.
```

## Parameters

**filepath** (`std::string_view`)
    The destination file path.

**config** (`const pluss::loader::Config&`, optional)
    A generic configuration bundle, translated into the underlying JSON writer
    options. Ignored when the registry takes the `to_json()`
    fallback path).

## Returns

`void`
    Writes the file as a side effect; produces no return value.

## Examples

Example 1. A stream-compatible registry writes a row-major array-of-objects JSON,
the same shape [`load_json()`](load_json.md) reads back.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_json("cities.json");

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

Example 2. A registry with a genuinely mixed-type field falls back to `to_json()`'s
per-value output, rather than throwing.

```cpp
auto mixed = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"mixed", pluss::bundle::Slot({pluss::make_series(10), pluss::make_note("hello"), pluss::make_series(3.5)})},
});

mixed.save_json("mixed.json");

std::ifstream saved("mixed.json");
std::cout << saved.rdbuf();
```

### Output

```text
{
  "_type": "Registry",
  "defined": true,
  "nfields": 2,
  "fields": {
    "id": {
  "_type": "Slot",
  "defined": true,
  "size": 3,
  "values": [
    "Series[int64]",
    "Series[int64]",
    "Series[int64]"
  ]
},
    "mixed": {
  "_type": "Slot",
  "defined": true,
  "size": 3,
  "values": [
    "Series[int64]",
    "Note[numel=1]",
    "Series[float64]"
  ]
}
  }
}
```

Note that every `id` row correctly reports `Series[int64]` — CSV-loaded or
directly-constructed integer values no longer get mistyped as `Series[bool]` for an
ambiguous first value; see [`Registry::to_json`](to_json.md) for details.

## Notes

- The only `save_*` method that can succeed on a genuinely heterogeneous registry —
  see the important callout above.
- The uniform-kind path's JSON shape matches
  [`Catalog::save_json`](../catalog/save_json.md).
- The fallback path's shape matches [`Registry::to_json`](to_json.md) exactly, since
  it writes that method's output verbatim.
- [`save_as()`](save_as.md)`("json", filepath)` is equivalent to this method's
  no-config overload — `Registry::save_as` special-cases `"json"` to call
  `save_json()` directly, rather than routing through the shared
  `to_kindle()`-requiring path every other target type uses.

## See Also

- [`Registry::load_json`](load_json.md): the read-side counterpart for the
  stream-compatible shape.
- [`Registry::to_json`](to_json.md): the per-value output this method falls back
  to for heterogeneous registries.
- [`Catalog::save_json`](../catalog/save_json.md): the equivalent saver on the
  kindle model.

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
