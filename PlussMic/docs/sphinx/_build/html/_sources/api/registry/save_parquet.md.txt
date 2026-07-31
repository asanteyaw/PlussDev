<!-- pluss-hand-authored -->

# Registry::save_parquet

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
void Registry::save_parquet(std::string_view filepath) const
void Registry::save_parquet(std::string_view filepath, const pluss::loader::Config& config) const
```

## What It Does

Writes the registry to a `.parquet` file, following the same
[`to_kindle()`](to_kindle.md) → `tabelize()` → format-specific writer path as
[`save_csv()`](save_csv.md).

```{important} Parquet support is an optional build feature (requires Arrow + Parquet C++)
`save_parquet()` (and [`load_parquet()`](load_parquet.md)) require the library to
have been built with Apache Arrow and Parquet C++ available. In a build without
them, `save_parquet()` throws
`"loader::save_parquet: built without parquet-cpp support (install Arrow + Parquet C++ and reconfigure)"`.
```

## Parameters

**filepath** (`std::string_view`)
    The destination `.parquet` file path.

**config** (`const pluss::loader::Config&`, optional)
    A generic configuration bundle, translated into the underlying Parquet writer
    options.

## Returns

`void`
    Writes the file as a side effect; produces no return value.

## Examples

Example 1. Attempting to save to `.parquet` in a build without Parquet support
throws — this is the exact message this documentation environment produces, since
it was built without Arrow/Parquet C++.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

try {
  rg.save_parquet("cities.parquet");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
loader::save_parquet: built without parquet-cpp support (install Arrow + Parquet C++ and reconfigure)
```

## Notes

- Requires Apache Arrow + Parquet C++ at build time and every field to be
  stream-compatible — see the important callout above. Only the throw example above
  could actually be verified in this documentation environment; the intended call
  shape otherwise matches [`save_csv()`](save_csv.md).
- [`save_as()`](save_as.md)`("parquet", filepath)` is equivalent to this method's
  no-config overload.

## See Also

- [`Registry::load_parquet`](load_parquet.md): the read-side counterpart, with the
  same Arrow/Parquet C++ requirement.
- [`Registry::save_csv`](save_csv.md): the equivalent saver with no optional build
  dependency.
- [`Catalog::save_parquet`](../catalog/save_parquet.md): the equivalent saver on the
  kindle model.

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
