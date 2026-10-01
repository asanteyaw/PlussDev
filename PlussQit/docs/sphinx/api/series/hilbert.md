<!-- pluss-hand-authored -->

# Series::hilbert

[Back to Series API Reference](../../series_api_reference)

## Signatures

```cpp
Series Series::hilbert() const
Series Series::hilbert(std::int64_t dim) const
```

## Build An Analytic Signal Along One Axis

`hilbert()` converts each selected one-dimensional slice into its analytic
signal. It transforms the slice to the frequency domain, retains the DC term,
doubles strictly positive frequencies, retains the Nyquist term when the
length is even, removes negative frequencies, and transforms back. For a real
receiver, the real part reconstructs the original samples and the imaginary
part is their discrete Hilbert transform.

This is useful when a real oscillatory signal needs a complex representation
whose magnitude and angle can be interpreted as an envelope and instantaneous
phase. It is not a frequency-bin result; the returned values live in the
original sample domain.

The no-argument form transforms the last axis. `hilbert(dim)` selects another
axis, and negative axes are normalized in the usual way. Every slice across
the remaining axes is processed independently. Rank and shape are preserved.
The selected axis must contain at least one element.

The output dtype is `complex128`, including for integral and lower-precision
input. A complex receiver is also accepted; in that case the operation is the
one-sided spectral projection itself, and its real part need not reproduce the
input. The output is newly allocated and contiguous.

The transform uses a batched CPU FFT bridge and returns the result to the
receiver's platform. Work is proportional to the number of independent slices
times `N log N` for a transformed length `N`.

## Reverse Mode

The analytic projection is linear, and its real frequency mask is
self-adjoint under the transform convention used here. Reverse mode therefore
applies the same projection to the incoming sensitivity. When the receiver is
real, only the real component is accumulated into its sensitivity.

## Examples

Example 1. An even-length real signal becomes a complex analytic signal.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1.0, 2.0, 3.0, 4.0}).hilbert());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=complex128, platform=cpu, numel=4, contiguous=true)
data=[(1.0000+1.0000j), (2.0000-1.0000j), (3.0000-1.0000j), (4.0000+1.0000j)]
```

Example 2. Odd lengths have no Nyquist singleton, so their imaginary pattern
differs from the even-length case.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}).hilbert());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=complex128, platform=cpu, numel=5, contiguous=true)
data=[(1.0000+1.7013j), (2.0000-1.3764j), (3.0000-0.6498j), (4.0000-1.3764j), (5.0000+1.7013j)]
```

Example 3. Omitting `dim` processes each row along the last axis.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
pluss::series::print(std::cout, x.hilbert());
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=complex128, platform=cpu, numel=6, contiguous=true)
data=
⎡ (1.0000+0.5774j) (2.0000-1.1547j) (3.0000+0.5774j) ⎤
⎣ (4.0000+0.5774j) (5.0000-1.1547j) (6.0000+0.5774j) ⎦
```

Example 4. `dim=0` processes each column. A two-sample real slice contains
only DC and Nyquist bins, so no quadrature component is introduced.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
pluss::series::print(std::cout, x.hilbert(0));
```

### Output

```text
Series(shape=[2,3], strides=[3,1], dtype=complex128, platform=cpu, numel=6, contiguous=true)
data=
⎡ (1.0000+0.0000j) (2.0000+0.0000j) (3.0000+0.0000j) ⎤
⎣ (4.0000+0.0000j) (5.0000+0.0000j) (6.0000+0.0000j) ⎦
```

Example 5. Negative axes identify the same dimension.

```cpp
auto x = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
std::cout << std::boolalpha
          << x.hilbert(-1).allclose(x.hilbert(1), 1e-12, 1e-12);
```

### Output

```text
true
```

Example 6. For real input, the real component recovers the samples to
transform precision.

```cpp
pluss::series::print(
    std::cout,
    pluss::make_series({1.0, 2.0, 3.0, 4.0}).hilbert().real());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.0000, 2.0000, 3.0000, 4.0000]
```

Example 7. A complex receiver is treated as a general one-sided spectral
projection.

```cpp
using Complex = std::complex<double>;
auto x = pluss::make_series(
    std::vector<Complex>{{1.0, 1.0}, {2.0, 0.0},
                         {3.0, -1.0}, {4.0, 0.0}},
    {4});
pluss::series::print(std::cout, x.hilbert());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=complex128, platform=cpu, numel=4, contiguous=true)
data=[(1.0000+2.0000j), (1.0000-1.0000j), (3.0000-2.0000j), (5.0000+1.0000j)]
```

Example 8. Reverse mode through the real projection returns the output weights
to the corresponding real samples.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
x.tracks_slope_(true);
x.hilbert()
 .real()
 .mul(pluss::make_series({1.0, 2.0, 3.0, 4.0}))
 .sum()
 .reverse();
pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.0000, 2.0000, 3.0000, 4.0000]
```

Example 9. The selected axis must exist.

```cpp
try {
  (void)pluss::make_series({{1.0, 2.0}, {3.0, 4.0}}).hilbert(2);
} catch (const std::exception& e) {
  const std::string message = e.what();
  std::cout << message.substr(0, message.find('\n'));
}
```

### Output

```text
hilbert: dim out of range
```

## See Also

- [`angle()`](angle.md) extracts phase from the analytic signal.
- [`abs()`](abs.md) extracts its magnitude envelope.
- [`stft()`](stft.md) resolves changing frequency content into overlapping frames.
