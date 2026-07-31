<!-- pluss-hand-authored -->

# Registry::load_parquet

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
static Registry Registry::load_parquet(std::string_view filepath)
static Registry Registry::load_parquet(std::string_view filepath, const pluss::loader::Config& config)
```

## What It Does

**A `static` factory method** — loads a `.parquet` file into a new `Registry`, one
field (slot) per column. Parquet already carries per-column types, so this generally
preserves dtypes more precisely than the text-based loaders
([`load_csv()`](load_csv.md)/[`load_txt()`](load_txt.md)/[`load_json()`](load_json.md)),
which have to infer them.

```{important} Parquet support is an optional build feature (requires Arrow + Parquet C++)
`load_parquet()` (and `save_parquet()`) require the library to have been built with
Apache Arrow and Parquet C++ available. In a build without them, both throw
`"...: built without parquet-cpp support (install Arrow + Parquet C++ and reconfigure)"`
— but only once the file itself is found; a nonexistent path throws its own
`"Registry::load_parquet: file not found '<path>'"` first, checked before the
build-support check.
```

## Parameters

**filepath** (`std::string_view`)
    The `.parquet` file to read.

**config** (`const pluss::loader::Config&`, optional)
    A generic configuration bundle, translated into the underlying reader options.

## Returns

`Registry`
    A new registry with one field per column.

## Examples

Example 1. Attempting to load an existing `.parquet` file in a build without Parquet
support throws — this is the exact message this documentation environment produces,
since it was built without Arrow/Parquet C++.

```cpp
try {
  auto rg = pluss::bundle::Registry::load_parquet("cities.parquet");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
loader::read_parquet_raw_batch: built without parquet-cpp support (install Arrow + Parquet C++ and reconfigure)
```

Example 2. A path that doesn't exist at all throws a distinct, earlier message —
checked before the build-support requirement.

```cpp
try {
  auto rg = pluss::bundle::Registry::load_parquet("does_not_exist.parquet");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Registry::load_parquet: file not found 'does_not_exist.parquet'
```

## Notes

- Requires Apache Arrow + Parquet C++ at build time — see the important callout
  above. Only the two throw examples above could actually be verified in this
  documentation environment.
- File existence is checked before the build-support check — a missing file always
  throws the "file not found" message, even in a build with Parquet support.
- Parquet's own column types generally survive the load more precisely than
  text-format loaders' inferred types.

## See Also

- [`Registry::load_csv`](load_csv.md): the equivalent CSV loader, with no optional
  build dependency.
- [`Catalog::load_parquet`](../catalog/load_parquet.md): the equivalent loader on the
  kindle model, with the same Arrow/Parquet C++ requirement.
- [`Registry::save_parquet`](save_parquet.md): the write-side counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_json.html">
    <span class="label">Previous</span>
    <span class="title">load_json</span>
  </a>
  <a class="next-link" href="load_txt.html">
    <span class="label">Next</span>
    <span class="title">load_txt</span>
  </a>
</div>
