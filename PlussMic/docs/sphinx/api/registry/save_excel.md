<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 7647e9d4f746 -->

# Registry::save_excel

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
void Registry::save_excel(std::string_view filepath) const
void Registry::save_excel(std::string_view filepath, const pluss::loader::Config& config) const
```

## What It Does

Writes the registry to an `.xlsx` workbook, following the same
[`kindelize()`](kindelize.md) → `tabelize()` → format-specific writer path as
[`save_csv()`](save_csv.md).

```{important} Excel support is an optional build feature (requires libzip)
`save_excel()` (and [`load_excel()`](load_excel.md)) require the library to have
been built with `libzip` available. In a build without it, `save_excel()` throws
`"save_excel: Excel support not built (libzip not available)"` — this is a
build-configuration issue, not something fixable by changing your call.
```

## Parameters

**filepath** (`std::string_view`)
    The destination `.xlsx` file path.

**config** (`const pluss::loader::Config&`, optional)
    A generic configuration bundle, translated into the underlying Excel writer
    options.

## Returns

`void`
    Writes the file as a side effect; produces no return value.

## Examples

Example 1. Attempting to save to `.xlsx` in a build without Excel support throws —
this is the exact message this documentation environment produces, since it was
built without `libzip`.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

try {
  rg.save_excel("cities.xlsx");
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
save_excel: Excel support not built (libzip not available)
```

## Notes

- Requires `libzip` at build time and every field to be stream-compatible — see the
  important callout above. Only the throw example above could actually be verified
  in this documentation environment; the intended call shape otherwise matches
  [`save_csv()`](save_csv.md).
- [`save_as()`](save_as.md)`("excel", filepath)` (or `"xlsx"`) is equivalent to this
  method's no-config overload.

## See Also

- [`Registry::load_excel`](load_excel.md): the read-side counterpart, with the same
  `libzip` requirement.
- [`Registry::save_csv`](save_csv.md): the equivalent saver with no optional build
  dependency.
- [`Catalog::save_excel`](../catalog/save_excel.md): the equivalent saver on the
  kindle model.

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
