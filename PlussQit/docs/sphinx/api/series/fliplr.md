<!-- pluss-hand-authored -->

# Series::fliplr

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::fliplr() const
```

## What It Does

`fliplr()` reverses axis `1`. In a matrix this is left-to-right reversal of
every row. In a higher-rank Series, it reverses the second axis while leaving
the outer and remaining inner axes structurally intact. Unlike `flip(-1)`, its
meaning does not follow the last axis, and it requires rank at least two.

## Returns

A new Series of the same shape and dtype, with axis `1` mirrored.

## Gradient

This is a coordinate permutation. Compatible sensitivity is correspondingly
permuted through axis `1`.

## Examples

Example 1. Matrix rows reverse independently.

```cpp
auto table = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
pluss::series::print(std::cout, table.fliplr());
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[3, 2, 1],
  [6, 5, 4]]
```

Example 2. On a cube, the second axis is reversed in each outer slice.

```cpp
auto cube = pluss::make_series({
    {{1.0, 2.0}, {3.0, 4.0}},
    {{5.0, 6.0}, {7.0, 8.0}},
});
std::cout << cube.fliplr();
```

### Output

```text
pluss::Series(shape=[2,2,2], strides=[4,2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[[3, 4],
  [1, 2]],
  [[7, 8],
  [5, 6]]]
```

Example 3. A vector has no axis `1`.

```cpp
try { std::cout << pluss::make_series({1.0, 2.0, 3.0}).fliplr(); }
catch (const std::exception& error) { std::cout << error.what(); }
```

### Output

```text
fliplr: expected input with rank >= 2
```

## Gotchas And Failure Modes

- `fliplr()` means axis `1`, not the final axis.
- Rank-zero and rank-one input are rejected.
- Use `flip(-1)` when the desired axis is always the final one.

## Summary

Use `fliplr()` for an explicit second-axis mirror on rank-two-or-greater data.
It preserves shape and dtype, but does not generalise to vectors.

## See Also

- [`Series::flip`](flip.md)
- [`Series::flipud`](flipud.md)
