<!-- pluss-hand-authored -->

# Series::ifft

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::ifft(std::int64_t n = -1,
                    std::int64_t dim = -1,
                    const std::string& norm = "backward") const
```

## What It Computes

`ifft()` is the inverse discrete Fourier transform along one axis. For a spectrum
`X` of length `n`, the result has entries

```text
x[j] = (1/n) * sum over k = 0 .. n-1 of  X[k] * exp(+2 pi i j k / n),      j = 0 .. n-1
```

It undoes [`fft()`](fft.md) when both use the same `norm`: for every input,
`x.fft().ifft()` equals `x` to rounding, and `X.ifft().fft()` equals `X`. The
input is read as a spectrum in the layout `fft()` produces, with zero frequency
at index 0 and the negative frequencies wrapped into the upper half.

The result is always complex. When the spectrum is conjugate-symmetric (entry
`n - k` the conjugate of entry `k`), which is what `fft()` of a real signal
produces, the imaginary part of the result is zero to rounding, and
[`real()`](real.md) recovers the real signal. For a spectrum that is stored as
its one-sided half, [`irfft()`](irfft.md) rebuilds the symmetry and returns the
real signal directly.

The algorithm, its accuracy and its cost are those of [`fft()`](fft.md):
`O(n log n)` for every length, powers of two by radix-2 and other lengths by a
chirp-z convolution, with accuracy near `1e-15` in double precision at every
length.

## Parameters

- **`n`** is the transform length along `dim`. The default `-1` uses the length
  of the input. A different positive `n` first resizes the *spectrum*: a smaller
  `n` keeps its first `n` entries, a larger `n` appends zeros after the last
  entry. Because the negative frequencies sit at the end of the layout, appending
  zeros is not the same as adding high frequencies in the middle: it does not
  interpolate a signal. `n` must be positive.
- **`dim`** is the axis to transform; negative values count from the end, and the
  default is the last axis. All other axes are batch axes. The input must have at
  least one dimension, and non-contiguous inputs are accepted.
- **`norm`** selects the scaling and must match the call that produced the
  spectrum.

| `norm` | `ifft()` scales by | The matching `fft()` scales by |
| --- | --- | --- |
| `"backward"` (default) | `1/n` | `1` |
| `"forward"` | `1` | `1/n` |
| `"ortho"` | `1/sqrt(n)` | `1/sqrt(n)` |

Any other string, including the empty string, is rejected.

## Dtype And Shape

The output has the shape of the input with `dim` resized to `n`. `Complex128` and
`Float64` inputs give `Complex128`; every other input (`Float32`, `Complex64`,
`Bool`, the integers) gives `Complex64`. `Float16` and `BFloat16` input is
rejected; convert it to `Float32` first. A real input is
read as a spectrum with zero imaginary parts.

## Reverse Mode

`ifft()` is linear, and its reverse rule is the adjoint transform: the incoming
sensitivity is passed through the forward transform and divided by `n` (for the
default normalization). Gradients flow through the resizing implied by `n` and
through the `norm` scaling. A complex input receives the sensitivity
`dL/dRe + i dL/dIm` of a real loss `L`.

## Examples

Example 1. The round trip. A transform and its inverse return the original
samples; the result is complex with a zero imaginary part.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
auto back = x.fft().ifft();
pluss::series::print(std::cout, back);
std::cout << "\n" << std::boolalpha << "equals x: " << back.real().equal(x) << "\n";
```

### Output

```text
Series(shape=[4], strides=[1], dtype=complex128, platform=cpu, numel=4, contiguous=true)
data=[(1.0000+0.0000j), (2.0000+0.0000j), (3.0000+0.0000j), (4.0000+0.0000j)]
equals x: true
```

Example 2. Low-pass filtering. The signal alternates between 3 and 1: a constant
level 2 plus an oscillation at the highest frequency. Zeroing the highest bin
removes the oscillation and leaves the level.

```cpp
auto x = pluss::make_series({3.0, 1.0, 3.0, 1.0, 3.0, 1.0, 3.0, 1.0});
auto spectrum = x.fft();
auto mask = pluss::make_series({1.0, 1.0, 1.0, 1.0, 0.0, 1.0, 1.0, 1.0});
pluss::series::print(std::cout, spectrum.mul(mask).ifft().real());
```

### Output

```text
Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[2.0000, 2.0000, 2.0000, 2.0000, 2.0000, 2.0000, 2.0000, 2.0000]
```

Example 3. The convolution theorem. Multiplying two spectra and inverting gives
the circular convolution of the signals: `c[j] = sum_m a[m] b[(j - m) mod n]`.

```cpp
auto a = pluss::make_series({1.0, 2.0, 3.0, 0.0});
auto b = pluss::make_series({0.0, 1.0, 0.5, 0.0});
auto fast = a.fft().mul(b.fft()).ifft().real();
pluss::series::print(std::cout, fast);

std::vector<double> direct(4, 0.0);
const std::vector<double> av{1.0, 2.0, 3.0, 0.0}, bv{0.0, 1.0, 0.5, 0.0};
for (int j = 0; j < 4; ++j)
  for (int m = 0; m < 4; ++m) direct[j] += av[m] * bv[((j - m) % 4 + 4) % 4];
std::cout << "\ndirect sum: " << direct[0] << " " << direct[1] << " " << direct[2] << " " << direct[3] << "\n";
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.5000, 1.0000, 2.5000, 4.0000]
direct sum: 1.5 1 2.5 4
```

