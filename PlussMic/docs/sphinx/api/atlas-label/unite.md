<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 63ce27fb8892 -->

# AtlasLabelAccessor::unite

[Back to AtlasLabelAccessor API Reference](../../atlas_label_accessor_api_reference)

## Signature

```cpp
AtlasLabelAccessor::unite(const Slate& other) const -> Slate
```

## What It Does

Returns the set union of the atlas's current depth labels and `other` — every distinct
label that appears in either, with duplicates collapsed. This is a pure set operation:
it returns a `Slate` of labels, not a new `Atlas`, and does not touch the bound atlas's
frames.

## Parameters

**other** (`const Slate&`)
    The other set of labels to union with the atlas's current depth labels.

## Returns

`Slate`
    The union of the two label sets, named `"unite"`.

## Examples

Example 1. Union the depth labels of two atlases that only partially overlap.

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
auto a1 = pluss::table::Atlas({q1, q2, q3}, pluss::table::Slate(pluss::make_note({"jan", "feb", "mar"})));
auto a2 = pluss::table::Atlas({q1, q2, q3}, pluss::table::Slate(pluss::make_note({"feb", "mar", "apr"})));

std::cout << a1.label().unite(a2.label().values());
```

### Output

```text
0  jan
1  feb
2  mar
3  apr
Name: unite, ptype: Note
```

## Notes

- Pure set operation — returns a `Slate`, not an `Atlas`; it doesn't select or reorder
  the bound atlas's own frames.
- The result is named `"unite"` regardless of the input `Slate`'s own name.

## See Also

- [`AtlasLabelAccessor::intersect`](intersect.md): the labels present in both sets,
  rather than either.
- [`AtlasLabelAccessor::diff`](diff.md) / [`AtlasLabelAccessor::symdiff`](symdiff.md):
  the one-sided and two-sided complements of this union.
- [`AtlasLabelAccessor::align`](align.md): apply a target label order to the bound
  atlas's frames, rather than just computing a label set.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="symdiff.html">
    <span class="label">Previous</span>
    <span class="title">symdiff</span>
  </a>
  <a class="next-link" href="values.html">
    <span class="label">Next</span>
    <span class="title">values</span>
  </a>
</div>
