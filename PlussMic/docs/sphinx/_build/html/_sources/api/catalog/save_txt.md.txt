<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 175c8e0770c1 -->

# Catalog::save_txt

[Back to Catalog API Reference](../../catalog_api_reference)

## Signatures

```cpp
Catalog::save_txt(std::string_view filename) const -> void
Catalog::save_txt(std::string_view filename, const pluss::loader::Config& config) const -> void
```

## What It Does

Writes the catalog to a tab-separated text file — the same outer-align, positional
[`tabelize()`](tabelize.md) step [`save_csv()`](save_csv.md) uses, just written with tabs
instead of commas.

## Parameters

**filename** (`std::string_view`)
    The destination file path.

**config** (`const pluss::loader::Config&`, optional)
    A generic configuration scribble, translated into the underlying text writer options.

## Returns

`void`
    Writes the file as a side effect; produces no return value.

## Examples

Example 1. Save a catalog to a tab-separated text file and read it back.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_txt("cities.txt");

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

- Same ragged-catalog tolerance as [`save_csv()`](save_csv.md) — a shorter stream is
  padded with an empty cell rather than causing a throw.
- Column order in the file follows the catalog's own key order.

## See Also

- [`Catalog::load_txt`](load_txt.md): the read-side counterpart.
- [`Catalog::save_csv`](save_csv.md): the comma-delimited counterpart, sharing the same
  underlying alignment step.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_parquet.html">
    <span class="label">Previous</span>
    <span class="title">save_parquet</span>
  </a>
  <a class="next-link" href="select.html">
    <span class="label">Next</span>
    <span class="title">select</span>
  </a>
</div>
