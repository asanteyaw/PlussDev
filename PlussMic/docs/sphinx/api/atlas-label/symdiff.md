<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d9dfbce0d97e -->

# AtlasLabelAccessor::symdiff

[Back to AtlasLabelAccessor API Reference](../../atlas_label_accessor_api_reference)

## Signature

```cpp
Slate AtlasLabelAccessor::symdiff(const Slate& other) const
```

## What It Does

Returns the symmetric set difference of the atlas's current depth labels and `other` —
labels present in exactly one of the two sets, excluding anything present in both. A
pure set operation: it returns a `Slate` of labels, not a new `Atlas`, and does not
touch the bound atlas's frames.

## Parameters

**other** (`const Slate&`)
    The other set of labels to compute the symmetric difference against.

## Returns

`Slate`
    The labels present in exactly one of the two sets, named `"symdiff"`.

## Examples

Example 1. Find the depth labels that are unique to one atlas or the other.

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

std::cout << a1.label().symdiff(a2.label().values());
```

### Output

```text
0  jan
1  apr
Name: symdiff, ptype: Note
```

`"jan"` (unique to `a1`) and `"apr"` (unique to `a2`) both appear; `"feb"` and `"mar"`
(shared by both) are excluded.

## Notes

- Unlike [`diff()`](diff.md), direction doesn't matter — `a.label().symdiff(b)` and the
  equivalent call the other direction produce the same set of labels (order may differ).
- Pure set operation — returns a `Slate`, not an `Atlas`.

## See Also

- [`AtlasLabelAccessor::diff`](diff.md): the one-directional (asymmetric) difference.
- [`AtlasLabelAccessor::intersect`](intersect.md): the complement of this
  operation — labels present in both sets.
- [`AtlasLabelAccessor::unite`](unite.md): the labels present in either set, including
  the ones this method excludes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sort.html">
    <span class="label">Previous</span>
    <span class="title">sort</span>
  </a>
  <a class="next-link" href="unite.html">
    <span class="label">Next</span>
    <span class="title">unite</span>
  </a>
</div>
