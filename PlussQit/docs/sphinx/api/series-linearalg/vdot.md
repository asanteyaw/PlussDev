<!-- pluss-hand-authored -->

# Series::LinearAlgAccessor::vdot

[Back to Series::LinearAlgAccessor API Reference](../../series_linearalg_accessor_api_reference.md)

## Signatures

```cpp
Series Series::LinearAlgAccessor::vdot(const Series& other) const
Series SeriesLinearAlgProxy::vdot() const
Series Series::vdot(const Series& other) const
```

## What It Does

`vdot()` computes the Hermitian inner product of two one-dimensional `Series`.
It conjugates the left vector, multiplies corresponding positions, and sums:

```text
vdot(x, y) = sum(conj(x[i]) * y[i], over i)
```

For real values, conjugation changes nothing, so `vdot()` and `dot()` produce
the same number. For complex values, the distinction is essential: changing
which operand is conjugated can change both the sign and magnitude of the
imaginary component.

## The Hermitian Inner Product

For complex vectors, `vdot()` is the inner product that behaves like a squared
length when a vector is paired with itself. Every self-product is
`conj(z) * z = |z|^2`, a nonnegative real value. Swapping different operands
conjugates the scalar result:

```text
vdot(y, x) = conj(vdot(x, y))
```

That symmetry is the practical reason operand order must remain visible in
complex code.

## Parameter

**other** (`const Series&`)
    The right vector. Both operands must be one-dimensional and have equal
    length. There is no broadcasting.

## Returns

`Series`
    A rank-zero scalar. Its dtype follows the numeric operation; complex input
    produces a complex scalar.

## Gradient

`vdot()` is built from conjugation, elementwise multiplication, and summation,
so tracked inputs participate in reverse-mode propagation according to those
operations' complex-sensitivity rules.

For real vectors, the familiar result is identical to `dot()`:

```text
dL/dx = y
dL/dy = x
```

For complex calculations, keep the operand order deliberate because `vdot(x,
y)` conjugates `x`, not `y`.

## Examples

Example 1. Real vectors produce the ordinary inner product.

```cpp
std::cout << pluss::make_series({1.0, 2.0, 3.0}).linearalg.vdot(
    pluss::make_series({4.0, 5.0, 6.0})) << "\n";
```

### Output

```text
pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=32
```

Example 2. Complex input shows exactly what left conjugation changes.

```cpp
using Complex = std::complex<double>;

auto left = pluss::make_series(
    std::vector<Complex>{{1.0, 2.0}, {3.0, -1.0}}, {2});
auto right = pluss::make_series(
    std::vector<Complex>{{2.0, -1.0}, {4.0, 2.0}}, {2});

std::cout << "dot=" << left.linearalg.dot(right) << "\n";
std::cout << "vdot=" << left.linearalg.vdot(right) << "\n";
```

Without conjugation, the paired products sum to `18 + 5j`. With left
conjugation:

```text
conj(1 + 2j)(2 - 1j) + conj(3 - 1j)(4 + 2j)
= (1 - 2j)(2 - 1j) + (3 + 1j)(4 + 2j)
= 10 + 5j
```

### Output

```text
dot=pluss::Series(shape=[], strides=[], dtype=complex128, platform=cpu, numel=1, contiguous=true)
data=(18+5j)
vdot=pluss::Series(shape=[], strides=[], dtype=complex128, platform=cpu, numel=1, contiguous=true)
data=(10+5j)
```

Example 3. Continuing with the complex vectors from Example 2, swapping
operands conjugates the result.

```cpp
std::cout << "forward=" << left.linearalg.vdot(right) << "\n";
std::cout << "reverse=" << right.linearalg.vdot(left) << "\n";
```

### Output

```text
forward=pluss::Series(shape=[], strides=[], dtype=complex128, platform=cpu, numel=1, contiguous=true)
data=(10+5j)
reverse=pluss::Series(shape=[], strides=[], dtype=complex128, platform=cpu, numel=1, contiguous=true)
data=(10-5j)
```

Example 4. Pair the same `left` vector from Example 2 with itself to obtain
its squared complex length.

```cpp
std::cout << left.linearalg.vdot(left) << "\n";
```

For `left = [1+2j, 3-1j]`, the real scalar is
`|1+2j|^2 + |3-1j|^2 = 5 + 10 = 15`.

### Output

```text
pluss::Series(shape=[], strides=[], dtype=complex128, platform=cpu, numel=1, contiguous=true)
data=(15+0j)
```

Example 5. Rank is part of the contract.

```cpp
try {
  std::cout << pluss::make_series({{1.0, 2.0}}).linearalg.vdot(
      pluss::make_series({{3.0, 4.0}})) << "\n";
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
vdot: expected two 1D Series
```

Example 6. Equal vector lengths are required.

```cpp
try {
  std::cout << pluss::make_series({1.0}).linearalg.vdot(
      pluss::make_series({2.0, 3.0})) << "\n";
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
vdot: inputs must have the same length
```

## Gotchas And Failure Modes

- `vdot(left, right)` conjugates `left` only.
- Both operands must be one-dimensional and equally long.
- There is no shape broadcasting.
- For real-only code, `dot()` may communicate intent more directly because
  conjugation has no effect.

## Summary

`vdot()` is the strict complex-aware vector inner product. It returns
`sum(conj(left) * right)` as a scalar and rejects non-vector inputs.

## See Also

- [`Series::LinearAlgAccessor::dot`](dot.md): non-conjugating vector inner product.
- [`Series::LinearAlgAccessor::matmul`](matmul.md): vector, matrix, and batched
  product.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="triangular_solve.html">
    <span class="label">Previous</span>
    <span class="title">triangular_solve</span>
  </a>
  <a class="next-link" href="../../series_api_reference.html">
    <span class="label">Next</span>
    <span class="title">Series API Reference</span>
  </a>
</div>
