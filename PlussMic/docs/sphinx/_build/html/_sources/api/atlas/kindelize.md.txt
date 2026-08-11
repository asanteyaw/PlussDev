<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 05ca394cae3e -->

# Atlas::kindelize

[Back to Atlas API Reference](../../atlas_api_reference)

## Signatures

```cpp
Atlas::kindelize() const -> pluss::kindle::Archive
Atlas::kindelize(const pluss::kindle::KindelizePolicy& policy) const -> pluss::kindle::Archive
```

## What It Does

Converts the atlas into an `Archive`, one catalog per frame, in the same depth order — the
inverse of [`Archive::tabelize()`](../archive/tabelize.md), which goes the other direction
(`Archive` → `Atlas`). Internally, each frame is converted via
[`DataFrame::kindelize()`](../dataframe/kindelize.md), using the same `policy` for every one,
and the atlas's own depth labels are carried over.

## Parameters

**policy** (`const pluss::kindle::KindelizePolicy&`, optional)
    `attach_labels` (default `true`) carries each frame's own case-label axis into its
    resulting catalog's streams. `materialize_label` (default `false`) additionally adds
    the case-label axis as its own stream, named `label_key` (default `"_label"`). See
    [`DataFrame::kindelize`](../dataframe/kindelize.md) for the full field breakdown.

## Returns

`pluss::kindle::Archive`
    A new archive with one catalog per frame, carrying over the atlas's own depth labels.

## Examples

Example 1. Convert a two-frame atlas into an `Archive`.

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1.0, 2.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({3.0, 4.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1}, pluss::table::Thread(pluss::make_note({"jan", "feb"})));

auto archive = atlas.kindelize();
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

- The atlas's own depth labels are carried over onto the resulting archive's
  [`label()`](label.md).
- `policy` is applied uniformly to every frame via
  [`DataFrame::kindelize()`](../dataframe/kindelize.md) — see that page for the full field
  breakdown.
- Round-trips with [`Archive::tabelize()`](../archive/tabelize.md):
  `atlas.kindelize().tabelize()` reconstructs an equivalent atlas.

## See Also

- [`Archive::tabelize`](../archive/tabelize.md): the inverse conversion, from `Archive`
  back to `Atlas`.
- [`DataFrame::kindelize`](../dataframe/kindelize.md): the single-frame delegate this method
  applies to every frame.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="head.html">
    <span class="label">Previous</span>
    <span class="title">head</span>
  </a>
  <a class="next-link" href="label.html">
    <span class="label">Next</span>
    <span class="title">label</span>
  </a>
</div>
