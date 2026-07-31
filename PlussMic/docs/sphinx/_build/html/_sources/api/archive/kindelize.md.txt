<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b1e31b7fa5d0 -->

# Archive::kindelize

[Back to Archive API Reference](../../archive_api_reference)

## Signature

```cpp
static Archive Archive::kindelize(const pluss::table::Atlas& atlas, const KindelizePolicy& policy = {})
```

```cpp
struct KindelizePolicy {
  bool attach_labels{true};
  bool materialize_label{false};
  std::string label_key{"_label"};
};
```

## What It Does

**A `static` factory method** — call it as `Archive::kindelize(...)`, not
`archive.kindelize(...)`. Converts a `pluss::table::Atlas` (a depth-stacked collection of
`DataFrame`s) into an `Archive`, one catalog per `DataFrame`, in the same depth order. This
is the inverse of [`tabelize()`](tabelize.md)/[`to_table()`](to_table.md), which go the
other direction (`Archive` → `Atlas`). Internally, each `DataFrame` is converted via
[`Catalog::kindelize()`](../catalog/kindelize.md), using the same `policy` for every one.

## Parameters

**atlas** (`const pluss::table::Atlas&`)
    The source atlas. Must be defined.

**policy** (`const KindelizePolicy&`, optional)
    `attach_labels` (default `true`) carries each `DataFrame`'s own case-label axis into
    its resulting catalog's streams. `materialize_label` (default `false`) additionally
    adds the case-label axis as its own stream, named `label_key` (default `"_label"`).
    Applied uniformly to every `DataFrame` in the atlas.

## Returns

`Archive`
    A new archive with one catalog per `DataFrame` in `atlas`, carrying over the atlas's
    own depth labels.

## Examples

Example 1. Convert a two-frame `Atlas` into an `Archive`.

```cpp
using namespace pluss::table;

auto df0 = DataFrame({
    {"id", Thread(pluss::make_series({1.0, 2.0}))},
});
auto df1 = DataFrame({
    {"id", Thread(pluss::make_series({3.0, 4.0}))},
});
auto atlas = Atlas({df0, df1}, Thread(pluss::make_note({"jan", "feb"})));

auto archive = pluss::kindle::Archive::kindelize(atlas);
std::cout << "depth: " << archive.depth() << "\n";
std::cout << "depth labels carried over: " << archive.label().has() << "\n";
std::cout << archive.catalog(0);
```

### Output

```text
depth: 2
depth labels carried over: 1
========================================================================================================================
kindle::Catalog  keys=1
keys: [id]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
id                       | Series       | Series[float64]  | 2       | [1, 2]                                           
========================================================================================================================
```

## Notes

- A `static` factory — call as `Archive::kindelize(atlas)`, not on an existing instance.
- The atlas's own depth labels are carried over onto the resulting archive's
  [`label()`](label.md).
- `policy` is applied uniformly to every `DataFrame` in the atlas via
  [`Catalog::kindelize()`](../catalog/kindelize.md) — see that page for the full field
  breakdown.

## See Also

- [`Archive::tabelize`](tabelize.md) / [`Archive::to_table`](to_table.md): the inverse
  conversion, from `Archive` back to `Atlas`.
- [`Catalog::kindelize`](../catalog/kindelize.md): the single-frame delegate this method
  applies to every `DataFrame` in the atlas.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="keys.html">
    <span class="label">Previous</span>
    <span class="title">keys</span>
  </a>
  <a class="next-link" href="label.html">
    <span class="label">Next</span>
    <span class="title">label</span>
  </a>
</div>