Example 4. Matching normalizations. With `"ortho"` the pair undoes itself; if
the two calls disagree the result is off by a power of `n`. Here `"ortho"` followed by the default scales the result by `1/sqrt(n)`, which for four samples halves it.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
pluss::series::print(std::cout, x.fft(-1, -1, "ortho").ifft(-1, -1, "ortho").real());
std::cout << "\n";
pluss::series::print(std::cout, x.fft(-1, -1, "ortho").ifft().real());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.0000, 2.0000, 3.0000, 4.0000]
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.5000, 1.0000, 1.5000, 2.0000]
```

Example 5. A conjugate-symmetric spectrum gives a real signal. The imaginary part
of the result is zero to rounding, which is why [`real()`](real.md) is the
natural last step.

```cpp
auto x = pluss::make_series({0.5, -1.25, 2.0, 3.5, -0.75, 1.0, 0.25});
auto back = x.fft().ifft();
std::cout << std::boolalpha;
std::cout << "imaginary part below 1e-15: " << (back.imag().abs().max().item<double>() < 1e-15) << "\n";
std::cout << "real part matches within 1e-15: " << (back.real().sub(x).abs().max().item<double>() < 1e-15) << "\n";
```

### Output

```text
imaginary part below 1e-15: true
real part matches within 1e-15: true
```

Example 6. Resizing the spectrum with `n` changes the length of the result, here
by truncation to the first three entries and by padding to six.

```cpp
auto spectrum = pluss::make_series({4.0, 1.0, 0.0, 1.0});
std::cout << "shape with n = 3: [" << spectrum.ifft(3).size(0) << "]\n";
std::cout << "shape with n = 6: [" << spectrum.ifft(6).size(0) << "]\n";
pluss::series::print(std::cout, spectrum.ifft(6).real());
```

### Output

```text
shape with n = 3: [3]
shape with n = 6: [6]
Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[1.0000, 0.5833, 0.7500, 0.3333, 0.7500, 0.5833]
```

Example 7. Axes. The inverse acts along `dim`; here it undoes a transform of the
columns of a matrix.

```cpp
auto m = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}, {5.0, 7.0}});
auto columns = m.fft(-1, 0);
pluss::series::print(std::cout, columns.ifft(-1, 0).real());
```

### Output

```text
Series(shape=[3,2], strides=[2,1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=
⎡ 1.0000 2.0000 ⎤
⎢ 3.0000 4.0000 ⎥
⎣ 5.0000 7.0000 ⎦
```

Example 8. Reverse mode. The sum of the real part of `ifft(X)` over all entries
equals `X[0]`, so its gradient with respect to the complex spectrum is `1` at
entry 0 and `0` elsewhere.

```cpp
auto spectrum = pluss::make_series(
    std::vector<std::complex<double>>{{3, 1}, {-1, 2}, {0.5, 0}, {2, -2}}, {4});
spectrum.tracks_slope_(true);
spectrum.ifft().real().sum().reverse();
pluss::series::print(std::cout, spectrum.sensitivity());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=complex128, platform=cpu, numel=4, contiguous=true)
data=[(1.0000+0.0000j), (0.0000+0.0000j), (0.0000+0.0000j), (0.0000+0.0000j)]
```

Example 9. Invalid requests are rejected with a message that names the problem.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
for (auto call : std::vector<std::function<pluss::Series()>>{
         [&] { return x.ifft(0); },
         [&] { return x.ifft(-1, -2); },
         [&] { return x.ifft(-1, -1, "unitary"); }}) {
  try {
    (void)call();
  } catch (const std::exception& e) {
    const std::string message = e.what();
    std::cout << message.substr(0, message.find('\n')) << "\n";
  }
}
```

### Output

```text
ifft: transform length must be greater than zero
ifft: dim out of range
ifft: norm must be 'backward', 'forward', or 'ortho'
```

## Gotchas And Failure Modes

- **The result is complex.** Take [`real()`](real.md) when the spectrum is known to
  be conjugate-symmetric; the leftover imaginary part is rounding, not signal.
- **A modified spectrum may no longer be symmetric.** Masking entry `k` but not
  entry `n - k` produces a complex signal. Filter both halves of the spectrum
  together, or work with [`rfft()`](rfft.md) and [`irfft()`](irfft.md), which
  keep the symmetry by construction.
- **The two normalization strings must match.** A mismatch scales the result by
  `n`, `1/n` or `sqrt(n)` (as in Example 4) and raises no error.
- **`n` resizes the spectrum, not the signal.** Appending zeros to a spectrum does
  not interpolate the signal; to refine a signal, pad the signal before
  [`fft()`](fft.md) instead.
- **The layout is positional.** The input must have zero frequency at index 0.
  A spectrum that was centred with [`fftshift()`](fftshift.md) must go through
  [`ifftshift()`](ifftshift.md) first.

## Summary

`ifft()` returns the signal whose [`fft()`](fft.md) is the input, along `dim`,
for every length in `O(n log n)`, with the scaling chosen by `norm`. The result
is complex and real to rounding for symmetric spectra. It is differentiable.

## See Also

- [`fft()`](fft.md) is the forward transform.
- [`irfft()`](irfft.md) inverts a one-sided spectrum to a real signal.
- [`ifftshift()`](ifftshift.md) restores index-zero layout before inversion.
- [`istft()`](istft.md) inverts a short-time transform by overlap-add.
- [`idct()`](idct.md) and [`idst()`](idst.md) invert the real-valued transforms.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="idst.html">
    <span class="label">Previous</span>
    <span class="title">idst</span>
  </a>
  <a class="next-link" href="ifftshift.html">
    <span class="label">Next</span>
    <span class="title">ifftshift</span>
  </a>
</div>
