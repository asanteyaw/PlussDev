<!-- pluss-hand-authored -->

# Series::rfft

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::rfft(std::int64_t n = -1,
                    std::int64_t dim = -1,
                    const std::string& norm = "backward") const
```

## What It Computes

`rfft()` is the discrete Fourier transform of a **real** signal, returned as its
one-sided spectrum. The spectrum of a real signal is conjugate-symmetric:
entry `n - k` is the conjugate of entry `k`. Half of the full spectrum is
therefore redundant, and `rfft()` keeps only the non-redundant half, the entries
for frequencies `0` to `n/2`:

```text
X[k] = sum over j = 0 .. n-1 of  x[j] * exp(-2 pi i j k / n),      k = 0 .. n/2
```

The output length along `dim` is `n/2 + 1` using integer division: five entries
for eight samples, four entries for seven. The entries are identical to the first
`n/2 + 1` entries of [`fft()`](fft.md) of the same signal, including the exactness
of the zero-frequency entry (and, for even `n`, the Nyquist entry), whose
imaginary parts are exactly zero. [`irfft()`](irfft.md) is the inverse.

Use `rfft()` instead of `fft()` when the input is real and only the spectrum's
magnitude, phase or filtering is needed. The output is half the size, which
halves the memory of everything downstream, and it cannot be given a spectrum
that is not symmetric by construction. The computation itself costs about the
same as a complex transform of the same length: `O(n log n)` for every length,
with the algorithm and accuracy described on [`fft()`](fft.md).

## Parameters

- **`n`** is the signal length along `dim`. The default `-1` uses the length of
  the input. A smaller positive `n` keeps the first `n` samples; a larger `n`
  pads with zeros at the end. The output length is then `n/2 + 1`, not a function
  of the original length. `n` must be positive.
- **`dim`** is the axis to transform; negative values count from the end, and the
  default is the last axis. All other axes are batch axes, transformed
  independently. The input must have at least one dimension.
- **`norm`** selects the scaling: `"backward"` (default) leaves the plain sum,
  `"forward"` multiplies by `1/n` so entry 0 is the mean, and `"ortho"`
  multiplies by `1/sqrt(n)`. The scaling uses the signal length `n`, not the
  output length. Give [`irfft()`](irfft.md) the same string to invert. An empty
  string means `"backward"`; any other string is rejected.

## Dtype And Shape

The input must be real. A complex input is rejected, because its spectrum has no
conjugate symmetry to exploit; use [`fft()`](fft.md). The output is `Complex128`
for `Float64` input and `Complex64` for every other real input (`Float32`,
`Bool` and the integer types). `Float16` and `BFloat16` input is rejected;
convert it to `Float32` first. The output shape is the
input shape with `dim` resized to `n/2 + 1`.

## Reverse Mode

`rfft()` is linear, so its reverse rule is the adjoint of the real-to-one-sided
map: the incoming sensitivity is embedded back into a full-length spectrum,
passed through the inverse transform and scaled. The result is a real
sensitivity of the same shape as the input, matching the gradient of a real loss
computed through the equivalent [`fft()`](fft.md) call and sliced.

## Examples

Example 1. The output length is `n/2 + 1`: five entries for eight samples, four
for seven.

```cpp
auto even = pluss::series::arange(0, 8, 1, pluss::ScalarType::Float64);
auto odd = pluss::series::arange(0, 7, 1, pluss::ScalarType::Float64);
std::cout << "length 8 -> " << even.rfft().size(0) << " entries\n";
std::cout << "length 7 -> " << odd.rfft().size(0) << " entries\n";
```

### Output

```text
length 8 -> 5 entries
length 7 -> 4 entries
```

Example 2. The entries are the first half of the full spectrum, and the
zero-frequency entry is exactly real.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
auto half = x.rfft();
pluss::series::print(std::cout, half);
std::cout << "\n" << std::boolalpha
          << "equals the first four entries of fft(): " << half.equal(x.fft().slice(0, 0, 4)) << "\n";
```

### Output

```text
Series(shape=[4], strides=[1], dtype=complex128, platform=cpu, numel=4, contiguous=true)
data=[(21.0000+0.0000j), (-3.0000+5.1962j), (-3.0000+1.7321j), (-3.0000+0.0000j)]
equals the first four entries of fft(): true
```

Example 3. Reading an amplitude spectrum. A signal of two cosines, amplitude 3 at
2 cycles and amplitude 1 at 5 cycles over 16 samples. Dividing the magnitudes by
`n/2` recovers the amplitudes; every other bin is zero to rounding.

```cpp
const int n = 16;
std::vector<double> samples(n);
for (int j = 0; j < n; ++j) {
  samples[j] = 3.0 * std::cos(2.0 * 3.141592653589793 * 2 * j / n) +
               1.0 * std::cos(2.0 * 3.141592653589793 * 5 * j / n);
}
auto amplitude = pluss::make_series(samples).rfft().abs().mul(2.0 / n);
std::cout << std::fixed << std::setprecision(4);
std::cout << "bin 2: " << amplitude.select(0, 2).item<double>() << "\n";
std::cout << "bin 5: " << amplitude.select(0, 5).item<double>() << "\n";
std::vector<double> rest = amplitude.host<double>();
rest[2] = rest[5] = 0.0;
std::cout << "every other bin is below 1e-14: "
          << std::boolalpha << (*std::max_element(rest.begin(), rest.end()) < 1e-14) << "\n";
```

