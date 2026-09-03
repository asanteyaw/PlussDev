<!-- pluss-hand-authored -->

# Series::scatter_

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series::scatter_(std::int64_t dim, const Series& index, const Series& src) const -> Series&
```

## What It Does

`scatter_()` performs the same indexed overwrite as [`scatter()`](scatter.md),
but it writes directly into the current `Series` and returns the same handle.

The axis rules, overwrite semantics, index requirements, and source-shape
requirements are therefore the same. The main difference is lifetime and
mutation:

- `scatter()` leaves `self` unchanged and returns a new result
- `scatter_()` mutates `self`

One public quirk is worth saying plainly: this method is declared `const` on
the handle even though it mutates the underlying value. On this surface, handle
constness does not imply value immutability.

## Parameters

The parameters are the same as [`scatter()`](scatter.md):

- `dim` chooses the destination axis
- `index` supplies integral destination positions
- `src` supplies the replacement values, with the same shape as `index`

## Returns

`Series&`
    The same `Series`, after mutation.

## Examples

Example 1. A simple in-place scatter overwrites the indexed destination slots.

```cpp
auto base = pluss::make_series({10.0, 20.0, 30.0, 40.0});
auto index = pluss::make_series({1LL, 3LL});
auto src = pluss::make_series({5.0, 6.0});

base.scatter_(0, index, src);
std::cout << base;
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10, 5, 30, 6]
```

Example 2. Duplicate targets still follow overwrite semantics in-place.

```cpp
auto duplicate_base = pluss::make_series({10.0, 20.0, 30.0, 40.0});
auto duplicate_index = pluss::make_series({1LL, 1LL});
auto duplicate_src = pluss::make_series({5.0, 6.0});

duplicate_base.scatter_(0, duplicate_index, duplicate_src);
pluss::series::print(std::cout, duplicate_base) << "\n";
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10.0000, 6.0000, 30.0000, 40.0000]
```

Example 3. Negative dims normalize the same way they do on the out-of-place
method.

```cpp
auto matrix = pluss::make_series({
    {0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0},
});
auto matrix_index = pluss::make_series({
    {2LL, 0LL, 1LL},
    {1LL, 2LL, 0LL},
});
auto matrix_src = pluss::make_series({
    {10.0, 20.0, 30.0},
    {40.0, 50.0, 60.0},
});

matrix.scatter_(-1, matrix_index, matrix_src);
std::cout << matrix;
```

### Output

```text
pluss::Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[[20, 30, 10],
  [60, 40, 50]]
```

## Gotchas And Failure Modes

- This method mutates the current value. If you need the original value later,
  use [`scatter()`](scatter.md) instead.
- Repeated targets overwrite rather than accumulate.
- `src` still does not broadcast to the `index` shape.
- Error rules for dtype, shape, and index bounds match [`scatter()`](scatter.md).

## Summary

Use `scatter_()` when you want `scatter()` semantics but need the write to land
directly on the current `Series`.

## See Also

- [`Series::scatter`](scatter.md): the out-of-place sibling.
- [`Series::scatter_add_`](scatter_add_.md): in-place additive scattering.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="scatter.html">
    <span class="label">Previous</span>
    <span class="title">scatter</span>
  </a>
  <a class="next-link" href="scatter_add.html">
    <span class="label">Next</span>
    <span class="title">scatter_add</span>
  </a>
</div>
