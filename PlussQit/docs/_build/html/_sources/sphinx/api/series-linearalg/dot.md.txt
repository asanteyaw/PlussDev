<!-- pluss-hand-authored -->

# Series::LinearAlgAccessor::dot

[Back to Series::LinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
Series Series::LinearAlgAccessor::dot(const Series& other) const
Series SeriesLinearAlgProxy::dot() const
Series Series::dot(const Series& other) const
```

## What It Does

`dot()` computes the inner product of two one-dimensional `Series`.

For vectors `x` and `y` of length `n`:

```text
dot(x, y) = x[0]y[0] + x[1]y[1] + ... + x[n-1]y[n-1]
```

The result is a rank-zero scalar because the vectors' only dimension is fully
reduced.

`dot()` does not conjugate complex values. If the left vector may be complex
and the Hermitian inner product is intended, use [`vdot()`](vdot.md).

## Pair, Multiply, Reduce

`dot()` measures alignment by pairing positions. Positive paired products add
agreement, negative paired products subtract it, and orthogonal vectors cancel
to zero. It is not elementwise multiplication because no vector dimension
survives: every paired product is reduced into one scalar.

The operation is bilinear. Scaling either input scales the result by the same
factor, and scaling the scalar result scales both reverse sensitivities.

## Parameter

**other** (`const Series&`)
    The right vector. Both operands must be one-dimensional and have the same
    length. Their elementwise multiplication must also have a valid common
    dtype and platform.

## Returns

`Series`
    A rank-zero scalar `Series` containing the sum of paired products.

## Gradient

For real vectors and scalar objective `L = dot(x, y)`:

```text
dL/dx = y
dL/dy = x
```

If `dot()` is part of a larger expression with incoming scalar sensitivity
`g`, both results are scaled by `g`.

```text
dL/dx = g y
dL/dy = g x
```

## Examples

Example 1. Pair corresponding vector positions, then sum.

```cpp
auto left = pluss::make_series({1.0, 2.0, 3.0, 4.0});
auto right = pluss::make_series({10.0, 20.0, 30.0, 40.0});

std::cout << left.linearalg.dot(right) << "\n";
```

The scalar is

```text
1*10 + 2*20 + 3*30 + 4*40 = 300
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=300
```

Example 2. Both input vectors receive the other vector as sensitivity.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0});
auto y = pluss::make_series({4.0, 5.0, 6.0});
x.tracks_slope_(true);
y.tracks_slope_(true);

x.linearalg.dot(y).reverse();

std::cout << "dL_dx=" << x.sensitivity() << "\n";
std::cout << "dL_dy=" << y.sensitivity() << "\n";
```

### Output

```text
dL_dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[4, 5, 6]
dL_dy=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 2, 3]
```

Example 3. An upstream scalar multiplier becomes the incoming sensitivity for
the dot product.

```cpp
auto x = pluss::make_series({1.0, -2.0, 3.0});
auto y = pluss::make_series({4.0, 5.0, -1.0});
x.tracks_slope_(true);
y.tracks_slope_(true);

x.linearalg.dot(y).mul(2.5).reverse();

std::cout << "value=" << x.linearalg.dot(y) << "\n";
std::cout << "dL_dx=" << x.sensitivity() << "\n";
std::cout << "dL_dy=" << y.sensitivity() << "\n";
```

The unscaled dot product is `4 - 10 - 3 = -9`. The objective is
`2.5 * dot(x, y)`, so the sensitivities are `2.5*y` and `2.5*x`.

### Output

```text
value=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=-9
dL_dx=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[10, 12.5, -2.5]
dL_dy=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[2.5, -5, 7.5]
```

Example 4. Orthogonal vectors produce zero through cancellation, not because
either vector is zero.

```cpp
std::cout << pluss::make_series({2.0, 1.0}).linearalg.dot(
    pluss::make_series({-1.0, 2.0})) << "\n";
```

Here `2*(-1) + 1*2 = 0`.

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=0
```

Example 5. Matrices are rejected even when they contain the same number of
elements.

```cpp
try {
  std::cout << pluss::make_series({{1.0, 2.0}}).linearalg.dot(
      pluss::make_series({{3.0, 4.0}})) << "\n";
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
dot: expected two 1D Series
```

Example 6. Vector lengths must match.

```cpp
try {
  std::cout << pluss::make_series({1.0, 2.0}).linearalg.dot(
      pluss::make_series({3.0, 4.0, 5.0})) << "\n";
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
dot: inputs must have the same length
```

## Gotchas And Failure Modes

- Both operands must be one-dimensional. Reshape explicitly before calling if
  transforming a matrix into a vector is genuinely part of the algorithm.
- Equal element count is not a substitute for the one-dimensional contract.
- Vector lengths must match; there is no broadcasting.
- Complex `dot()` does not conjugate either input.

## Summary

`dot()` is a strict vector inner product: pair positions, multiply, sum, and
return a scalar. Use `vdot()` when conjugating the left complex vector is
required.

## See Also

- [`Series::LinearAlgAccessor::vdot`](vdot.md): conjugating vector inner product.
- [`Series::LinearAlgAccessor::mv`](mv.md): matrix-vector product.
- [`Series::LinearAlgAccessor::matmul`](matmul.md): rank-polymorphic product.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="det.html">
    <span class="label">Previous</span>
    <span class="title">det</span>
  </a>
  <a class="next-link" href="eigh.html">
    <span class="label">Next</span>
    <span class="title">eigh</span>
  </a>
</div>
