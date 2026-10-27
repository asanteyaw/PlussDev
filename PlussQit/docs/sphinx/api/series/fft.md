<!-- pluss-hand-authored -->

# Series::fft

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::fft(std::int64_t n = -1,
                   std::int64_t dim = -1,
                   const std::string& norm = "backward") const
```

## What It Computes

`fft()` is the discrete Fourier transform along one axis. For a signal `x` of
length `n`, the result has entries

```text
X[k] = sum over j = 0 .. n-1 of  x[j] * exp(-2 pi i j k / n),      k = 0 .. n-1
```

Entry `k` measures how much of the signal oscillates `k` times over the `n`
samples. Entry `0` is the sum of the samples (the zero-frequency or DC term).
Entries `1` to `n/2` are the positive frequencies. The remaining entries hold
the negative frequencies in wrapped order: entry `n - k` is the frequency `-k`.
[`fftshift()`](fftshift.md) reorders an axis so that zero frequency sits in the
middle when a centred display or a centred filter is wanted.

The result is always complex, even for real input. Every transform is computed
in `O(n log n)` operations, for every length: powers of two use an iterative
radix-2 algorithm, and any other length, including primes, uses a chirp-z
(Bluestein) convolution built on the same radix-2 core. Twiddle factors and
chirp sequences are computed directly rather than by repeated multiplication,
and the chirp exponent is reduced exactly modulo `2n`, so accuracy does not
degrade with the length. Measured against an independent double-precision
implementation across lengths from 1 to a million, the relative error stays near
`1e-15` for double precision and near `4e-7` for single precision.

## Parameters

- **`n`** is the transform length along `dim`. The default `-1` uses the length
  the input already has. A different positive `n` first resizes the axis: if
  `n` is smaller, only the first `n` samples are used; if `n` is larger, the
  signal is padded with zeros at the end. Padding does not add information, but
  it samples the spectrum more finely, which is the usual way to interpolate a
  spectrum or to reach a length that suits other code. `n` must be positive.
- **`dim`** is the axis to transform. Negative values count from the last axis,
  and the default is the last axis. Every other axis is a batch axis: each lane
  along `dim` is transformed independently, in parallel. The input must have at
  least one dimension. Inputs that are not contiguous, such as a transposed
  matrix, are accepted.
- **`norm`** selects where the factor `1/n` goes. The same string must be given
  to [`ifft()`](ifft.md) for the pair to undo each other.

| `norm` | `fft()` scales by | `ifft()` scales by | Use it when |
| --- | --- | --- | --- |
| `"backward"` (default) | `1` | `1/n` | the spectrum should be the plain sum above |
| `"forward"` | `1/n` | `1` | entry 0 should be the mean of the signal |
| `"ortho"` | `1/sqrt(n)` | `1/sqrt(n)` | the transform should preserve energy (it is then unitary) |

Any other string, including the empty string, is rejected.

## Dtype And Shape

The output has the shape of the input, with `dim` resized to `n`. The output
dtype follows the input: `Float64` and `Complex128` give `Complex128`; every
other input, including `Float32`, `Bool` and the integer types, gives
`Complex64`. `Float16` and `BFloat16` input is rejected; convert it to `Float32`
first. Complex input is transformed as given; real input is
treated as a signal with zero imaginary part.

For real input the spectrum is conjugate-symmetric, `X[n-k] = conj(X[k])`. The
result is built so that this holds exactly, not just to rounding: the zero
frequency, and for even `n` the Nyquist entry `n/2`, have an imaginary part of
exactly zero, and the entries above `n/2` are exact conjugates of those below.
If only the non-redundant half is needed, [`rfft()`](rfft.md) returns it.

Small integer data transforms to exact small integers, because the trigonometric
factors at multiples of a quarter turn are exactly `0` and `±1`.

## Platform

The transform runs on the host. A tensor on another device is copied to the host
and the result is moved back. A device without complex storage cannot receive the
result: on MPS the call fails while converting to a complex dtype, and the input
should be moved to the CPU first.

## Reverse Mode

`fft()` is linear, so its reverse rule is the adjoint transform: the incoming
sensitivity is passed through the inverse transform and multiplied by `n`
(for the default normalization). Gradients flow through the resizing implied by
`n` and through the `norm` scaling. A real input receives a real sensitivity; a
complex input receives the sensitivity `dL/dRe + i dL/dIm` of a real loss `L`.

## Examples

Example 1. The transform of four samples. The entries are exact: entry 0 is the
sum, and the rest follow from the quarter-turn factors.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
pluss::series::print(std::cout, x.fft());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=complex128, platform=cpu, numel=4, contiguous=true)
data=[(10.0000+0.0000j), (-2.0000+2.0000j), (-2.0000+0.0000j), (-2.0000-2.0000j)]
```

