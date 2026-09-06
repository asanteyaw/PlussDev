<!-- pluss-hand-authored -->

# Series::ravel

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::ravel() const -> Series
```

## What It Does

`ravel()` returns the series flattened to one dimension, reading its elements in
row-major (last-axis-fastest) logical order.

It is equivalent to [`flatten()`](flatten.md) over the whole shape. For a
`2 x 4` matrix it produces a length-8 vector; for a rank-1 series it is a no-op.

When the input is contiguous the result can share storage; when it is not (for
example after a [`transpose()`](transpose.md)), `ravel()` materializes the
elements in visible order into a fresh contiguous buffer.

## Parameters

None.

## Returns

`Series`
    A 1-D `Series` of length `numel()`, in row-major logical order.

## Gradient

`ravel()` is differentiable. It only relabels axes, so the backward reshapes the
upstream gradient back to the original shape — the values are carried through
unchanged.

## Examples

Example 1. A `2 x 4` matrix flattened to a length-8 vector.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0, 4.0}, {5.0, 6.0, 7.0, 8.0}});

pluss::series::print(std::cout, m.ravel());
```

### Output

```text
Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[1.0000, 2.0000, 3.0000, 4.0000, 5.0000, 6.0000, 7.0000, 8.0000]
```

Example 2. On a non-contiguous input, `ravel()` reads the elements in their
visible order and materializes them.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0, 4.0}, {5.0, 6.0, 7.0, 8.0}});

pluss::series::print(std::cout, m.transpose(0, 1).ravel());
```

### Output

```text
Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[1.0000, 5.0000, 2.0000, 6.0000, 3.0000, 7.0000, 4.0000, 8.0000]
```

The transposed view is `[[1,5],[2,6],[3,7],[4,8]]`, and `ravel()` walks it
row by row.

Example 3. The backward reshapes the gradient back to the original shape.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0, 4.0}, {5.0, 6.0, 7.0, 8.0}});
m.tracks_slope_(true);

m.ravel().sum().reverse();

pluss::series::print(std::cout, m.sensitivity());
```

### Output

```text
Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=
⎡ 1.0000 1.0000 1.0000 1.0000 ⎤
⎣ 1.0000 1.0000 1.0000 1.0000 ⎦
```

## Gotchas And Failure Modes

- `ravel()` always flattens the whole series; there is no partial-range option
  (that is [`flatten()`](flatten.md) with explicit axes).
- The result may or may not share storage with the input depending on
  contiguity. Do not assume it aliases (or does not).
- Row-major order: the last axis varies fastest.

## Summary

`ravel()` flattens the series to 1-D in row-major order:

- equivalent to a whole-shape [`flatten()`](flatten.md)
- copies when the input is non-contiguous, may alias when it is
- differentiable, backward just reshapes the gradient

## See Also

- [`Series::flatten`](flatten.md): flatten a chosen range of axes; `ravel()` is
  the whole-shape case.
- [`Series::reshape`](reshape.md): choose an arbitrary target shape.
- [`Series::contiguous`](contiguous.md): force a dense copy without changing the
  shape.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rad2deg.html">
    <span class="label">Previous</span>
    <span class="title">rad2deg</span>
  </a>
  <a class="next-link" href="real.html">
    <span class="label">Next</span>
    <span class="title">real</span>
  </a>
</div>
