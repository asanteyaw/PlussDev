<!-- pluss-hand-authored -->

# Series::dct

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series Series::dct() const
Series Series::dct(std::int64_t dim) const
```

## A Real Frequency Basis For Finite Samples

`dct()` applies an unnormalized type-II discrete cosine transform along one
axis. For an axis of length `N`, output coefficient `k` is

```text
Y[k] = sum from t=0 to N-1 of
       x[t] * cos(pi * (t + 1/2) * k / N)
```

The first coefficient is therefore the sum of the samples. Later coefficients
measure progressively faster cosine patterns. This representation is useful
when the signal is real and an even continuation at its boundaries is the
appropriate model: no complex output is introduced, and smooth variation is
often concentrated in low-index coefficients.

The no-argument form transforms the last axis. The `dim` overload selects any
axis and accepts negative indexing, so `-1` is the last axis. Every independent
slice across the other dimensions is transformed separately. Rank and shape do
not change.

Real floating input is computed in `float64` on CPU. Integral and Boolean
input also produce `float64`, avoiding truncation of cosine coefficients.
Complex input is rejected because this operation's contract is a real cosine
basis. A CPU result is returned to the receiver's platform; the current MPS
route stores its result as `float32` to match the platform's supported path.

The implementation uses a batched `2N` transform when that length has a radix-
two route and a direct vectorized cosine kernel otherwise. Both paths implement
the same formula. The result is newly allocated and contiguous.

Use [`idct()`](idct.md) for the matching inverse. The pair owns its
normalization: `x.dct(dim).idct(dim)` reconstructs `x` to floating precision.

## Reverse Mode

The transform is linear. Reverse mode multiplies the incoming sensitivity by
the transpose of the cosine matrix. This is not generally the same operation
as calling `idct()` on the sensitivity because the public inverse includes its
own normalization. The recorded rule uses the exact transpose coefficients.

## Examples

Example 1. The zero-frequency coefficient is the input sum.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1.0, 2.0, 3.0, 4.0}).dct());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10.0000, -3.1543, 0.0000, -0.2242]
```

Example 2. Omitting `dim` transforms each row along the last axis.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
pluss::series::print(std::cout, x.dct());
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 6.0000e+00 -1.7321e+00 3.3307e-16 ⎤
⎣ 1.5000e+01 -1.7321e+00 1.1102e-15 ⎦
```

Example 3. `dim=0` transforms down the columns instead.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
pluss::series::print(std::cout, x.dct(0));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡  5.0000  7.0000  9.0000 ⎤
⎣ -2.1213 -2.1213 -2.1213 ⎦
```

Example 4. Negative axes normalize to the corresponding non-negative axis.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
std::cout << std::boolalpha << x.dct(-1).equal(x.dct(1));
```

### Output

```text
true
```

Example 5. The matching inverse reconstructs the samples.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1.0, 2.0, 3.0, 4.0}).dct().idct());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.0000, 2.0000, 3.0000, 4.0000]
```

Example 6. Integral samples are promoted before fractional coefficients form.

```cpp
auto y = pluss::make_series({1, 0, -1, 0}).dct();
pluss::series::print(std::cout, y);
std::cout << "dtype=" << y.scalar_type()
          << " dim=" << y.dim() << " shape=[" << y.size(0) << ']';
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.0000, 1.3066, 1.4142, -0.5412]
dtype=float64 dim=1 shape=[4]
```

Example 7. Weighted reverse mode demonstrates the transpose transform.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
x.tracks_slope_(true);
x.dct().mul(pluss::make_series({1.0, 0.0, -1.0, 2.0})).sum().reverse();
pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.0583, -0.1407, 3.5549, -0.4725]
```

Example 8. An axis outside the receiver's rank is rejected.

```cpp
try {
  (void)pluss::make_series({{1.0, 2.0}, {3.0, 4.0}}).dct(2);
} catch (const std::exception& e) {
  const std::string message = e.what();
  std::cout << message.substr(0, message.find('\n'));
}
```

### Output

```text
dct/idct/dst/idst: dim out of range
```

## See Also

- [`idct()`](idct.md) is the normalization-matched inverse.
- [`dst()`](dst.md) uses a sine basis and odd boundary continuation.
- [`fftshift()`](fftshift.md) re-centers already computed frequency bins.