### Output

```text
bin 2: 3.0000
bin 5: 1.0000
every other bin is below 1e-14: true
```

Example 4. Bin to frequency. For a signal sampled at 100 Hz, bin `k` of an `n`
point transform is `k * 100 / n` hertz. A 10 Hz tone over 50 samples peaks in
bin 5.

```cpp
const int n = 50;
const double rate = 100.0;
std::vector<double> samples(n);
for (int j = 0; j < n; ++j) samples[j] = std::sin(2.0 * 3.141592653589793 * 10.0 * j / rate);
auto magnitude = pluss::make_series(samples).rfft().abs();
auto peak = magnitude.argmax().item<std::int64_t>();
std::cout << "peak bin: " << peak << ", frequency " << peak * rate / n << " Hz\n";
```

### Output

```text
peak bin: 5, frequency 10 Hz
```

Example 5. Resizing with `n`. Four samples padded to eight give a five-entry
spectrum; the padding refines the frequency grid.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
pluss::series::print(std::cout, x.rfft(8).abs());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[10.0000, 7.2545, 2.8284, 2.7153, 2.0000]
```

Example 6. Normalizations. With `"forward"`, entry 0 is the mean of the samples;
with `"ortho"`, entry 0 is the sum divided by `sqrt(n)`.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
std::cout << "forward, entry 0: " << x.rfft(-1, -1, "forward").real().select(0, 0).item<double>() << "\n";
std::cout << "ortho,   entry 0: " << x.rfft(-1, -1, "ortho").real().select(0, 0).item<double>() << "\n";
```

### Output

```text
forward, entry 0: 2.5
ortho,   entry 0: 5
```

Example 7. Axes and batches. Each row is a signal along the last axis; `dim = 0`
transforms the columns, and the resized axis is the one transformed.

```cpp
auto m = pluss::make_series({{1.0, 2.0, 3.0, 4.0}, {5.0, 6.0, 7.0, 8.0}, {1.0, 0.0, -1.0, 0.0}});
std::cout << "along the last axis: [" << m.rfft(-1, -1).size(0) << ", " << m.rfft(-1, -1).size(1) << "]\n";
std::cout << "along axis 0:        [" << m.rfft(-1, 0).size(0) << ", " << m.rfft(-1, 0).size(1) << "]\n";
```

### Output

```text
along the last axis: [3, 3]
along axis 0:        [2, 4]
```

Example 8. Reverse mode. The loss is the sum of the real parts of the three
entries. Entry 0 is `x0+x1+x2+x3`, entry 1 has real part `x0-x2`, and entry 2 is
`x0-x1+x2-x3`, so the gradient is `[3, 0, 1, 0]`.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
x.tracks_slope_(true);
x.rfft().real().sum().reverse();
pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[3.0000, 0.0000, 1.0000, 0.0000]
```

Example 9. A complex input and invalid lengths are rejected.

```cpp
auto real = pluss::make_series({1.0, 2.0, 3.0, 4.0});
auto complex = real.fft();
for (auto call : std::vector<std::function<pluss::Series()>>{
         [&] { return complex.rfft(); },
         [&] { return real.rfft(0); },
         [&] { return real.rfft(-1, -1, "unitary"); }}) {
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
rfft: input must be real-valued
rfft: transform length must be greater than zero
rfft: norm must be 'backward', 'forward', or 'ortho'
```

## Gotchas And Failure Modes

- **The output length depends on whether `n` is even or odd.** `n/2 + 1` is the
  same for `2m` and `2m + 1`. A one-sided spectrum alone therefore does not say
  which length it came from; pass the original length as `n` to
  [`irfft()`](irfft.md) when it is odd.
- **Complex input is an error, not a conversion.** To transform a complex signal
  use [`fft()`](fft.md); to analyse the real part of one, call
  [`real()`](real.md) first.
- **Amplitudes need a factor of two.** Each non-zero, non-Nyquist bin holds half of
  the amplitude of a cosine, the other half being in the discarded mirror bin
  (Example 3).
- **Padding refines, it does not resolve.** Zero-padding samples the same
  spectrum more densely; it does not separate close frequencies.

## Summary

`rfft()` returns the one-sided spectrum of a real signal, `n/2 + 1` entries along
`dim`, identical to the first half of [`fft()`](fft.md), with exactly real
zero-frequency and Nyquist entries. It is differentiable and pairs with
[`irfft()`](irfft.md).

## See Also

- [`irfft()`](irfft.md) inverts the one-sided spectrum.
- [`fft()`](fft.md) is the complex transform, for complex input or the full spectrum.
- [`stft()`](stft.md) applies the real transform to overlapping windows.
- [`fftshift()`](fftshift.md) centres the zero frequency of a full spectrum.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="reverse.html">
    <span class="label">Previous</span>
    <span class="title">reverse</span>
  </a>
  <a class="next-link" href="roll.html">
    <span class="label">Next</span>
    <span class="title">roll</span>
  </a>
</div>
