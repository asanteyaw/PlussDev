<!-- pluss-hand-authored -->

# Series::irfft

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::irfft(std::int64_t n = -1,
                     std::int64_t dim = -1,
                     const std::string& norm = "backward") const
```

## What It Computes

`irfft()` is the inverse of [`rfft()`](rfft.md): it takes the one-sided spectrum
of a real signal and returns the **real** signal. The input holds the entries for
frequencies `0` to `n/2`. The missing entries, for the negative frequencies, are
rebuilt from the conjugate symmetry of a real signal, `X[n - k] = conj(X[k])`,
and the full spectrum is then inverted:

```text
x[j] = (1/n) * sum over k = 0 .. n-1 of  X[k] * exp(+2 pi i j k / n),      j = 0 .. n-1
```

The result is real by construction, with no leftover imaginary part to discard.
For every real signal `x` of length `n`, `x.rfft().irfft(n)` equals `x` to
rounding.

Because a one-sided spectrum has `n/2 + 1` entries for both `n = 2m` and
`n = 2m + 1`, the output length cannot be recovered from the input alone. The
default assumes the even length `2 (m - 1)` for an input of `m` entries. For an
odd length, pass `n` explicitly (Example 2).

Two entries of a one-sided spectrum are constrained to be real: the zero-frequency
entry, and for even `n` the Nyquist entry `n/2`. A real signal cannot have an
imaginary part there. If the input carries one anyway, `irfft()` ignores it
(Example 3); this is the one place where the output is not a function of the
whole input.

The cost is `O(n log n)` for every length, with the algorithm and accuracy
described on [`fft()`](fft.md).

## Parameters

- **`n`** is the length of the **output** along `dim`. The default `-1` means
  `2 * (m - 1)` for an input of `m` entries. Any positive `n` is allowed. The
  input is resized to the `n/2 + 1` entries that `n` needs: extra entries at the
  end are dropped, and missing ones are filled with zeros. Padding the one-sided
  spectrum with zeros at the high-frequency end adds frequencies that are absent,
  so it interpolates the signal (Example 5). `n` must be positive, so a
  one-entry input needs an explicit `n`.
- **`dim`** is the axis to transform; negative values count from the end, and the
  default is the last axis. All other axes are batch axes. The input must have at
  least one dimension.
- **`norm`** selects the scaling and must match the [`rfft()`](rfft.md) call that
  produced the spectrum: `"backward"` (default) scales by `1/n`, `"forward"` by
  `1`, and `"ortho"` by `1/sqrt(n)`. Any other string, including the
  empty string, is rejected.

## Dtype And Shape

The input may be real or complex; a real input is read as a spectrum with zero
imaginary parts. The output is real: `Float64` for `Float64` or `Complex128`
input, and `Float32` for every other input. `Float16` and `BFloat16` input is
rejected; convert it to `Float32` first. The output shape is the input shape
with `dim` resized to `n`.

## Reverse Mode

`irfft()` is linear in the real and imaginary parts of the input, and its reverse
rule is the adjoint of the rebuild-and-invert map. Interior entries, which are
counted twice in the reconstruction, receive twice the weight; the
zero-frequency entry and the even-length Nyquist entry are counted once, and
their imaginary parts, which are ignored, receive no sensitivity. A complex input
receives the sensitivity `dL/dRe + i dL/dIm` of a real loss `L`.

## Examples

Example 1. The round trip for an even length. Six samples give a four-entry
spectrum, and the default output length `2 * (4 - 1)` is six.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
auto back = x.rfft().irfft();
pluss::series::print(std::cout, back);
std::cout << "\n" << std::boolalpha << "matches x within 1e-14: "
          << (back.sub(x).abs().max().item<double>() < 1e-14) << "\n";
```

### Output

```text
Series(shape=[6], strides=[1], dtype=float64, platform=cpu, numel=6, contiguous=true)
data=[1.0000, 2.0000, 3.0000, 4.0000, 5.0000, 6.0000]
matches x within 1e-14: true
```

Example 2. An odd length needs `n`. Five samples also give a three-entry spectrum,
and the default guesses the even length four, which is wrong. Passing the original
length restores the signal.

```cpp
auto x = pluss::make_series({2.0, -1.0, 0.5, 4.0, -3.0});
auto spectrum = x.rfft();
std::cout << "default length: " << spectrum.irfft().size(0) << "\n";
pluss::series::print(std::cout, spectrum.irfft(5));
```

### Output

```text
default length: 4
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[2.0000, -1.0000, 0.5000, 4.0000, -3.0000]
```

Example 3. An imaginary part in the zero-frequency entry is ignored. The two
inputs differ only there, and give the same signal.

```cpp
using C = std::complex<double>;
auto with_imag = pluss::make_series(std::vector<C>{{1, 3}, {2, 1}, {4, 5}}, {3});
auto without = pluss::make_series(std::vector<C>{{1, 0}, {2, 1}, {4, 0}}, {3});
pluss::series::print(std::cout, with_imag.irfft());
std::cout << "\n" << std::boolalpha << "same as the real-entry version: "
          << with_imag.irfft().equal(without.irfft()) << "\n";
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[2.2500, -1.2500, 0.2500, -0.2500]
same as the real-entry version: true
```

Example 4. Filtering a real signal without leaving the real domain. The signal
alternates between 3 and 1: a level of 2 plus an oscillation at the highest
frequency. Zeroing the last entry of the one-sided spectrum removes it.

