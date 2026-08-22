<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6333b43aa253 -->

# Catalog::save_excel

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
Catalog::save_excel(std::string_view filename) const -> void
Catalog::save_excel(std::string_view filename, const pluss::loader::Config& config) const -> void
```

## What It Does

Writes the catalog to an `.xlsx` workbook — the same outer-align, positional
[`tabelize()`](tabelize.md) step every other `save_*` method (except
[`save_as()`](save_as.md)) uses.

```{important} Excel support is an optional build feature (requires libzip)
Like [`load_excel()`](load_excel.md), `save_excel()` requires the library to have been
built with `libzip` available. In a build without it, it throws
`"save_excel: Excel support not built (libzip not available)"` on any call, regardless of
the filename or options passed.
```

## Parameters

**filename** (`std::string_view`)
    The destination `.xlsx` file path.

**config** (`const pluss::loader::Config&`, optional)
    A generic configuration scribble, translated into the underlying Excel writer options.

## Returns

`void`
    Writes the file as a side effect; produces no return value.

## Examples

Example 1. Save a catalog to an Excel workbook — the intended call shape, mirroring
[`save_csv()`](save_csv.md).

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_excel("cities.xlsx");
```

Example 2. In a build without Excel support, the call throws — this is the exact message
this documentation environment produces, since it was built without `libzip`.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
});

try {
  ct.save_excel("cities.xlsx");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
save_excel: Excel support not built (libzip not available)
```

## Notes

- Requires `libzip` at build time — see the important callout above. Only Example 2's
  throw could actually be verified in this documentation environment.
- Same ragged-catalog tolerance as [`save_csv()`](save_csv.md), once the build supports
  Excel at all.

## See Also

- [`Catalog::load_excel`](load_excel.md): the read-side counterpart, with the same
  `libzip` requirement.
- [`Catalog::save_csv`](save_csv.md): the equivalent CSV writer, with no optional build
  dependency.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_csv.html">
    <span class="label">Previous</span>
    <span class="title">save_csv</span>
  </a>
  <a class="next-link" href="save_json.html">
    <span class="label">Next</span>
    <span class="title">save_json</span>
  </a>
</div>
