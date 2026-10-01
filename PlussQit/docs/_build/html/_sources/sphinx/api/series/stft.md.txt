<!-- pluss-hand-authored -->

# Series::stft

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::stft(
    std::int64_t n_fft,
    std::int64_t hop_length = -1,
    std::int64_t win_length = -1,
    const Series& window = {},
    bool center = true,
    const std::string& pad_mode = "reflect",
    bool normalized = false,
    bool onesided = true,
    bool return_complex = true) const
```

## Turning A Signal Into Local Spectra

`stft()` is for signals whose frequency content changes over time. A single
transform describes the receiver as one global mixture of frequencies;
`stft()` instead takes overlapping windows, transforms each window, and keeps
the frame positions. The result can therefore answer both *which frequencies
are present?* and *where in the signal are they present?*

For each frame `m` and frequency bin `k`, the method computes

$$
X[k,m] = \sum_{n=0}^{n_{fft}-1} x_m[n]w[n]
          e^{-2\pi i kn/n_{fft}}.
$$

Here `x_m` is the frame beginning `hop_length` samples after its predecessor,
and `w` is the supplied window after symmetric zero-padding to `n_fft`. The
method does not flatten a batch: a vector produces `[frequency, frame]`, while
a matrix shaped `[batch, time]` produces `[batch, frequency, frame]`.

## Parameters

| Parameter | Meaning | Default |
|---|---|---|
| `n_fft` | Transform length and number of bins in a full spectrum. Must be positive. | Required |
| `hop_length` | Distance in samples between adjacent frames. `-1` selects `n_fft / 4`; the resolved value must be positive. | `-1` |
| `win_length` | Number of signal samples weighted in each frame. `-1` selects `n_fft`; it must be positive and no greater than `n_fft`. | `-1` |
| `window` | Rank-one floating or complex weights of exactly `win_length` lanes. An undefined Series selects a rectangular window of ones. | Undefined |
| `center` | Pads both ends by `n_fft / 2`, making frame `m` centered near sample `m * hop_length`. | `true` |
| `pad_mode` | Boundary rule used only by centered analysis. Supported values are `"reflect"`, `"replicate"`, `"circular"`, and `"constant"`. | `"reflect"` |
| `normalized` | Divides every transform by `sqrt(n_fft)`. | `false` |
| `onesided` | Keeps only bins `0` through `n_fft / 2` for real input and a real window. | `true` |
| `return_complex` | Returns a complex Series when true; otherwise appends a final extent `2` containing real and imaginary components. | `true` |

`center=false` performs no boundary extension. A frame is emitted only when a
complete `n_fft`-sample frame fits, so a signal shorter than `n_fft` has no
valid uncentered frame. With centering enabled, `pad_mode` determines how the
synthetic boundary samples are obtained; it does not affect interior frames.

## Shape, Dtype, And Frequency Layout

The receiver must be rank one or two and must have a floating-point or complex
dtype. A rank-two receiver is interpreted strictly as `[batch, time]`, not as
an arbitrary collection of signal axes.

For real input with `onesided=true`, the frequency extent is
`floor(n_fft / 2) + 1`; the omitted negative-frequency bins are redundant
conjugates. `onesided=false` keeps all `n_fft` bins. Complex input or a complex
window requires `onesided=false`, because its negative frequencies are not in
general recoverable from the positive half.

The receiver and explicit window must share a platform. Their dtypes are
promoted before multiplication. Complex output preserves the corresponding
precision. Packed output is real and adds a final `[real, imag]` axis; it is a
representation choice, not a different transform.

## Reverse Mode And Ownership

`stft()` is out of place. It allocates its framed spectrum and leaves the
receiver and window unchanged. Framing, padding, window multiplication, and
the transform basis all remain on the reverse graph, so sensitivity reaches a
tracked signal and a tracked explicit window. Reflected or repeated boundary
samples accumulate their sensitivity back into the source lanes that supplied
them.

Use [`istft()`](istft.md) with matching parameters to reconstruct a signal.
`n_fft`, hop, window length, window values, centering, normalization, and
one-sided layout together define the analysis contract; changing one during
synthesis changes the inverse problem.

## Examples

### 1. Use the complete default framing policy

```cpp
const auto signal = pluss::make_series(
    {1.0, 2.0, 0.0, -1.0, 3.0, 2.0, 1.0, 0.0});