Example 2. A pure oscillation lands in one pair of bins. This signal completes
two cycles over eight samples, so its energy is in bins 2 and 6 (frequencies
`+2` and `-2`). For a sine of unit amplitude the imaginary parts are `-n/2` and
`+n/2`, and every real part is zero.

```cpp
auto wave = pluss::make_series({0.0, 1.0, 0.0, -1.0, 0.0, 1.0, 0.0, -1.0});
auto spectrum = wave.fft();
pluss::series::print(std::cout, spectrum.imag());
std::cout << "\nlargest real part: " << spectrum.real().abs().max().item<double>() << "\n";
```

### Output

```text
Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[0.0000, 0.0000, -4.0000, 0.0000, 0.0000, -0.0000, 4.0000, -0.0000]
largest real part: 0
```

Example 3. Resizing with `n`. A smaller `n` keeps the leading samples; a larger
`n` pads with zeros. Both resize the signal before transforming.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
std::cout << "n = 2 matches the transform of the first two samples: "
          << std::boolalpha
          << x.fft(2).equal(pluss::make_series({1.0, 2.0}).fft()) << "\n";
pluss::series::print(std::cout, x.fft(6).abs());
```

### Output

```text
n = 2 matches the transform of the first two samples: true
Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[10.0000, 5.5678, 2.6458, 2.0000, 2.6458, 5.5678]
```

Example 4. The three normalizations on the same input. `"forward"` makes entry 0
the mean, and `"ortho"` preserves the sum of squared magnitudes (Parseval).

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
pluss::series::print(std::cout, x.fft(-1, -1, "backward").real());
std::cout << "\n";
pluss::series::print(std::cout, x.fft(-1, -1, "forward").real());
std::cout << "\n";
pluss::series::print(std::cout, x.fft(-1, -1, "ortho").real());
auto ortho = x.fft(-1, -1, "ortho");
std::cout << "\nenergy of the signal:   " << x.square().sum().item<double>() << "\n";
std::cout << "energy of the spectrum: " << ortho.abs().square().sum().item<double>() << "\n";
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10.0000, -2.0000, -2.0000, -2.0000]
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[2.5000, -0.5000, -0.5000, -0.5000]
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[5.0000, -1.0000, -1.0000, -1.0000]
energy of the signal:   30
energy of the spectrum: 30
```

Example 5. A length that is not a power of two. Six samples are transformed
exactly as four are. The result is conjugate-symmetric, so entry 5 is the
conjugate of entry 1, and entry 3 (the Nyquist entry) is real.

```cpp
auto x = pluss::make_series({1.0, 0.0, -1.0, 0.0, 2.0, 5.0});
auto spectrum = x.fft().host<std::complex<double>>();
std::cout << std::boolalpha;
std::cout << "entry 5 is the conjugate of entry 1: " << (spectrum[5] == std::conj(spectrum[1])) << "\n";
std::cout << "entry 3 has zero imaginary part:     " << (spectrum[3].imag() == 0.0) << "\n";
pluss::series::print(std::cout, x.fft());
```

### Output

```text
entry 5 is the conjugate of entry 1: true
entry 3 has zero imaginary part:     true
Series(shape=[6], strides=[1], dtype=complex128, platform=cpu, numel=6, contiguous=true)
data=[(7.0000+0.0000j), (3.0000+6.9282j), (-2.0000+1.7321j), (-3.0000+0.0000j), (-2.0000-1.7321j), (3.0000-6.9282j)]
```

