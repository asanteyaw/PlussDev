<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: abe53e190dd6 -->

# Atlas::align_depth

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::align_depth(const Slate& labels) const -> Atlas
```

## What It Does

Returns a new atlas with its frames reordered/filtered to match `labels`, by depth label —
every entry in `labels` must exist among the atlas's own depth labels, or the call throws.
The `Atlas` analog of [`Archive::label`](../archive/label.md)`.align()`.

```{important} Throws on any unmatched target label — it does not fill with NULL
Unlike `Slate::align`/`Stream::lbl().align()` (which fill unmatched positions with `NULL`
or a supplied `fill` value), `align_depth()` requires every label in `labels` to exist
among the atlas's own depth labels; a target label with no match throws
`"Atlas::align_depth: label not found '<label>'"`.
```

## Parameters

**labels** (`const Slate&`)
    The target depth-label order. Every entry must exist among the atlas's own depth
    labels.

## Returns

`Atlas`
    A new atlas with frames reordered to match `labels`' order.

## Examples

Example 1. Reorder frames to an explicit target label order.

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({1.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({2.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1}, pluss::table::Slate(pluss::make_note({"b", "a"})));

auto out = atlas.align_depth(pluss::table::Slate(pluss::make_note({"a", "b"})));
std::cout << out.dloc(0);
```

### Output

```text
===========
     | id  
-----------
0    | 2   
-----------
[1 cs x 1 feat]
```

Example 2. An unmatched target label throws instead of producing a `NULL` frame.

```cpp
auto f0 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({1.0}))},
});
auto f1 = pluss::table::DataFrame({
    {"id", pluss::table::Slate(pluss::make_series({2.0}))},
});
auto atlas = pluss::table::Atlas({f0, f1}, pluss::table::Slate(pluss::make_note({"b", "a"})));

try {
  atlas.align_depth(pluss::table::Slate(pluss::make_note({"a", "c"})));
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Atlas::align_depth: label not found 'c'
```

## Notes

- Throws on any unmatched target label — see the important callout above.
- Requires depth labels to already be set on the atlas.

## See Also

- [`Atlas::sort_depth`](sort_depth.md): reorder by the existing label order, instead of an
  explicit target.
- [`Atlas::relabel_depth`](relabel_depth.md): replace labels without reordering frames.
- [`Archive::label`](../archive/label.md)`.align()`: the depth-stacked-catalog analog of
  this method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="aggregate_over_frames.html">
    <span class="label">Previous</span>
    <span class="title">aggregate_over_frames</span>
  </a>
  <a class="next-link" href="axis0_labels.html">
    <span class="label">Next</span>
    <span class="title">axis0_labels</span>
  </a>
</div>