pluss::series::print(std::cout, signal.stft(4));
```

#### Output

```text
Series(shape=[3,9], strides=[9,1], dtype=complex128, platform=cpu, numel=27, contiguous=true)
data=
⎡  (5.0000e+00+0.0000e+00j) (5.0000e+00+0.0000e+00j) (2.0000e+00+0.0000e+00j)  (4.0000e+00+0.0000e+00j)  (4.0000e+00+0.0000e+00j) ...  (4.0000e+00+0.0000e+00j) ⎤
⎢ (-1.0000e+00+0.0000e+00j) (0.0000e+00-1.0000e+00j) (1.0000e+00-3.0000e+00j)  (3.0000e+00+3.0000e+00j) (-3.0000e+00+3.0000e+00j) ... (-3.6739e-16+2.0000e+00j) ⎥
⎣ (-3.0000e+00-7.3479e-16j) (3.0000e+00+3.6739e-16j) (0.0000e+00+1.2246e-16j) (-2.0000e+00-1.3471e-15j)  (2.0000e+00+1.2246e-16j) ...  (0.0000e+00-4.8986e-16j) ⎦
```

For `n_fft=4`, the default hop is one and centering adds two samples to each
end. That yields nine frames. Real input permits the three-bin one-sided
spectrum.

### 2. Analyze only complete frames from the original signal

```cpp
pluss::series::print(
    std::cout,
    signal.stft(4, 2, 4, pluss::series::undefined(),
                false, "constant", false, true, true));
```

#### Output

```text
Series(shape=[3,3], strides=[3,1], dtype=complex128, platform=cpu, numel=9, contiguous=true)
data=
⎡ (2.0000e+00+0.0000e+00j)  (4.0000e+00+0.0000e+00j) (6.0000e+00+0.0000e+00j) ⎤
⎢ (1.0000e+00-3.0000e+00j) (-3.0000e+00+3.0000e+00j) (2.0000e+00-2.0000e+00j) ⎥
⎣ (0.0000e+00+1.2246e-16j)  (2.0000e+00+1.2246e-16j) (2.0000e+00+0.0000e+00j) ⎦
```

The complete frames begin at samples zero, two, and four. Since centering is
off, `pad_mode` is accepted but has no boundary work to perform.

### 3. Supply a window and normalized transform

```cpp
const auto window = pluss::make_series({0.5, 1.0, 1.0, 0.5});
pluss::series::print(
    std::cout,
    signal.stft(4, 2, 4, window, true, "reflect", true, true, true));
```

#### Output

```text
Series(shape=[3,5], strides=[5,1], dtype=complex128, platform=cpu, numel=15, contiguous=true)
data=
⎡  (2.0000e+00+0.0000e+00j)  (1.0000e+00+0.0000e+00j)  (1.5000e+00+0.0000e+00j) (2.2500e+00+0.0000e+00j)  (1.2500e+00+0.0000e+00j) ⎤
⎢ (-5.0000e-01-5.0000e-01j)  (2.5000e-01-1.2500e+00j) (-1.5000e+00+1.0000e+00j) (2.5000e-01-1.0000e+00j) (-2.5000e-01+5.0000e-01j) ⎥
⎣ (-1.0000e+00-1.8370e-16j) (-5.0000e-01-3.0616e-17j)  (1.5000e+00+2.4493e-16j) (2.5000e-01+0.0000e+00j)  (2.5000e-01-6.1232e-17j) ⎦
```

The window attenuates frame edges before transformation. Normalization divides
the unnormalized coefficients by `sqrt(4)`, so synthesis must also request
normalized scaling.

### 4. Retain the full two-sided spectrum

```cpp
pluss::series::print(
    std::cout,
    signal.stft(4, 2, 4, pluss::series::undefined(),
                true, "reflect", false, false, true));
```

#### Output

```text
Series(shape=[4,5], strides=[5,1], dtype=complex128, platform=cpu, numel=20, contiguous=true)
data=
⎡  (5.0000e+00+0.0000e+00j) (2.0000e+00+0.0000e+00j)  (4.0000e+00+0.0000e+00j) (6.0000e+00+0.0000e+00j)  (4.0000e+00+0.0000e+00j) ⎤
⎢ (-1.0000e+00-1.2246e-16j) (1.0000e+00-3.0000e+00j) (-3.0000e+00+3.0000e+00j) (2.0000e+00-2.0000e+00j) (-3.6739e-16+2.0000e+00j) ⎥
⎢ (-3.0000e+00-7.3479e-16j) (0.0000e+00+1.2246e-16j)  (2.0000e+00+1.2246e-16j) (2.0000e+00+0.0000e+00j)  (0.0000e+00-4.8986e-16j) ⎥
⎣ (-1.0000e+00-3.6739e-16j) (1.0000e+00+3.0000e+00j) (-3.0000e+00-3.0000e+00j) (2.0000e+00+2.0000e+00j)  (1.1022e-15-2.0000e+00j) ⎦
```

The fourth row is the negative-frequency partner of the second for this real
signal. Keeping it is useful when a downstream operation expects a full cyclic
frequency axis.

### 5. Request real-and-imaginary component packing

```cpp
const auto packed = signal.stft(
    4, 2, 4, pluss::series::undefined(),
    true, "reflect", false, true, false);
