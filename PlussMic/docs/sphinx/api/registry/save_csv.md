<!-- pluss-hand-authored -->

# Registry::save_csv

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
void Registry::save_csv(std::string_view filepath) const
void Registry::save_csv(std::string_view filepath, const pluss::loader::Config& config) const
```

## What It Does

Writes the registry to a CSV file. Internally, `save_csv()` (and every `save_*`
method except [`save_json()`](save_json.md)) first converts the registry to a
`Catalog` via [`to_kindle()`](to_kindle.md), then aligns it into a `DataFrame` via
`tabelize()`, then writes that `DataFrame` to CSV.

```{important} Every field must be stream-compatible
Because the conversion goes through `to_kindle()`, every field (slot) must hold
values of a single, uniform kind — the same requirement `to_kindle()` itself
enforces. A registry with a genuinely mixed-type field (e.g. one row `Series`, the
next `Note`) throws before anything is written. [`save_json()`](save_json.md) is the
one exception — it falls back to `to_json()`'s per-value output for mixed fields
instead of throwing.
```

## Parameters

**filepath** (`std::string_view`)
    The destination file path.

**config** (`const pluss::loader::Config&`, optional)
    A generic configuration bundle, translated into the underlying CSV writer
    options.

## Returns

`void`
    Writes the file as a side effect; produces no return value.

## Examples

Example 1. Save a registry to CSV and read the file back to confirm its contents.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_csv("cities.csv");

std::ifstream saved("cities.csv");
std::cout << saved.rdbuf();
```

### Output

```text
id,city
1,Accra
2,Tema
3,Kumasi
```

Example 2. A field with genuinely mixed value kinds throws — the underlying
`to_kindle()` conversion cannot represent it as a uniform `Stream`.

```cpp
auto mixed = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"mixed", pluss::bundle::Slot({pluss::make_series(10), pluss::make_note("hello"), pluss::make_series(3.5)})},
});

try {
  mixed.save_csv("bad.csv");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Registry::save_as: slot 'mixed': slot is not stream-compatible (kind=Mixed)
```

```{important} The throw message says "save_as", not "save_csv"
Every `save_*` method except `save_json()` routes through the same internal
helper, which reports the error under the name `"Registry::save_as"` regardless of
which `save_*` method was actually called.
```

## Notes

- Requires every field to be stream-compatible — see the important callout above.
- Column order in the file follows the registry's own field order.
- [`save_as()`](save_as.md)`("csv", filepath)` is equivalent to this method's
  no-config overload.

## See Also

- [`Registry::load_csv`](load_csv.md): the read-side counterpart.
- [`Registry::save_as`](save_as.md): save to a format chosen by a runtime string,
  rather than a fixed method name.
- [`Registry::to_kindle`](to_kindle.md): the underlying conversion every `save_*`
  method except `save_json()` performs first.
- [`Catalog::save_csv`](../catalog/save_csv.md): the equivalent saver on the kindle
  model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_as.html">
    <span class="label">Previous</span>
    <span class="title">save_as</span>
  </a>
  <a class="next-link" href="save_excel.html">
    <span class="label">Next</span>
    <span class="title">save_excel</span>
  </a>
</div>
