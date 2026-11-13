<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d37344265293 -->

# DataFrame::nfeatures

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
std::int64_t DataFrame::nfeatures() const
```

## What It Does

Returns the number of features (columns) in the frame. This is the second number in a
printed frame's `[N cs x M feat]` footer, and the length every feature-aligned
`Slate` argument (the `Axis::Feature` side of [`add()`](add.md), [`eq()`](eq.md), and
similar methods) is checked against.

## Parameters

`None`

## Returns

`std::int64_t`
    The number of features in the frame.

## Examples

Example 1. Read the feature count directly.

```cpp
using namespace pluss::table;

auto scores = DataFrame({
    {"quiz",  pluss::make_series({84.0, 90.0, 84.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out = scores.nfeatures();
std::cout << out;
```

### Output

```text
2
```

This matches the `2` in `scores`'s own `[3 cs x 2 feat]` footer when printed, and the
length of [`features()`](features.md)'s returned name list.

## Notes

- `nfeatures()` always equals `features().size()` — the two are kept in sync by
  construction.
- Every case (row) in the frame has exactly `nfeatures()` cells; this is an invariant
  the frame maintains internally.

## See Also

- [`DataFrame::ncases`](ncases.md): the companion count for rows instead of columns.
- [`DataFrame::shape`](shape.md): both counts together as one struct.
- [`DataFrame::features`](features.md): the actual ordered name list this count
  matches the length of.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ne.html">
    <span class="label">Previous</span>
    <span class="title">ne</span>
  </a>
  <a class="next-link" href="note.html">
    <span class="label">Next</span>
    <span class="title">note</span>
  </a>
</div>