std::cout << "shape=" << packed.sizes() << '\n';
```

#### Output

```text
shape=[3, 5, 2]
```

The final extent is always two: lane zero is real and lane one is imaginary.
Prefer complex output for arithmetic; packed output is intended for consumers
whose interchange format cannot carry complex scalars.

### 6. Preserve a batch axis

```cpp
const auto batch = pluss::make_series({{1.0, 2.0, 3.0, 4.0},
                                       {4.0, 3.0, 2.0, 1.0}});
const auto spectrum = batch.stft(
    4, 2, 4, pluss::series::undefined(),
    false, "constant", false, true, true);
std::cout << "shape=" << spectrum.sizes() << '\n';
```

#### Output

```text
shape=[2, 3, 1]
```

Each row is transformed independently. The first extent remains two; the
single complete frame appears on the final axis.

### 7. Transform genuinely complex samples

```cpp
using Complex = std::complex<double>;
const auto z = pluss::make_series(
    std::vector<Complex>{{1.0, 1.0}, {2.0, 0.0},
                         {3.0, -1.0}, {4.0, 0.0}}, {4});
pluss::series::print(
    std::cout,
    z.stft(4, 4, 4, pluss::series::undefined(),
           false, "constant", false, false, true));
```

#### Output

```text
Series(shape=[4,1], strides=[1,1], dtype=complex128, platform=cpu, numel=4, contiguous=true)
data=
⎡  (1.0000e+01+0.0000e+00j) ⎤
⎢ (-2.0000e+00+4.0000e+00j) ⎥
⎢ (-2.0000e+00-9.1446e-16j) ⎥
⎣ (-2.0000e+00-8.8818e-16j) ⎦
```

All four bins are retained. Requesting one-sided output for this input throws
because there is no conjugate-symmetry guarantee.

### 8. Differentiate through analysis

```cpp
auto tracked = signal.clone();
tracked.tracks_slope_(true);
tracked.stft(4, 2, 4, pluss::series::undefined(),
             true, "reflect", false, true, true)
       .real().sum().reverse();
pluss::series::print(std::cout, tracked.sensitivity());
```

#### Output

```text
Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[4.0000e+00, -2.2204e-16, 7.0000e+00, -2.2204e-16, 4.0000e+00, -4.4409e-16, 5.0000e+00, -2.2204e-16]
```

Boundary lanes receive additional contributions because reflected centered
frames reuse them. Values near zero are transform roundoff.

### 9. Reject incompatible input contracts

```cpp
try {
  (void)pluss::make_series({1, 2, 3, 4}).stft(4);
} catch (const std::exception& error) {
  std::cout << error.what() << '\n';
}
```

#### Output

```text
stft: input must have a floating-point or complex dtype
```

Invalid ranks, nonpositive transform dimensions, a mismatched window length,
an unsupported pad mode, and one-sided complex analysis are likewise rejected
before a spectrum is returned.

## Performance

Work grows with batch size, frame count, and `n_fft`. Framing and windowing are
linear in the framed sample count; the current dense transform basis makes the
frequency transform the dominant cost for large `n_fft`. Transform a complete
batch in one call, keep the signal contiguous, and reuse an explicit window.
The returned spectrum owns new storage; it is never a view of the signal.

## See Also

- [`istft()`](istft.md) overlap-adds compatible spectra into signals.
- [`fftshift()`](fftshift.md) reorders a full frequency axis around zero frequency.
- [`real()`](real.md) and [`imag()`](imag.md) project complex coefficients explicitly.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="stat.html">
    <span class="label">Previous</span>
    <span class="title">stat</span>
  </a>
  <a class="next-link" href="storage_version.html">
    <span class="label">Next</span>
    <span class="title">storage_version</span>
  </a>
</div>
