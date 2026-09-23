<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6a97b7a1f6a3 -->

# Catalog::save_as

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
Catalog::save_as(std::string_view target_type, std::string_view filename, const pluss::loader::Config& config = {}) const -> void
```

## What It Does

Saves the catalog to a format chosen by a runtime string — `"csv"`, `"txt"`, `"excel"`/
`"xlsx"`, `"json"`, or `"parquet"` — rather than calling a fixed method name. Like the
named `save_*()` methods, it goes through the same outer-align, positional
[`tabelize()`](tabelize.md) step first. Use `save_as()` when the output format is only
known at runtime; reach for the specific `save_*()` method directly otherwise.

## Parameters

**target_type** (`std::string_view`)
    The output format name. Anything not recognized by the underlying writer throws.

**filename** (`std::string_view`)
    The destination file path.

**config** (`const pluss::loader::Config&`, optional)
    A generic configuration scribble, translated into the format-specific writer options.

## Returns

`void`
    Writes the file as a side effect; produces no return value.

## Examples

Example 1. Save to JSON by naming the format explicitly.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_as("json", "cities.json");

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

Example 2. An unrecognized format name throws.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});

try {
  ct.save_as("xml", "out.xml");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
loader::save_as: unsupported target type 'xml'
```

## Notes

- `target_type` matching is case-sensitive to the underlying writer's own accepted
  strings (lowercase format names, as shown above).
- Same ragged-catalog tolerance as the individual `save_*()` methods, since it goes
  through the same `tabelize()` step.

## See Also

- [`Catalog::save_csv`](save_csv.md) / [`Catalog::save_txt`](save_txt.md) /
  [`Catalog::save_excel`](save_excel.md) / [`Catalog::save_json`](save_json.md) /
  [`Catalog::save_parquet`](save_parquet.md): the format-specific methods, for when the
  target format is known at compile time.
- [`Catalog::load`](load.md): the read-side, runtime-dispatched counterpart.

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
