<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3a280380f04b -->

# Catalog::kindelize

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
static Catalog Catalog::kindelize(const pluss::table::DataFrame& frame, const KindelizePolicy& policy = {})
```

```cpp
struct KindelizePolicy {
  bool attach_labels{true};
  bool materialize_label{false};
  std::string label_key{"_label"};
};
```

## What It Does

**A `static` factory method** — call it as `Catalog::kindelize(...)`, not
`catalog.kindelize(...)`. Converts a `DataFrame` into a `Catalog`, one stream per feature.
This is the inverse of [`tabelize()`](tabelize.md)/[`to_table()`](to_table.md), which go
the other direction (`Catalog` → `DataFrame`).

## Parameters

**frame** (`const pluss::table::DataFrame&`)
    The source frame. Must be defined.

**policy** (`const KindelizePolicy&`, optional)
    `attach_labels` (default `true`) carries the frame's case-label axis into each
    resulting stream's own label. `materialize_label` (default `false`) additionally adds
    the case-label axis as its own stream, named `label_key` (default `"_label"`).

## Returns

`Catalog`
    A new catalog with one stream per feature in `frame`.

## Examples

Example 1. Convert a `DataFrame` (round-tripped through [`tabelize()`](tabelize.md) here
for a self-contained example) back into a `Catalog`.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});
auto frame = ct.tabelize();

auto back = pluss::kindle::Catalog::kindelize(frame);
std::cout << back;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=2
keys: [id, city]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 3       | [1, 2, 3]                                        
city                     | Note         | Note             | 3       | [Accra, Tema, Kumasi]                            
========================================================================================================================
```

Example 2. `materialize_label = true` adds the frame's case-label axis as its own stream.

```cpp
auto ct = pluss::kindle::Catalog({
    "id"_k   = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});
auto frame = ct.tabelize();

pluss::kindle::KindelizePolicy policy;
policy.materialize_label = true;
auto out = pluss::kindle::Catalog::kindelize(frame, policy);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=3
keys: [id, city, _label]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 3       | [1, 2, 3]                                        
city                     | Note         | Note             | 3       | [Accra, Tema, Kumasi]                            
_label                   | Series       | Series[int64]    | 3       | [0, 1, 2]                                        
========================================================================================================================
```

Since the round-tripped frame had a default positional case-label axis, the materialized
`_label` stream is just `[0, 1, 2]` — this is more useful when the frame's label axis
carries meaningful values (e.g. after [`load_csv_with_key()`](load_csv_with_key.md)).

## Notes

- `Catalog::kindelize` is `static` — call it on the class, not an instance.
- `attach_labels` (default `true`) affects each resulting stream's own label metadata, not
  whether a visible `_label` stream appears — that's controlled separately by
  `materialize_label`.
- Requires a defined `frame`; throws otherwise.

## See Also

- [`Catalog::tabelize`](tabelize.md) / [`Catalog::to_table`](to_table.md): the inverse
  direction, `Catalog` → `DataFrame`.
- [`Catalog::load_csv_with_key`](load_csv_with_key.md): uses this same
  `materialize_label` mechanism internally to expose a loaded CSV's key column.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="keys.html">
    <span class="label">Previous</span>
    <span class="title">keys</span>
  </a>
  <a class="next-link" href="load.html">
    <span class="label">Next</span>
    <span class="title">load</span>
  </a>
</div>
