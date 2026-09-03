<!-- pluss-hand-authored -->

# Series::flipud

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::flipud() const -> Series
```

## What It Does

`flipud()` is the named form of `flip(0)`. It reverses the outermost axis:
the top-to-bottom direction for a matrix, or the outer batch-like direction for
any higher-rank Series. It preserves shape, dtype, and the order inside every
individual slice.

## Returns

A new Series with axis `0` reversed. The source is unchanged.

## Gradient

This is an axis-0 permutation, so compatible reverse-mode sensitivity follows
the same axis-0 reversal.

## Examples

Example 1. A matrix's first and last rows exchange places.

```cpp
auto table = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
std::cout << table.flipud();
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[4, 5, 6],
  [1, 2, 3]]
```

Example 2. It remains an outer-axis operation for a cube.

```cpp
auto cube = pluss::make_series({
    {{1.0, 2.0}, {3.0, 4.0}},
    {{5.0, 6.0}, {7.0, 8.0}},
});
std::cout << cube.flipud();
```

### Output

```text
pluss::Series(shape=[2,2,2], strides=[4,2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[[5, 6],
  [7, 8]],
  [[1, 2],
  [3, 4]]]
```

## Gotchas And Failure Modes

- This is exactly `flip(0)`, not a rank-two-only geometric primitive.
- A scalar has no axis `0` and fails axis validation.

## Summary

Use `flipud()` when the intended operation is always outer-axis reversal. Use
`flip(dim)` instead when the axis is chosen dynamically.

## See Also

- [`Series::flip`](flip.md)
- [`Series::fliplr`](fliplr.md)
