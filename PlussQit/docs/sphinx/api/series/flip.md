<!-- pluss-hand-authored -->

# Series::flip

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::flip(std::int64_t dim) const
Series Series::flip(pluss::RangeView<std::int64_t> dims) const
```

## What It Does

`flip()` reverses the order of values along one or more axes. It does not
reduce, rotate, or flatten the Series: dtype and shape are preserved, while
the coordinate used to read each selected axis is mirrored. For an axis of
length `n`, output index `i` reads source index `n - 1 - i`.

The scalar overload reverses one axis. The braced-list overload reverses every
listed axis. Negative dimensions count from the end. Repeated dimensions are
deduplicated, so repeating an axis does not apply two reversals and therefore
does not cancel the first one.

## Parameters

**dim** (`std::int64_t`) is one valid axis. `-1` means the final axis.

**dims** is a brace-list of valid axes, such as `{0, -1}`. Each unique axis is
reversed once. It is useful when the intended transformation has to mirror two
or more coordinate directions together.

## Returns

A new Series of the same shape and dtype. `flip()` is out-of-place and leaves
the source unchanged.

## Gradient

Flip is a pure permutation: it changes where values occur, not their numeric
values. Its reverse mapping is the same reversal, so a compatible sensitivity
is mirrored back along the same axes.

## Examples

Example 1. Axis `0` reverses the row order of a matrix.

```cpp
auto table = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
pluss::series::print(std::cout, table.flip(0));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 4.0000 5.0000 6.0000 ⎤
⎣ 1.0000 2.0000 3.0000 ⎦
```

Example 2. `-1` reverses the final axis, the columns of this matrix.

```cpp
std::cout << table.flip(-1);
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[3, 2, 1],
  [6, 5, 4]]
```

Example 3. A dimension list applies the mirror in both directions.

```cpp
std::cout << table.flip({0, 1});
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[6, 5, 4],
  [3, 2, 1]]
```

Example 4. Duplicate dimensions are intentionally used once.

```cpp
std::cout << table.flip({0, 0});
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[4, 5, 6],
  [1, 2, 3]]
```

Example 5. The outer axis of a cube is just another valid axis.

```cpp
auto cube = pluss::make_series({
    {{1.0, 2.0}, {3.0, 4.0}},
    {{5.0, 6.0}, {7.0, 8.0}},
});
std::cout << cube.flip(0);
```

### Output

```text
pluss::Series(shape=[2,2,2], strides=[4,2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[[5, 6],
  [7, 8]],
  [[1, 2],
  [3, 4]]]
```

Example 6. A missing axis is an error.

```cpp
try { std::cout << table.flip(2); }
catch (const std::exception& error) { std::cout << error.what(); }
```

### Output

```text
flip: dim out of range
```

## Gotchas And Failure Modes

- `flip({0, 0})` is `flip(0)`, not the original Series.
- Shape is unchanged even though the physical traversal order may differ.
- Invalid axes throw; they are never ignored.

## Summary

Use `flip(dim)` to mirror one coordinate direction and `flip({dims...})` to
mirror several at once. It is an out-of-place, shape-preserving permutation;
negative axes work and duplicate axes are deliberately deduplicated.

## See Also

- [`Series::flipud`](flipud.md)
- [`Series::fliplr`](fliplr.md)
- [`Series::rot90`](rot90.md)
