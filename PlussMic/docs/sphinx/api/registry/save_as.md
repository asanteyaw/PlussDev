<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e960a909b7a3 -->

# Registry::save_as

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
void Registry::save_as(std::string_view target_type, std::string_view filepath, const pluss::loader::Config& config = {}) const
```

## What It Does

Saves the registry to a format chosen by a runtime string — `"csv"`, `"txt"`,
`"excel"`/`"xlsx"`, `"json"`, or `"parquet"` — rather than calling a fixed method
name.

`target_type == "json"` is special-cased: it calls [`save_json()`](save_json.md)
directly, which means it inherits that method's heterogeneous-registry fallback.
Every other target type routes through the shared `kindelize()`-requiring path (the
same one [`save_csv()`](save_csv.md), [`save_txt()`](save_txt.md),
[`save_excel()`](save_excel.md), and [`save_parquet()`](save_parquet.md) use), and
throws on a genuinely mixed-type field.

## Parameters

**target_type** (`std::string_view`)
    The output format name. Anything not recognized by the underlying writer throws.

**filepath** (`std::string_view`)
    The destination file path.

**config** (`const pluss::loader::Config&`, optional)
    A generic configuration bundle, translated into the format-specific writer
    options.

## Returns

`void`
    Writes the file as a side effect; produces no return value.

## Examples

Example 1. Save to JSON by naming the format explicitly.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_as("json", "cities.json");

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

Example 2. An unrecognized `target_type` throws.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

try {
  rg.save_as("xml", "cities.xml");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
loader::save_as: unsupported target type 'xml'
```

## Notes

- `"json"` is the only target type that tolerates a heterogeneous registry — see the
  important behavior described above.
- Use `save_as()` when the output format is only known at runtime; reach for the
  specific `save_*()` method directly otherwise.

## See Also

- [`Registry::load`](load.md): the format-dispatching read-side counterpart.
- [`Registry::save_json`](save_json.md): the method `save_as("json", ...)` calls
  directly, including its heterogeneous-registry fallback.
- [`Catalog::save_as`](../catalog/save_as.md): the equivalent saver on the kindle
  model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sample.html">
    <span class="label">Previous</span>
    <span class="title">sample</span>
  </a>
  <a class="next-link" href="save_csv.html">
    <span class="label">Next</span>
    <span class="title">save_csv</span>
  </a>
</div>
