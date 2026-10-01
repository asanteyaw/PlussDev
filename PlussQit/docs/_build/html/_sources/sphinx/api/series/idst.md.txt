<!-- pluss-hand-authored -->

# Series::idst

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series Series::idst() const
Series Series::idst(std::int64_t dim) const
```

## Reconstruct Samples From Sine Coefficients

`idst()` is the normalization-matched inverse of [`dst()`](dst.md). It maps
type-II discrete sine coefficients back to samples along one axis. For a
length-`N` coefficient vector, the inverse uses the sine basis

```text
sin(pi * (t + 1) * (k + 1/2) / N)
```

with scale `2/N` for ordinary coefficient rows and `1/N` for the final row.
That last coefficient has twice the squared norm of the others. Its half
weight is part of the inverse contract for both odd and even lengths; callers
must not special-case or pre-scale it.

Use `idst()` when coefficients describe an odd boundary continuation and must
return to sample space. Use [`idct()`](idct.md) for an even continuation and a
cosine basis. This method is real-valued and rejects complex coefficients.

The no-argument form reconstructs the last axis. The `dim` overload accepts
positive or negative axes and reconstructs every independent slice across the
other dimensions. Shape and rank do not change.

Integral and Boolean input promote to `float64`; real floating input is
computed as `float64` on CPU. The newly allocated contiguous result returns to
the receiver's platform. Transform-friendly lengths use the fast kernel, while
other lengths use the same normalized formula through a direct batched path.

## Reverse Mode

The inverse sine transform is linear. Its reverse rule applies the transpose
of the normalized inverse matrix to incoming sensitivity. Consequently the
final source coefficient retains its distinct `1/N` weighting in reverse mode;
the rule is not an unscaled call to the public `dst()` method.

## Examples

Example 1. A coefficient vector produced by `dst()` reconstructs the source
samples.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({6.532815, -2.828427, 2.705981, -2.0}).idst());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.0000, 2.0000, 3.0000, 4.0000]
```

Example 2. By default, each row of a matrix is an independent coefficient
vector.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
pluss::series::print(std::cout, x.idst());
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 2.4880 -0.3333 0.1786 ⎤
⎣ 6.2201  0.6667 0.4466 ⎦
```

Example 3. `dim=0` reconstructs each column instead.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
pluss::series::print(std::cout, x.idst(0));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡  2.7071  3.9142  5.1213 ⎤
⎣ -1.2929 -1.0858 -0.8787 ⎦
```

Example 4. The matching pair round-trips an even transformed axis.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
pluss::series::print(std::cout, x.dst().idst());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.0000, 2.0000, 3.0000, 4.0000]
```

Example 5. The final-coefficient normalization also reconstructs odd lengths.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
pluss::series::print(std::cout, x.dst().idst());
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 1.0000 2.0000 3.0000 ⎤
⎣ 4.0000 5.0000 6.0000 ⎦
```

Example 6. Integer coefficients produce floating reconstruction values.

```cpp
auto x = pluss::make_series({4, 2, 0, -1}).idst();
pluss::series::print(std::cout, x);
std::cout << "dtype=" << x.scalar_type()
          << " dim=" << x.dim() << " shape=[" << x.size(0) << ']';
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.2225, 2.8049, 0.8907, 0.3083]
dtype=float64 dim=1 shape=[4]
```

Example 7. Reverse mode preserves the inverse matrix's asymmetric final-row
normalization.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
x.tracks_slope_(true);
x.idst()
 .mul(pluss::make_series({1.0, 0.0, -1.0, 2.0}))
 .sum()
 .reverse();
pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.1209e-01, -2.2204e-16, 1.5772e+00, -5.0000e-01]
```

Example 8. Invalid axes and complex coefficients are rejected.

```cpp
try {
  (void)pluss::make_series({{1.0, 2.0}, {3.0, 4.0}}).idst(-3);
} catch (const std::exception& e) {
  const std::string message = e.what();
  std::cout << message.substr(0, message.find('\n')) << '\n';
}

try {
  using Complex = std::complex<double>;
  (void)pluss::make_series(std::vector<Complex>{{1.0, 1.0}}, {1}).idst();
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

- [`dst()`](dst.md) produces the coefficients consumed here.
- [`idct()`](idct.md) reconstructs an even-boundary cosine representation.
- [`ifftshift()`](ifftshift.md) reorders frequency bins without applying an inverse transform.
