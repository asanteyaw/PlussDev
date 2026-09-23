<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f1d063783034 -->

# Registry::to_json

[Back to Registry API Reference](../../registry_api_reference)

## Signature

```cpp
std::string Registry::to_json() const
```

## What It Does

Renders the registry as a JSON string with **per-value** type detail — for each
field, a `values` array listing every individual value's type (e.g.
`"Series[int64]"`, `"Note[numel=1]"`), in order. This is finer-grained than
[`to_string()`](to_string.md)/[`show()`](show.md), which only report a per-kind
*count* per field, not each value's own type.

```{important} Correctly types CSV-loaded integer columns, including the first row
A field loaded from a CSV column of plain integers reports `"Series[int64]"` for
**every** row, including the first one — even when that row's literal string is
`"1"` or `"0"`. Earlier versions of the per-cell type-inference path used during
CSV loading could misresolve an ambiguous literal like `"1"` toward `Series[bool]`
for a single-token cell; this is fixed, and integer columns type consistently
regardless of which row's value happens to look boolean-like. See
[`Registry::load_csv`](load_csv.md).
```

## Parameters

`None`

## Returns

`std::string`
    A JSON string with a `values` array per field, one entry per value.

## Examples

Example 1. A registry with a uniform-kind field and a genuinely mixed field —
each value's own type is listed individually.

```cpp
auto rg = pluss::scribble::Registry({
    {"id", pluss::scribble::Slot({pluss::make_series(1), pluss::make_series(2), pluss::make_series(3)})},
    {"mixed", pluss::scribble::Slot({pluss::make_series(10), pluss::make_note("hello"), pluss::make_series(3.5)})},
});

std::cout << rg.to_json();
```

### Output

```text
{
  "_type": "Registry",
  "defined": true,
  "nfields": 2,
  "fields": {
    "id": {
  "_type": "Slot",
  "defined": true,
  "size": 3,
  "values": [
    "Series[int64]",
    "Series[int64]",
    "Series[int64]"
  ]
},
    "mixed": {
  "_type": "Slot",
  "defined": true,
  "size": 3,
  "values": [
    "Series[int64]",
    "Note[numel=1]",
    "Series[float64]"
  ]
}
  }
}
```

## Notes

- Each field's embedded object is exactly [`Slot::to_json()`](../slot/to_json.md)'s
  own output, concatenated in — not independently re-derived.
- Every `id` value above reports `Series[int64]` consistently, including the
  first row — see the important callout above.
- On an undefined registry, returns `{ "_type": "Registry", "defined": false }`
  rather than throwing.

## See Also

- [`Registry::to_string`](to_string.md): the coarser, per-kind-count
  counterpart.
- [`Registry::load_csv`](load_csv.md): the loader whose per-cell type inference
  this method's important callout describes.
- [`Slot::to_json`](../slot/to_json.md): the per-field method this one embeds
  for every field.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tl.html">
    <span class="label">Previous</span>
    <span class="title">tl</span>
  </a>
  <a class="next-link" href="to_string.html">
    <span class="label">Next</span>
    <span class="title">to_string</span>
  </a>
</div>
