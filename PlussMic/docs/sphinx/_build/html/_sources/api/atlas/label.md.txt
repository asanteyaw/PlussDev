<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 70eff7ab2f20 -->

# Atlas::label

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::label() const -> AtlasLabelAccessor
```

## What It Does

The entry point into the atlas's **depth labels** — an optional per-frame label axis, set
via the constructor's `axis0_labels` parameter or via `label().set(...)`. `label()` binds
the current atlas and hands back an `AtlasLabelAccessor`, documented in full on its own
pages (see [`AtlasLabelAccessor` API Reference](../../atlas_label_accessor_api_reference)):
`has()`, `values()`, `set()`, `range()`, `reset()`, `sort()`, `align()`, plus set-algebra
and lookup helpers (`unite()`, `intersect()`, `diff()`, `symdiff()`, `find()`, `pos()`,
`slice()`).

## Parameters

`None`

## Returns

`AtlasLabelAccessor`
    A binder for the current atlas — call one of its methods to inspect, set, reorder, or
    query its depth labels.

## Examples

Example 1. Check and read back the depth labels via the accessor.

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1.0}))},
});
auto atlas = pluss::table::Atlas({f0}, pluss::table::Thread(pluss::make_note({"jan"})));

std::cout << "has(): " << atlas.label().has() << "\n";
std::cout << atlas.label().values();
```

### Output

```text
has(): 1
0  jan
ptype: Note
```

## Notes

- `label()` alone does no work — it only binds the atlas; call one of its methods to
  actually inspect, set, reorder, or query.
- The full accessor surface is documented separately — see
  [`AtlasLabelAccessor` API Reference](../../atlas_label_accessor_api_reference).

## See Also

- [`Atlas::sort_depth`](sort_depth.md) / [`Atlas::relabel_depth`](relabel_depth.md) /
  [`Atlas::align_depth`](align_depth.md): direct top-level shorthands for the accessor's own
  `sort()`/`set()`/`align()`.
- [`Archive::label`](../archive/label.md): the depth-stacked-catalog analog of this method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kindelize.html">
    <span class="label">Previous</span>
    <span class="title">kindelize</span>
  </a>
  <a class="next-link" href="loc.html">
    <span class="label">Next</span>
    <span class="title">loc</span>
  </a>
</div>
