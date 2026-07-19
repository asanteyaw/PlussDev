<!-- pluss-hand-authored -->

# DataFrame::matrix

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::matrix() const -> MatrixValues
```

## What It Does

Packs the frame's features into raw, matrix-shaped `pluss` payloads, grouped by
payload kind — one packed matrix for the `Series` features (if any), one for the
`Note` features (if any), one for the `Timeline` features, and one for the
`TimeIndex` features. `matrix()` never coerces different kinds together into one
mixed block; a frame with both numeric and text features gets one `Series` matrix
and one `Note` matrix back, not a single combined block.

This is the lower-level escape hatch behind higher-level numeric operations like
[`matmul()`](matmul.md) and [`corr()`](corr.md), for when you need direct access to
the packed matrix payload itself.

## Parameters

`None`

## Returns

`MatrixValues`
    A struct of four vectors (`Series`, `Note`, `Timeline`, `TimeIndex`), each holding
    zero or one packed matrix — never more than one per kind, since every feature of
    a given kind is packed together into a single block. `.series_span()` /
    `.note_span()` / `.timeline_span()` / `.timeindex_span()` expose each vector
    directly; `.as_series()` / `.as_note()` / `.as_timeline()` / `.as_timeindex()` are
    convenience accessors that return the single block directly, throwing if that
    kind isn't present on the frame (or, in principle, if more than one block of that
    kind exists — which never happens in practice, since `matrix()` always packs a
    kind into exactly one block).

## Examples

Example 1. Inspect how many matrix blocks a mixed-kind frame produces.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"id",    Thread(pluss::make_series({1, 2, 3}))},
    {"sales", Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city",  Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
std::cout << orders;
```

### Output

```text
==========================
     | id    sales  city  
--------------------------
0    | 1     100    Accra 
1    | 2     120    Tema  
2    | 3     90     Kumasi
--------------------------
[3 cs x 3 feat]
```

```cpp
auto blocks = orders.matrix();
std::cout << blocks.series_span().size() << "\n";
std::cout << blocks.note_span().size() << "\n";
std::cout << blocks.timeline_span().size() << "\n";
std::cout << blocks.timeindex_span().size() << "\n";
```

### Output

```text
1
1
0
0
```

`id` and `sales` (both `Series`) are packed together into one numeric matrix block;
`city` (`Note`) is packed into its own separate text matrix block. There are no
`Timeline` or `TimeIndex` features on this frame, so those two counts are `0`.

Example 2. Pull out the packed numeric matrix directly.

```cpp
auto numeric_matrix = blocks.as_series();
std::cout << numeric_matrix.sizes()[0] << " x " << numeric_matrix.sizes()[1] << "\n";
```

### Output

```text
3 x 2
```

`orders` has `3` cases and `2` numeric features (`id`, `sales`), so the packed
`Series` matrix has shape `3 x 2` — cases as rows, features as columns.

## Notes

- `matrix()` groups by payload kind only; it never mixes `Series` and `Note` data
  into one block, since they have no shared numeric representation.
- `.as_series()` (and its `Note` / `Timeline` / `TimeIndex` counterparts) throw if
  that kind isn't present on the frame at all — check `.series_span().size()` (or the
  equivalent for the kind you need) first if the frame's composition isn't already
  known.
- The packed matrix's row order always matches the frame's case order, and its
  column order always matches the order those features appear in the frame.

## See Also

- [`DataFrame::matmul`](matmul.md): matrix multiplication built directly on top of
  the packed `Series` matrix this method exposes.
- [`DataFrame::corr`](corr.md): pairwise correlation across every numeric feature,
  another consumer of the packed numeric matrix.
- [`DataFrame::dtypes`](dtypes.md): inspect each feature's concrete kind and dtype
  individually, without packing anything into a matrix.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="matmul.html">
    <span class="label">Previous</span>
    <span class="title">matmul</span>
  </a>
  <a class="next-link" href="max.html">
    <span class="label">Next</span>
    <span class="title">max</span>
  </a>
</div>
