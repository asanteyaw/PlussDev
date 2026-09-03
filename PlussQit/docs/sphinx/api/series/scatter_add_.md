<!-- pluss-hand-authored -->

# Series::scatter_add_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::scatter_add_(std::int64_t dim, const Series& index, const Series& src) const -> Series&
```

## What It Does

`scatter_add_()` performs the same indexed accumulation as
[`scatter_add()`](scatter_add.md), but it adds directly into the current
`Series` and returns the same handle.

Use it when the destination itself should be updated in place rather than
copied first.

As with the other in-place write family on this surface, this method is
declared `const` on the handle even though it mutates the underlying value.

## Parameters

The parameters match [`scatter_add()`](scatter_add.md):

- `dim` chooses the destination axis
- `index` supplies integral destination positions
- `src` supplies additive contributions and may broadcast to the index shape

## Returns

`Series&`
    The same `Series`, after mutation.

## Examples

Example 1. A basic in-place additive scatter updates the current value.

```cpp
auto base = pluss::make_series({10.0, 20.0, 30.0, 40.0});
auto index = pluss::make_series({1LL, 3LL});
auto src = pluss::make_series({7.0, 8.0});

base.scatter_add_(0, index, src);
std::cout << base;
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10, 27, 30, 48]
```

Example 2. Repeated targets add into the same destination slot in-place.

```cpp
auto repeated = pluss::make_series({0.0, 0.0, 0.0, 0.0});
auto repeated_index = pluss::make_series({0LL, 2LL, 2LL, 3LL});
auto repeated_src = pluss::make_series({0.1, 0.2, 0.3, 0.4});

repeated.scatter_add_(0, repeated_index, repeated_src);
pluss::series::print(std::cout, repeated) << "\n";
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.1000, 0.0000, 0.5000, 0.4000]
```

Example 3. Source broadcasting works on the in-place form too.

```cpp
auto matrix = pluss::make_series({
    {0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0},
});
auto matrix_index = pluss::make_series({
    {0LL, 1LL, 2LL, 3LL},
    {0LL, 1LL, 2LL, 3LL},
});
auto row_src = pluss::make_series({{1.0, 2.0, 3.0, 4.0}});

matrix.scatter_add_(1, matrix_index, row_src);
std::cout << matrix;
```

### Output

```text
pluss::Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[[1, 2, 3, 4],
  [1, 2, 3, 4]]
```

## Gotchas And Failure Modes

- This method mutates the current value.
- Repeated targets add together rather than overwriting.
- `src` may broadcast to the index shape.
- If you want a copied result instead, use [`scatter_add()`](scatter_add.md).

## Summary

Use `scatter_add_()` when indexed additions should land directly on the current
`Series`.

## See Also

- [`Series::scatter_add`](scatter_add.md): the out-of-place sibling.
- [`Series::scatter_`](scatter_.md): in-place overwrite rather than addition.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="scatter_add.html">
    <span class="label">Previous</span>
    <span class="title">scatter_add</span>
  </a>
  <a class="next-link" href="scatter_reduce.html">
    <span class="label">Next</span>
    <span class="title">scatter_reduce</span>
  </a>
</div>
