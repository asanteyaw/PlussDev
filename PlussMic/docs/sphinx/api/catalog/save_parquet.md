<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a7860dd7ec8c -->

# Catalog::save_parquet

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
Catalog::save_parquet(std::string_view filename) const -> void
Catalog::save_parquet(std::string_view filename, const pluss::loader::Config& config) const -> void
```

## What It Does

Writes the catalog to a `.parquet` file — the same outer-align, positional
[`tabelize()`](tabelize.md) step every other `save_*` method (except
[`save_as()`](save_as.md)) uses.

```{important} Parquet support is an optional build feature (requires Arrow + Parquet C++)
Like [`load_parquet()`](load_parquet.md), `save_parquet()` requires the library to have
been built with Apache Arrow and Parquet C++ available. In a build without them, it throws
`"loader::save_parquet: built without parquet-cpp support (install Arrow + Parquet C++ and reconfigure)"`
on any call, regardless of the filename or options passed.
```

## Parameters

**filename** (`std::string_view`)
    The destination `.parquet` file path.

**config** (`const pluss::loader::Config&`, optional)
    A generic configuration bundle, translated into the underlying Parquet writer options.

## Returns

`void`
    Writes the file as a side effect; produces no return value.

## Examples

Example 1. Save a catalog to Parquet — the intended call shape, mirroring
[`save_csv()`](save_csv.md).

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_parquet("cities.parquet");
```

Example 2. In a build without Parquet support, the call throws — this is the exact
message this documentation environment produces, since it was built without
Arrow/Parquet C++.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});

try {
  ct.save_parquet("cities.parquet");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
loader::save_parquet: built without parquet-cpp support (install Arrow + Parquet C++ and reconfigure)
```

## Notes

- Requires Apache Arrow + Parquet C++ at build time — see the important callout above.
  Only Example 2's throw could actually be verified in this documentation environment.
- Same ragged-catalog tolerance as [`save_csv()`](save_csv.md), once the build supports
  Parquet at all.

## See Also

- [`Catalog::load_parquet`](load_parquet.md): the read-side counterpart, with the same
  Arrow/Parquet C++ requirement.
- [`Catalog::save_csv`](save_csv.md): the equivalent CSV writer, with no optional build
  dependency.

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
