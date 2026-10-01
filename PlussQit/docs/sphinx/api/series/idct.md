<!-- pluss-hand-authored -->

# Series::idct

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series Series::idct() const
Series Series::idct(std::int64_t dim) const
```

## Reconstruct Samples From Cosine Coefficients

`idct()` is the normalization-matched inverse of [`dct()`](dct.md). For an
axis of length `N`, it reconstructs sample `t` from coefficients `X[k]` as

```text
x[t] = X[0] / N
       + (2 / N) * sum from k=1 to N-1 of
         X[k] * cos(pi * k * (t + 1/2) / N)
```

The first coefficient is weighted differently because the constant cosine
basis has a different norm. Pass coefficients exactly as `dct()` produced
them; pre-dividing by `N` or doubling nonzero coefficients would normalize
twice.

Use `idct()` when values are represented in this real cosine basis and need to
return to sample space. It is not a generic inverse Fourier transform and does
not accept complex coefficients.

The no-argument form reconstructs along the last axis. `idct(dim)` selects an
axis and accepts negative indexing. Every slice across all other axes is
reconstructed independently. Shape and rank are preserved.

Computation uses `float64` on CPU. Integral and Boolean coefficients promote to
`float64`; real floating coefficients also return `float64`. Results return to
the receiver's platform. The output is newly allocated and contiguous.
Radix-two-compatible lengths use the fast transform kernel; other lengths use
the same formula through a direct batched kernel.

## Reverse Mode

`idct()` is linear. Reverse mode multiplies incoming sensitivity by the
transpose of the normalized inverse matrix. The first source coefficient
accumulates `1/N` from every output lane, while each later source coefficient
uses the corresponding `2/N` cosine weights.

## Examples

Example 1. Coefficients produced by the matching forward transform reconstruct
their four samples.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({10.0, -3.154322, 0.0, -0.224171}).idct());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.0000, 2.0000, 3.0000, 4.0000]
```

Example 2. With no axis argument, each row is an independent coefficient
vector.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
pluss::series::print(std::cout, x.idct());
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 2.4880 -1.6667 0.1786 ⎤
⎣ 6.2201 -2.6667 0.4466 ⎦
```

Example 3. Selecting `dim=0` reconstructs down each column instead.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
pluss::series::print(std::cout, x.idct(0));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡  3.3284  4.5355  5.7426 ⎤
⎣ -2.3284 -2.5355 -2.7426 ⎦
```

Example 4. The forward/inverse pair owns all normalization needed for a round
trip.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
pluss::series::print(std::cout, x.dct().idct());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.0000, 2.0000, 3.0000, 4.0000]
```

Example 5. Integer coefficients are promoted before normalized fractions are
formed.

```cpp
auto x = pluss::make_series({4, 2, 0, -1}).idct();
pluss::series::print(std::cout, x);
std::cout << "dtype=" << x.scalar_type()
          << " dim=" << x.dim() << " shape=[" << x.size(0) << ']';
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.7325, 1.8446, 0.1554, 0.2675]
dtype=float64 dim=1 shape=[4]
```

Example 6. A weighted reverse pass demonstrates the transpose of the inverse
matrix.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
x.tracks_slope_(true);
x.idct()
 .mul(pluss::make_series({1.0, 0.0, -1.0, 2.0}))
 .sum()
 .reverse();
pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.5000, -0.2706, 1.4142, -0.6533]
```

Example 7. Invalid axes and complex coefficients fail before reconstruction.

```cpp
try {
  (void)pluss::make_series({{1.0, 2.0}, {3.0, 4.0}}).idct(2);
} catch (const std::exception& e) {
  const std::string message = e.what();
  std::cout << message.substr(0, message.find('\n')) << '\n';
}

try {
  using Complex = std::complex<double>;
  (void)pluss::make_series(std::vector<Complex>{{1.0, 1.0}}, {1}).idct();
} catch (const std::exception& e) {
  const std::string message = e.what();
  std::cout << message.substr(0, message.find('\n'));
}
```

### Output

```text
dct/idct/dst/idst: dim out of range
dct/idct/dst/idst: complex input is not supported
```

## See Also

- [`dct()`](dct.md) produces the coefficients consumed here.
- [`idst()`](idst.md) reconstructs from the corresponding sine basis.
- [`ifftshift()`](ifftshift.md) restores wrapped frequency-bin layout but does not reconstruct samples.
