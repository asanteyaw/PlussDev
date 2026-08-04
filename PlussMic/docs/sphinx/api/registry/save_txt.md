<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a5d95d2177b7 -->

# Registry::save_txt

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
void Registry::save_txt(std::string_view filepath) const
void Registry::save_txt(std::string_view filepath, const pluss::loader::Config& config) const
```

## What It Does

Writes the registry to a delimited text file (tab-separated by default), following
the same [`to_kindle()`](to_kindle.md) → `tabelize()` → format-specific writer path
as [`save_csv()`](save_csv.md), just with a text-file-style default separator rather
than comma.

## Parameters

**filepath** (`std::string_view`)
    The destination file path.

**config** (`const pluss::loader::Config&`, optional)
    A generic configuration bundle, translated into the underlying text writer
    options — see [`save_csv()`](save_csv.md)'s important callout: every field must
    be stream-compatible, or this throws.

## Returns

`void`
    Writes the file as a side effect; produces no return value.

## Examples

Example 1. Save a registry to a tab-separated text file and read the file back to
confirm its contents.

```cpp
auto rg = pluss::bundle::Registry({
    {"id", pluss::bundle::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"city", pluss::bundle::Slot({pluss::make_note("Accra"), pluss::make_note("Tema"), pluss::make_note("Kumasi")})},
});

rg.save_txt("cities.txt");

std::ifstream saved("cities.txt");
std::cout << saved.rdbuf();
```

### Output

```text
id	city
1	Accra
2	Tema
3	Kumasi
```

## Notes

- Shares its writer engine with [`save_csv()`](save_csv.md), just with a
  text-file-style default separator rather than comma.
- Requires every field to be stream-compatible, same as `save_csv()` — see that
  page's important callout for the exact throw message and shape.
- [`save_as()`](save_as.md)`("txt", filepath)` (or `"text"`) is equivalent to this
  method's no-config overload.

## See Also

- [`Registry::load_txt`](load_txt.md): the read-side counterpart, sharing the same
  underlying parser.
- [`Registry::save_csv`](save_csv.md): the comma-delimited counterpart, sharing the
  same underlying writer.
- [`Catalog::save_txt`](../catalog/save_txt.md): the equivalent saver on the kindle
  model.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_parquet.html">
    <span class="label">Previous</span>
    <span class="title">save_parquet</span>
  </a>
  <a class="next-link" href="set.html">
    <span class="label">Next</span>
    <span class="title">select</span>
  </a>
</div>
