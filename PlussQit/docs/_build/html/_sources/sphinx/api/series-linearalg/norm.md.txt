<!-- pluss-hand-authored -->

# Series::LinearAlgAccessor::norm

[Back to Series::LinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signature

```cpp
Series Series::LinearAlgAccessor::norm() const
Series SeriesLinearAlgProxy::norm() const
```

## What It Does

`norm()` measures the Euclidean length of the entire `Series`. It squares every
element, adds those squares, and takes one square root:

```text
||x||2 = sqrt(sum_i x_i^2)
```

For a matrix this same calculation is called the Frobenius norm. Dimensions do
not identify independent rows, columns, or batches here: all elements
participate in one global reduction and the result is a rank-zero scalar.

## Length In Any Number Of Dimensions

The norm answers “how large is this value as one point?” A vector `[3,4]` has
length `5`; reshaping the same values does not change that length. This makes
the method useful for residual size, parameter magnitude, and normalization,
but not for obtaining one norm per row or per batch item.

## Returns

`Series`
    A scalar `Series` with `shape=[]`.

## Gradient

Away from zero, each element receives `x_i / ||x||2`. An element contributes
more sensitivity in proportion to its share of the total length. At the zero
input the implementation returns a zero subgradient rather than dividing by
zero.

## Examples

Example 1. The familiar 3-4-5 triangle becomes a scalar length.

```cpp
auto vector = pluss::make_series({3.0, 4.0});
std::cout << vector.linearalg.norm() << "\n";
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=5
```

Example 2. A matrix is reduced globally, not row by row.

```cpp
auto matrix = pluss::make_series({{1.0, 2.0}, {2.0, 1.0}});
std::cout << matrix.linearalg.norm() << "\n";
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=3.1622777
```

Example 3. Reverse mode points in the input's direction.

```cpp
auto vector = pluss::make_series({3.0, 4.0});
vector.tracks_slope_(true);
vector.linearalg.norm().reverse();
std::cout << vector.sensitivity() << "\n";
```

### Output

```text
pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.6, 0.8]
```

## Gotchas And Failure Modes

- `norm()` is global; it has no dimension or `keepdim` parameter.
- The result is a scalar `Series`, not a C++ `double`.
- Squaring very large values can overflow and very small values can underflow.

## Summary

`norm()` returns one Euclidean/Frobenius magnitude for the complete input.

## See Also

- [`Series::div`](../series/div.md): divide by a computed norm to rescale values.
- [`Series::LinearAlgAccessor::dot`](dot.md): inner product of two vectors.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mv.html"><span class="label">Previous</span><span class="title">mv</span></a>
  <a class="next-link" href="pinverse.html"><span class="label">Next</span><span class="title">pinverse</span></a>
</div>
