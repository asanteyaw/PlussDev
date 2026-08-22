<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 76920d3555fe -->

# Catalog::save_csv

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
Catalog::save_csv(std::string_view filename) const -> void
Catalog::save_csv(std::string_view filename, const pluss::loader::Config& config) const -> void
```

## What It Does

Writes the catalog to a CSV file. Internally, every `save_*` method (except
[`save_as()`](save_as.md)) first aligns the catalog's streams into a `DataFrame` via
[`tabelize()`](tabelize.md) with an **outer**, positional alignment policy — ragged
streams (different lengths) are padded with `NULL` rather than truncated or rejected —
then calls the corresponding `DataFrame::save_csv`.

## Parameters

**filename** (`std::string_view`)
    The destination file path.

**config** (`const pluss::loader::Config&`, optional)
    A generic configuration scribble, translated into the underlying CSV writer options.

## Returns

`void`
    Writes the file as a side effect; produces no return value.

## Examples

Example 1. Save a catalog to CSV and read the file back to confirm its contents.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_csv("cities.csv");

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

Example 2. Saving a **ragged** catalog (streams of different lengths) pads the shorter
stream with an empty (`NULL`) cell, rather than truncating or throwing.

```cpp
auto ragged = pluss::kindle::Catalog({
    "a"_k = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "b"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0})),
});

ragged.save_csv("ragged.csv");

std::ifstream saved("ragged.csv");
std::cout << saved.rdbuf();
```

### Output

```text
a,b
1,10
2,20
3,
```

## Notes

- Uses an outer-align, positional [`tabelize()`](tabelize.md) internally, so a ragged
  catalog saves successfully with `NULL`-padded cells rather than throwing — see
  Example 2.
- Column order in the file follows the catalog's own key order.

## See Also

- [`Catalog::load_csv`](load_csv.md): the read-side counterpart.
- [`Catalog::save_as`](save_as.md): save to a format chosen by a runtime string, rather
  than a fixed method name.
- [`Catalog::tabelize`](tabelize.md): the underlying alignment step every `save_*` method
  (except `save_as`) performs first.

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
