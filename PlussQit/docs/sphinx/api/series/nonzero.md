<!-- pluss-hand-authored -->

# Series::nonzero

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::nonzero() const -> Series
```

## What It Does

`nonzero()` returns the coordinates of every truthy element.

For numeric series, "nonzero" really means "not equal to zero". For boolean
series, it means every `true` position. The return value is not a mask and not
the values themselves. It is an index matrix.

That makes `nonzero()` the natural bridge between:

- value-level filtering logic
- explicit integer indexing
- sparse coordinate style workflows

## Returns

`Series`
    An `int64` series of coordinates with shape `[nnz, dim]`, where `nnz` is
    the number of truthy elements.

Practical shape rules:

- rank-2 input -> one row per hit, two columns for coordinates
- rank-1 input -> shape `[nnz, 1]`
- scalar nonzero -> shape `[1, 0]`
- scalar zero -> shape `[0, 0]`

That scalar behavior may look unusual at first, but it is internally
consistent: there are zero coordinate columns because a scalar has zero axes.

## Gradient

`nonzero()` is not a differentiable numeric transform.

It produces integer coordinates, so treat it as control information for later
indexing rather than something that participates in gradient flow.

## Examples

Example 1. A matrix returns one coordinate row per truthy entry.

```cpp
auto mask2d = pluss::make_series({
    {0.0, 2.0, 0.0},
    {3.0, 0.0, 4.0},
});

pluss::series::print(std::cout, mask2d.nonzero());
```

### Output

```text
Series(shape=[3,2], strides=[2,1], dtype=int64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 0 1 ⎤
⎢ 1 0 ⎥
⎣ 1 2 ⎦
```

The hits are at `(0, 1)`, `(1, 0)`, and `(1, 2)`.

Example 2. A vector still returns a 2D index result, but with one coordinate
column.

```cpp
std::cout << pluss::make_series({0.0, -2.0, 0.0, 5.0}).nonzero();
```

### Output

```text
pluss::Series(shape=[2,1], strides=[1,1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [3]]
```

This means the truthy elements live at flat positions `1` and `3`.

Example 3. A nonzero scalar reports one hit and zero coordinate columns.

```cpp
std::cout << pluss::make_series(5.0).nonzero();
```

### Output

```text
pluss::Series(shape=[1,0], strides=[0,1], dtype=int64, platform=cpu, numel=0, contiguous=true)
[]
```

There is one truthy scalar, but there are no axis coordinates to report.

Example 4. A zero scalar reports no hits.

```cpp
std::cout << pluss::make_series(0.0).nonzero();
```

### Output

```text
pluss::Series(shape=[0,0], strides=[0,1], dtype=int64, platform=cpu, numel=0, contiguous=true)
[]
```

Example 5. Empty inputs stay empty, but the coordinate width still matches the
input rank.

```cpp
std::cout << pluss::series::zeros({0, 3}).nonzero();
```

### Output

```text
pluss::Series(shape=[0,2], strides=[2,1], dtype=int64, platform=cpu, numel=0, contiguous=true)
[]
```

The input rank is `2`, so the empty coordinate matrix still has two columns.

## Gotchas And Failure Modes

- `nonzero()` returns coordinates, not values.
- Vector results are still rank `2`, not plain 1D index lists.
- Scalar outputs use the `[1,0]` and `[0,0]` conventions; that is deliberate,
  not a formatting bug.
- If what you want is a boolean mask, use comparisons or logical methods
  directly rather than `nonzero()`.

## Summary

Use `nonzero()` when you want to convert truthiness into explicit positions.

- read each output row as one hit
- read each output column as one axis coordinate
- expect integer coordinates, not data values
- expect scalar inputs to use zero coordinate columns

## See Also

- [`Series::where`](where.md): choose values by a boolean mask.
- [`Series::take`](take.md): consume explicit index positions.
- [`Series::index`](index.md): broader advanced indexing once you already have
  coordinate-like inputs.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nextafter.html">
    <span class="label">Previous</span>
    <span class="title">nextafter</span>
  </a>
  <a class="next-link" href="norm.html">
    <span class="label">Next</span>
    <span class="title">norm</span>
  </a>
</div>
