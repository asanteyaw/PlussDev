<!-- pluss-hand-authored -->

# Series::rot90

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::rot90() const -> Series
Series::rot90(std::int64_t k) const -> Series
Series::rot90(std::int64_t k, std::int64_t dim1, std::int64_t dim2) const -> Series
```

## What It Does

`rot90()` rotates one selected two-dimensional plane in quarter turns. By
default it makes one counter-clockwise turn in axes `0` and `1`. `k` counts
quarter turns: positive values are counter-clockwise, negative values are
clockwise, and all values are normalised modulo four. Thus `k = 0`, `4`, and
`-4` are no-ops.

On higher-rank data, `dim1` and `dim2` choose the plane inside every remaining
coordinate lane. A one- or three-quarter turn swaps the lengths of the two
chosen axes; a half turn retains them. The two dimensions must be distinct.

## Parameters

**k** (`std::int64_t`) is the signed count of quarter turns.

**dim1** and **dim2** (`std::int64_t`) are the plane axes. They may be negative
but must resolve to two different valid axes.

## Returns

A new Series with values rotated on the selected plane. Dtype is preserved;
shape is preserved for half turns and swaps the selected lengths for odd turns.

## Gradient

Rotation is a permutation of coordinates. Its reverse mapping is the opposite
rotation on the same plane, so it does not change sensitivity magnitudes.

## Examples

Example 1. The default is one counter-clockwise turn in the first two axes.

```cpp
auto table = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
pluss::series::print(std::cout, table.rot90());
```

### Output

```text
Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[3, 6],
  [2, 5],
  [1, 4]]
```

The selected lengths change from `[2, 3]` to `[3, 2]` because the turn is odd.

Example 2. Two quarter turns are a half turn.

```cpp
std::cout << table.rot90(2);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[6, 5, 4],
  [3, 2, 1]]
```

Example 3. A negative quarter turn rotates clockwise.

```cpp
std::cout << table.rot90(-1);
```

### Output

```text
pluss::Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[4, 1],
  [5, 2],
  [6, 3]]
```

Example 4. Explicit dimensions rotate the inner plane of every cube slice.

```cpp
auto cube = pluss::make_series({
    {{1.0, 2.0}, {3.0, 4.0}},
    {{5.0, 6.0}, {7.0, 8.0}},
});
std::cout << cube.rot90(1, 1, 2);
```

### Output

```text
pluss::Series(shape=[2,2,2], strides=[4,2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[[2, 4],
  [1, 3]],
  [[6, 8],
  [5, 7]]]
```

Example 5. A plane needs two distinct axes.

```cpp
try { std::cout << table.rot90(1, 0, 0); }
catch (const std::exception& error) { std::cout << error.what(); }
```

### Output

```text
rot90: dims must be different
```

## Gotchas And Failure Modes

- `k` is modulo four; do not use a large `k` to request repeated allocation.
- This rotates only the named plane. It never flattens the Series.
- An odd turn swaps selected axis lengths; a half turn does not.
- Equal or invalid dimensions throw.

## Summary

Use `rot90()` for a default matrix-plane quarter turn, `rot90(k)` to control
direction and count, and `rot90(k, dim1, dim2)` for a chosen plane in
higher-rank data. It is a shape-aware coordinate permutation, not a reduction.

## See Also

- [`Series::flip`](flip.md)
- [`Series::roll`](roll.md)
- [`Series::transpose`](transpose.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="round.html">
    <span class="label">Previous</span>
    <span class="title">round</span>
  </a>
  <a class="next-link" href="rsqrt.html">
    <span class="label">Next</span>
    <span class="title">rsqrt</span>
  </a>
</div>
