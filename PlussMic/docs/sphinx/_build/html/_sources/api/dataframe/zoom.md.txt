<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e9067bf84f98 -->

# DataFrame::zoom

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::zoom(std::int64_t case_start,
                std::int64_t case_stop,
                std::int64_t feature_start,
                std::int64_t feature_stop) const -> DataFrame
```

## What It Does

Carves out a positional rectangular block from the frame: a contiguous case range and
a contiguous feature range at the same time, returning the smaller block as a new
`DataFrame`. If you think of a `DataFrame` as a labeled matrix, `zoom()` is the
direct "give me this rectangle" convenience.

Semantically, `zoom()` is the positional shorthand for:

```cpp
df.case_study().slice(case_start, case_stop)
  .features(selected_feature_names)
```

So it is intentionally positional and half-open on both axes. If you need label-based
selection or a non-rectangular pick, reach for [`case_study()`](case_study.md)
instead.

The important distinction is that `zoom()` is not trying to be a second general
indexing language. It is the compact "rectangular block" helper:

- one contiguous case interval
- one contiguous feature interval
- one resulting smaller frame

That makes it especially useful when you are visually inspecting a large table,
debugging a generated frame, or preparing a focused block for display/export.

## Parameters

**case_start**, **case_stop** (`std::int64_t`)

The contiguous case block `[case_start, case_stop)`. Negative values follow the same
Python-style normalization as [`slice()`](slice.md).

**feature_start**, **feature_stop** (`std::int64_t`)

The contiguous feature block `[feature_start, feature_stop)`. Negative values are
normalized relative to the end of the feature axis the same way
the case-study accessor's positional slice does on the row axis.

## Returns

`DataFrame`
    A new frame containing only the requested rectangular block, preserving the
    original case labels and feature order inside that block.

## When To Use It

Use `zoom()` when the thing you want is specifically a rectangular positional block.

That usually happens in three situations:

- you are visually inspecting a large frame and want to focus on one local region
- you are debugging generated data and need a compact "window" into the middle or tail
  of a table
- you want a matrix-like block extraction without selecting cases and then resolving
  the corresponding feature names yourself

If your selection is not rectangular, not positional, or not contiguous, then
`zoom()` is the wrong tool. In that case, `case_study()`, `slice()`, `features()`, or
label-driven selection will be clearer.

## Examples

Example 1. Extract a middle rectangle from a labeled frame.

```cpp
using namespace pluss::table;

auto grid = DataFrame({
    {"w", pluss::make_series(std::vector<std::int64_t>{1, 2, 3, 4})},
    {"x", pluss::make_series(std::vector<std::int64_t>{5, 6, 7, 8})},
    {"y", pluss::make_series(std::vector<std::int64_t>{9, 10, 11, 12})},
    {"z", pluss::make_series(std::vector<std::int64_t>{13, 14, 15, 16})},
}).label().set(pluss::make_note({"r0", "r1", "r2", "r3"}));

auto out = grid.zoom(1, 3, 1, 4);
std::cout << out;
```

### Output

```text
================
     | x   y   z
----------------
r1   | 6   10  14
r2   | 7   11  15
----------------
[2 cs x 3 feat]
```

`zoom(1, 3, 1, 4)` means:

- cases `[1, 3)` -> rows `1` and `2`
- features `[1, 4)` -> columns `x`, `y`, `z`

Both axes are half-open, so case `3` and any feature at position `4` are excluded.

Example 2. Use negative indices to zoom relative to the end on both axes.

```cpp
auto tail_block = grid.zoom(-2, 4, -3, -1);
std::cout << tail_block;
```

### Output

```text
============
     | x   y
------------
r2   | 7   11
r3   | 8   12
------------
[2 cs x 2 feat]
```

Here:

- cases `[-2, 4)` normalize to `[2, 4)` -> rows `r2`, `r3`
- features `[-3, -1)` normalize to `[1, 3)` -> columns `x`, `y`

So `zoom()` inherits the same practical negative-index ergonomics users already expect
from the case-side slicing family.

Example 3. `zoom()` preserves labels and feature order inside the carved block.

```cpp
auto block = grid.zoom(0, 2, 0, 2);
std::cout << block;
```

### Output

```text
============
     | w   x
------------
r0   | 1   5
r1   | 2   6
------------
[2 cs x 2 feat]
```

Notice that the surviving labels remain `r0` and `r1`; they are not renumbered to
new local positions. This matters when you are matching a zoomed block back to the
source frame during debugging or reporting.

Example 4. `zoom()` is equivalent to an accessor slice followed by feature
projection.

```cpp
auto via_zoom = grid.zoom(1, 4, 2, 4);
auto via_case_study = grid.case_study().slice(1, 4)
                          .features({"y", "z"});

std::cout << via_zoom;
```

### Output

```text
============
     | y   z
------------
r1   | 10  14
r2   | 11  15
r3   | 12  16
------------
[3 cs x 2 feat]
```

`via_zoom` and `via_case_study` describe the same rectangle. `zoom()` exists because
that pattern is common enough to deserve the shorter surface.

Example 5. Reach for `zoom()` only when both axes matter.

If you only wanted the last two rows, this is clearer:

```cpp
auto rows_only = grid.slice(-2, 4);
```

If you only wanted columns `x` and `y`, this is clearer:

```cpp
auto cols_only = grid.features({"x", "y"});
```

`zoom()` earns its place when you want a row interval and a feature interval together
as one rectangular cut.

## Notes

- `zoom()` is purely positional. It is not meant to replace
  [`case_study()`](case_study.md); it is the shorter matrix-block convenience for the
  common positional rectangle case.
- Case labels on the surviving rows are preserved exactly; they are not renumbered.
- Feature order inside the carved block is preserved exactly as it appeared on the
  source frame.
- `zoom()` is half-open on both axes, not inclusive. This keeps it consistent with
  positional `slice(start, stop)` semantics elsewhere in the table stack.
- If you only need rows, [`slice()`](slice.md) is clearer. If you only need columns,
  [`features()`](features.md) is clearer. `zoom()` earns its place when both axes are
  part of the selection at once.

## See Also

- [`DataFrame::case_study`](case_study.md): the more general row/feature selector
  surface `zoom()` is built on.
- [`DataFrame::slice`](slice.md): contiguous case-only slicing.
- [`DataFrame::features`](features.md): feature-only projection by name.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="width.md">
    <span class="label">Previous</span>
    <span class="title">width</span>
  </a>
</div>
