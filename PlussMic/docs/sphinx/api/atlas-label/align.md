<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e4621124c9ca -->

# AtlasLabelAccessor::align

[Back to AtlasLabelAccessor API Reference](../../atlas_label_accessor_api_reference)

## Signature

```cpp
AtlasLabelAccessor::align(const Slate& labels) const -> Atlas
```

## What It Does

Returns a copy of the bound atlas whose frames are reordered (and subset) to match
`labels` exactly — the accessor form of
[`Atlas::align_depth`](../atlas/align_depth.md), which it delegates to internally.
Frames whose depth label doesn't appear in `labels` are dropped from the result.

```{important} Throws on a target label with no match — does not fill with NULL
Unlike [`Slate::align`](../slate/align.md) or
[`SlateLabelAccessor::align`](../slate-label/align.md), which fill unmatched target
positions with NULL, `AtlasLabelAccessor::align` (via
[`Atlas::align_depth`](../atlas/align_depth.md)) **throws** if `labels` contains a value
that isn't one of the atlas's current depth labels. There is no depth-axis equivalent of
"a row of nulls" — every depth position must correspond to a real frame.
```

## Parameters

**labels** (`const Slate&`)
    The target depth-label order. Every value in `labels` must already exist among the
    atlas's current depth labels.

## Returns

`Atlas`
    A new atlas containing exactly the frames named in `labels`, in that order.

## Examples

Example 1. Reorder and subset a 3-frame atlas down to 2, in a new order.

```cpp
auto q1 = pluss::table::DataFrame({
    {"sales", pluss::table::Slate(pluss::make_series({100.0, 120.0}))},
});
auto q2 = pluss::table::DataFrame({
    {"sales", pluss::table::Slate(pluss::make_series({110.0, 90.0}))},
});
auto q3 = pluss::table::DataFrame({
    {"sales", pluss::table::Slate(pluss::make_series({95.0, 130.0}))},
});
auto atlas = pluss::table::Atlas({q1, q2, q3}, pluss::table::Slate(pluss::make_note({"jan", "feb", "mar"})));

auto aligned = atlas.label().align(pluss::table::Slate(pluss::make_note({"mar", "jan"})));
std::cout << aligned.label().values() << "\n";
std::cout << "depth: " << aligned.depth();
```

### Output

```text
0  mar
1  jan
ptype: Note

depth: 2
```

Example 2. A target label absent from the current depth labels throws immediately.

```cpp
auto q1 = pluss::table::DataFrame({
    {"sales", pluss::table::Slate(pluss::make_series({100.0, 120.0}))},
});
auto q2 = pluss::table::DataFrame({
    {"sales", pluss::table::Slate(pluss::make_series({110.0, 90.0}))},
});
auto atlas = pluss::table::Atlas({q1, q2}, pluss::table::Slate(pluss::make_note({"jan", "feb"})));

try {
    auto aligned = atlas.label().align(pluss::table::Slate(pluss::make_note({"jan", "dec"})));
} catch (const std::exception& e) {
    std::cout << "threw: " << e.what();
}
```

### Output

```text
threw: Atlas::align_depth: label not found 'dec'
```

## Notes

- Read the important callout above before relying on this for a partial/outer alignment
  — it subsets down to exactly `labels`, and throws rather than padding with NULL for
  anything `labels` names that doesn't already exist.
- Directly delegates to [`Atlas::align_depth`](../atlas/align_depth.md); see that page
  for the exact throw wording.

## See Also

- [`Atlas::align_depth`](../atlas/align_depth.md): the underlying `Atlas` method this
  accessor wraps.
- [`AtlasLabelAccessor::sort`](sort.md): reorder frames into sorted order instead of a
  specific target order.
- [`AtlasLabelAccessor::find`](find.md): look up label positions without reordering or
  throwing on a miss.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="diff.html">
    <span class="label">Next</span>
    <span class="title">diff</span>
  </a>
</div>
