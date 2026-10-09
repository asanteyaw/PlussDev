<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 33cca46a8434 -->

# AtlasLabelAccessor::set

[Back to AtlasLabelAccessor API Reference](../../atlas_label_accessor_api_reference)

## Signature

```cpp
AtlasLabelAccessor::set(const Slate& labels) const -> Atlas
```

## What It Does

Returns a copy of the bound atlas with its depth-axis labels replaced by `labels` —
the depth-axis counterpart to
[`DataFrame::label().set()`](../dataframe-label/set.md), which replaces the case-axis
labels of a single frame instead.

## Parameters

**labels** (`const Slate&`)
    The new depth labels. Its length must equal `depth()`.

## Returns

`Atlas`
    A new atlas, identical in frame content, with `labels` as its depth-axis labels.

## Examples

Example 1. Set depth labels on an atlas that was constructed without any.

```cpp
auto q1 = pluss::table::DataFrame({
    {"sales", pluss::table::Slate(pluss::make_series({100.0, 120.0}))},
});
auto q2 = pluss::table::DataFrame({
    {"sales", pluss::table::Slate(pluss::make_series({110.0, 90.0}))},
});
auto atlas = pluss::table::Atlas({q1, q2});

std::cout << "has before: " << atlas.label().has() << "\n";

auto labeled = atlas.label().set(pluss::table::Slate(pluss::make_note({"jan", "feb"})));
std::cout << labeled.label().values();
```

### Output

```text
has before: 0
0  jan
1  feb
ptype: Note
```

## Notes

- The original atlas is left unchanged — `set()` returns a new `Atlas`, like every other
  method on this accessor.
- Passing a `labels` `Slate` whose length doesn't equal `depth()` throws (mirroring
  [`Atlas::relabel_depth`](../atlas/relabel_depth.md), which this accessor delegates to
  internally).

## See Also

- [`AtlasLabelAccessor::reset`](reset.md): the inverse — clear the depth labels back to
  the default integer sequence.
- [`AtlasLabelAccessor::range`](range.md): reset the labels to `0, 1, 2, ...` explicitly.
- [`Atlas::relabel_depth`](../atlas/relabel_depth.md): the underlying `Atlas` method this
  accessor wraps.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reset.html">
    <span class="label">Previous</span>
    <span class="title">reset</span>
  </a>
  <a class="next-link" href="slice.html">
    <span class="label">Next</span>
    <span class="title">slice</span>
  </a>
</div>
