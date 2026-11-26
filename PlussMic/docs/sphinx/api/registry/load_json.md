<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1bcb93fd1da8 -->

# Registry::load_json

[Back to Registry API Reference](../../registry_api_reference)

## Signatures

```cpp
static Registry Registry::load_json(std::string_view filepath)
static Registry Registry::load_json(std::string_view filepath, const pluss::loader::Config& config)
```

## What It Does

**A `static` factory method** — loads a JSON file (an array of flat objects) into a
new `Registry`, one field (slot) per distinct key across the objects, with per-column
dtype inference — the same underlying inference [`load_csv()`](load_csv.md) uses,
applied to the JSON values instead of parsed CSV text.

## Parameters

**filepath** (`std::string_view`)

The JSON file to read. Expected shape: a top-level array of flat (non-nested)
objects.

**config** (`const pluss::loader::Config&`, optional)

A generic configuration scribble, translated into the underlying reader options.

## Returns

`Registry`
    A new registry with one field per distinct key found across the JSON objects.

## Examples

Example 1. Load a JSON array of objects.

Given `cities.json`:

```text
[{"city": "Accra", "population": 2500000}, {"city": "Tema", "population": 161000}, {"city": "Kumasi", "population": 3000000}]
```

```cpp
auto rg = pluss::scribble::Registry::load_json("cities.json");
std::cout << rg;
```

### Output

```text
========================================================================================================================
scribble::Registry  fields=2
states=3
------------------------------------------------------------------------------------------------------------------------
field      | ptype  | states | preview
------------------------------------------------------------------------------------------------------------------------
city       | Note   | 3      | [Accra, Tema, Kumasi]
population | Series | 3      | [2500000, 161000, 3000000]
========================================================================================================================
```

## Notes

- Expects a flat array-of-objects JSON shape.
- Column dtype is inferred per-column from its values.
- [`load()`](load.md)`("json", filepath)` is equivalent to this method's no-config
  overload.

## See Also

- [`Registry::load`](load.md): the format-dispatching entry point this method backs.
- [`Catalog::load_json`](../catalog/load_json.md): the equivalent loader on the
  kindle model.
- [`Registry::save_json`](save_json.md): the write-side counterpart.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_excel.html">
    <span class="label">Previous</span>
    <span class="title">load_excel</span>
  </a>
  <a class="next-link" href="load_parquet.html">
    <span class="label">Next</span>
    <span class="title">load_parquet</span>
  </a>
</div>
