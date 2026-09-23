<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d68e2d55f873 -->

# Catalog::load_parquet

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
static Catalog Catalog::load_parquet(std::string_view filepath)
static Catalog Catalog::load_parquet(std::string_view filepath, const pluss::loader::Config& config)
```

## What It Does

Loads a `.parquet` file into a new `Catalog`, one stream per column — Parquet already
carries per-column types, so this generally preserves dtypes more precisely than the
text-based loaders ([`load_csv()`](load_csv.md)/[`load_txt()`](load_txt.md)/
[`load_json()`](load_json.md)), which have to infer them.

```{important} Parquet support is an optional build feature (requires Arrow + Parquet C++)
`load_parquet()` (and `save_parquet()`) require the library to have been built with Apache
Arrow and Parquet C++ available. In a build without them, both throw
`"...: built without parquet-cpp support (install Arrow + Parquet C++ and reconfigure)"` on
any call, regardless of the file path or options passed — this is a build-configuration
issue, not something fixable by changing your call. If you hit this, rebuild PlussMic with
Arrow + Parquet C++ available.
```

## Parameters

**filepath** (`std::string_view`)
    The `.parquet` file to read.

**config** (`const pluss::loader::Config&`, optional)
    A generic configuration scribble, translated into the underlying reader options.

## Returns

`Catalog`
    A new catalog with one stream per column.

## Examples

Example 1. Load a Parquet file — the same call shape as [`load_csv()`](load_csv.md), just
for `.parquet` files.

```cpp
auto ct = pluss::kindle::Catalog::load_parquet("cities.parquet");
std::cout << ct.nkeys();
```

Example 2. Attempting to load a `.parquet` file in a build without Parquet support
throws — this is the exact message this documentation environment produces, since it was
built without Arrow/Parquet C++.

```cpp
try {
  auto ct = pluss::kindle::Catalog::load_parquet("cities.parquet");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
loader::read_parquet_raw_batch: built without parquet-cpp support (install Arrow + Parquet C++ and reconfigure)
```

## Notes

- Requires Apache Arrow + Parquet C++ at build time — see the important callout above.
  Example 1 shows the intended call shape; only Example 2's throw could actually be
  verified in this documentation environment.
- Parquet's own column types generally survive the load more precisely than text-format
  loaders' inferred types.

## See Also

- [`Catalog::load_csv`](load_csv.md): the equivalent CSV loader, with no optional build
  dependency.
- [`Catalog::save_parquet`](save_parquet.md): the write-side counterpart, with the same
  Arrow/Parquet C++ requirement.

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
