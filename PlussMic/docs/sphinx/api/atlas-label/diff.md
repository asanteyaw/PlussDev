<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1e87b63c48da -->

# AtlasLabelAccessor::diff

[Back to AtlasLabelAccessor API Reference](../../atlas_label_accessor_api_reference)

## Signature

```cpp
Slate AtlasLabelAccessor::diff(const Slate& other) const
```

## What It Does

Returns the set difference of the atlas's current depth labels minus `other` — the
labels present in the bound atlas but absent from `other`. A pure set operation: it
returns a `Slate` of labels, not a new `Atlas`, and does not touch the bound atlas's
frames.

## Parameters

**other** (`const Slate&`)
    The set of labels to subtract from the atlas's current depth labels.

## Returns

`Slate`
    The labels present in the bound atlas's depth labels but not in `other`, named
    `"diff"`.

## Examples

Example 1. Find which depth labels exist in one atlas but not another.

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

std::cout << a1.label().diff(a2.label().values());
```

### Output

```text
0  jan
Name: diff, ptype: Note
```

## Notes

- Direction matters: `a.label().diff(b)` is not the same as `b`-side-minus-`a`. Use
  [`symdiff()`](symdiff.md) for the symmetric (either-direction) difference.
- Pure set operation — returns a `Slate`, not an `Atlas`.

## See Also

- [`AtlasLabelAccessor::symdiff`](symdiff.md): the symmetric difference — labels in
  either set but not both.
- [`AtlasLabelAccessor::intersect`](intersect.md): the complement of this
  operation — labels present in both sets.
- [`AtlasLabelAccessor::unite`](unite.md): the labels present in either set.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="align.html">
    <span class="label">Previous</span>
    <span class="title">align</span>
  </a>
  <a class="next-link" href="find.html">
    <span class="label">Next</span>
    <span class="title">find</span>
  </a>
</div>