Example 6. Axes and batches. Each row of a matrix is its own signal along the last
axis; `dim = 0` transforms the columns instead.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0, 4.0}, {5.0, 6.0, 7.0, 8.0}});
pluss::series::print(std::cout, m.fft(-1, -1).real());
std::cout << "\n";
pluss::series::print(std::cout, m.fft(-1, 0).real());
```

### Output

```text
Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=
⎡ 10.0000 -2.0000 -2.0000 -2.0000 ⎤
⎣ 26.0000 -2.0000 -2.0000 -2.0000 ⎦
Series(shape=[2,4], strides=[4,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=
⎡  6.0000  8.0000  10.0000  12.0000 ⎤
⎣ -4.0000 -4.0000  -4.0000  -4.0000 ⎦
```

Example 7. Dtypes. Double precision gives `Complex128`, single precision and
integers give `Complex64`, and complex input keeps its precision.

```cpp
auto f64 = pluss::make_series({1.0, 2.0, 3.0});
auto f32 = f64.to(pluss::ScalarType::Float32);
auto i64 = pluss::make_series(std::vector<std::int64_t>{1, 2, 3}, {3});
using pluss::ScalarType;
std::cout << std::boolalpha;
std::cout << "float64 -> complex128:    " << (f64.fft().scalar_type() == ScalarType::Complex128) << "\n";
std::cout << "float32 -> complex64:     " << (f32.fft().scalar_type() == ScalarType::Complex64) << "\n";
std::cout << "int64 -> complex64:       " << (i64.fft().scalar_type() == ScalarType::Complex64) << "\n";
std::cout << "complex64 stays complex64: " << (f32.fft().fft().scalar_type() == ScalarType::Complex64) << "\n";
```

### Output

```text
float64 -> complex128:    true
float32 -> complex64:     true
int64 -> complex64:       true
complex64 stays complex64: true
```

Example 8. Reverse mode. By Parseval, the loss `sum(|X|^2)` equals `n * sum(x^2)`
for the plain transform, so its gradient with respect to `x` is `2 n x`.

```cpp
auto x = pluss::make_series({1.0, -2.0, 0.5, 3.0, 4.0});
x.tracks_slope_(true);
auto spectrum = x.fft();
spectrum.real().square().sum().add(spectrum.imag().square().sum()).reverse();
pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[10.0000, -20.0000, 5.0000, 30.0000, 40.0000]
```

Example 9. Invalid requests are rejected with a message that names the problem.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
for (auto call : std::vector<std::function<pluss::Series()>>{
         [&] { return x.fft(0); },
         [&] { return x.fft(-1, 2); },
         [&] { return x.fft(-1, -1, "unitary"); },
         [&] { return pluss::make_series(1.0).fft(); }}) {
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
fft: transform length must be greater than zero
fft: dim out of range
fft: norm must be 'backward', 'forward', or 'ortho'
fft: input must have at least one dimension
```

## Gotchas And Failure Modes

- **Truncation drops the end, not the middle.** `fft(n)` with a smaller `n` keeps
  the first `n` samples. To transform a different window, slice first.
- **The two normalization strings must match.** `fft()` with `"ortho"` is undone
  only by `ifft()` with `"ortho"`; mixing them scales the result by a power of `n`
  (`n`, `1/n` or `sqrt(n)`) and raises no error.
- **Bin `k` is not a frequency in hertz.** It is `k` cycles per `n` samples. For
  a sampling interval `dt`, bin `k` is `k / (n dt)` hertz, with `k` above `n/2`
  meaning `k - n`.
- **Padding refines, it does not resolve.** Zero-padding to a larger `n` samples
  the same underlying spectrum more densely; two close frequencies do not
  separate because the padded transform has more bins.
- **Real input yields redundant output.** About half the entries repeat as
  conjugates. [`rfft()`](rfft.md) returns only the non-redundant half and is the
  natural pair of [`irfft()`](irfft.md).

## Summary

`fft()` returns the complex spectrum of each lane along `dim`, of length `n`,
for every length in `O(n log n)`, with the normalization chosen by `norm` and
exact conjugate symmetry for real input. It is differentiable.

## See Also

- [`ifft()`](ifft.md) is the inverse and must use the same `norm`.
- [`rfft()`](rfft.md) and [`irfft()`](irfft.md) are the real-signal forms.
- [`fftshift()`](fftshift.md) moves zero frequency to the centre of an axis.
- [`stft()`](stft.md) applies the transform to overlapping windows of a signal.
- [`dct()`](dct.md) and [`dst()`](dst.md) compute real-valued transforms.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="factorial.html">
    <span class="label">Previous</span>
    <span class="title">factorial</span>
  </a>
  <a class="next-link" href="fftshift.html">
    <span class="label">Next</span>
    <span class="title">fftshift</span>
  </a>
</div>
