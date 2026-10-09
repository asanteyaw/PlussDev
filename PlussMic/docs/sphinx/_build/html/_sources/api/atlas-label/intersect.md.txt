<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 370efc02a533 -->

# AtlasLabelAccessor::intersect

[Back to AtlasLabelAccessor API Reference](../../atlas_label_accessor_api_reference)

## Signature

```cpp
AtlasLabelAccessor::intersect(const Slate& other) const -> Slate
```

## What It Does

Returns the set intersection of the atlas's current depth labels and `other` — only the
labels that appear in both. A pure set operation: it returns a `Slate` of labels, not a
new `Atlas`, and does not touch the bound atlas's frames.

## Parameters

**other** (`const Slate&`)
    The other set of labels to intersect with the atlas's current depth labels.

## Returns

`Slate`
    The labels present in both sets, named `"intersect"`.

## Examples

Example 1. Intersect the depth labels of two atlases that only partially overlap.

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

std::cout << a1.label().intersect(a2.label().values());
```

### Output

```text
0  feb
1  mar
Name: intersect, ptype: Note
```

## Notes

- Pure set operation — returns a `Slate`, not an `Atlas`; combine with
  [`align()`](align.md) if you need to actually subset the bound atlas's frames down to
  this intersection.
- The result is named `"intersect"` regardless of the input `Slate`'s own name.

## See Also

- [`AtlasLabelAccessor::unite`](unite.md): the labels present in either set.
- [`AtlasLabelAccessor::diff`](diff.md) / [`AtlasLabelAccessor::symdiff`](symdiff.md):
  the complements of this intersection.
- [`AtlasLabelAccessor::align`](align.md): subset the bound atlas's frames to a specific
  label set, such as the result of this method.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has.html">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="pos.html">
    <span class="label">Next</span>
    <span class="title">pos</span>
  </a>
</div>