```cpp
auto x = pluss::make_series({3.0, 1.0, 3.0, 1.0, 3.0, 1.0, 3.0, 1.0});
auto spectrum = x.rfft();
auto keep = pluss::make_series({1.0, 1.0, 1.0, 1.0, 0.0});
pluss::series::print(std::cout, spectrum.mul(keep).irfft());
```

### Output

```text
Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[2.0000, 2.0000, 2.0000, 2.0000, 2.0000, 2.0000, 2.0000, 2.0000]
```

Example 5. Interpolation by padding the spectrum. Four samples are transformed
with `"forward"` normalization (so the entries are means), then inverted at length
eight with the same normalization. The signal has no Nyquist component, so the
samples reappear at the even positions with new values between them.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 2.0});
auto spectrum = x.rfft(-1, -1, "forward");
pluss::series::print(std::cout, spectrum.irfft(8, -1, "forward"));
```

### Output

```text
Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[1.0000, 1.2929, 2.0000, 2.7071, 3.0000, 2.7071, 2.0000, 1.2929]
```

Example 6. The scaling pairs with the forward call. With `"ortho"` on both sides
the round trip is exact; with mismatched strings the result is scaled.

```cpp
auto x = pluss::make_series({1.0, 2.0, 3.0, 4.0});
pluss::series::print(std::cout, x.rfft(-1, -1, "ortho").irfft(-1, -1, "ortho"));
std::cout << "\n";
pluss::series::print(std::cout, x.rfft(-1, -1, "ortho").irfft());
```

### Output

```text
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1.0000, 2.0000, 3.0000, 4.0000]
Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[0.5000, 1.0000, 1.5000, 2.0000]
```

Example 7. Axes. The inverse acts along `dim`; here it undoes a transform of the
columns.

```cpp
auto m = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}, {5.0, 7.0}, {2.0, 0.0}});
pluss::series::print(std::cout, m.rfft(-1, 0).irfft(4, 0));
```

### Output

```text
Series(shape=[4,2], strides=[2,1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=
⎡ 1.0000 2.0000 ⎤
⎢ 3.0000 4.0000 ⎥
⎢ 5.0000 7.0000 ⎥
⎣ 2.0000 0.0000 ⎦
```

Example 8. Reverse mode. The sum of all output samples equals the zero-frequency
entry, so the sensitivity of that sum is `1` at entry 0 and `0` elsewhere.

```cpp
auto spectrum = pluss::make_series(
    std::vector<std::complex<double>>{{3, 0}, {1, -2}, {0.5, 0}}, {3});
spectrum.tracks_slope_(true);
spectrum.irfft().sum().reverse();
pluss::series::print(std::cout, spectrum.sensitivity());
```

### Output

```text
Series(shape=[3], strides=[1], dtype=complex128, platform=cpu, numel=3, contiguous=true)
data=[(1.0000+0.0000j), (0.0000+0.0000j), (0.0000+0.0000j)]
```

Example 9. A one-entry input has no default length, and `n` must be positive.

```cpp
auto one = pluss::make_series(std::vector<std::complex<double>>{{5, 0}}, {1});
for (auto call : std::vector<std::function<pluss::Series()>>{
         [&] { return one.irfft(); },
         [&] { return one.irfft(0); }}) {
  try {
    (void)call();
  } catch (const std::exception& e) {
    const std::string message = e.what();
    std::cout << message.substr(0, message.find('\n')) << "\n";
  }
}
pluss::series::print(std::cout, one.irfft(1));
```

### Output

```text
irfft: transform length must be greater than zero
irfft: transform length must be greater than zero
Series(shape=[1], strides=[1], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=[5.0000]
```

## Gotchas And Failure Modes

- **Odd lengths need `n`.** The default returns an even length. Passing the
  original length is the only way to recover a signal of odd length from its
  one-sided spectrum.
- **A modified spectrum is still valid.** Whatever the one-sided entries are,
  the output is a real signal: the symmetry is imposed, not checked. Entries that
  are not consistent with any real signal (an imaginary zero-frequency entry)
  are partly ignored rather than rejected.
- **Padding with `n` adds frequencies, it does not shift them.** The Nyquist
  entry of an even-length spectrum is counted once in the original length and
  twice once it becomes an interior entry of a longer one, so Example 5 uses a
  signal without a Nyquist component to keep the samples exact.
- **The normalization strings must match.** A mismatch scales the result by a power
  of `n` (`n`, `1/n` or `sqrt(n)`) and raises no error.
- **The input must be in spectrum layout.** Zero frequency comes first; a
  spectrum centred with [`fftshift()`](fftshift.md) needs
  [`ifftshift()`](ifftshift.md) first.

## Summary

`irfft()` rebuilds the full conjugate-symmetric spectrum from the one-sided
half and returns the real signal of length `n` along `dim`, in `O(n log n)` for
every length. It is differentiable, pairs with [`rfft()`](rfft.md), and needs an
explicit `n` for odd lengths.

## See Also

- [`rfft()`](rfft.md) produces the one-sided spectrum.
- [`ifft()`](ifft.md) inverts a full spectrum to a complex signal.
- [`istft()`](istft.md) inverts a short-time transform by overlap-add.
- [`ifftshift()`](ifftshift.md) restores index-zero layout.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="invalid_index.html">
    <span class="label">Previous</span>
    <span class="title">invalid_index</span>
  </a>
  <a class="next-link" href="is_complex.html">
    <span class="label">Next</span>
    <span class="title">is_complex</span>
  </a>
</div>
