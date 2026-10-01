<!-- pluss-hand-authored -->

# Series::istft

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
Series Series::istft(
    std::int64_t n_fft,
    std::int64_t hop_length = -1,
    std::int64_t win_length = -1,
    const Series& window = {},
    bool center = true,
    bool normalized = false,
    bool onesided = true,
    std::int64_t length = -1) const
```

## Reconstructing A Signal From Overlapping Spectral Frames

`istft()` converts a complex short-time spectrum back into a real signal. It
performs an inverse transform for every frame, reapplies the analysis window,
overlap-adds adjacent frames at `hop_length` spacing, and divides by the
overlap-added squared window envelope. That final division is what reconstructs
the original amplitude where neighboring frames overlap.

The receiver is the spectrum, not the original signal. It must be the complex
output of a compatible [`stft()`](stft.md) call:

- shape `[frequency, frame]` reconstructs one signal and returns shape
  `[time]`;
- shape `[batch, frequency, frame]` reconstructs each batch independently and
  returns `[batch, time]`.

No additional batch ranks are accepted. The method returns a real Series whose
dtype corresponds to the complex input: `complex64` reconstructs `float32`, and
`complex128` reconstructs `float64`.

## Parameters

| Parameter | Meaning | Default |
|---|---|---|
| `n_fft` | Frame transform length and inverse basis size. Must be positive and must match the analysis call. | Required |
| `hop_length` | Number of output samples between adjacent frames. `-1` selects `n_fft / 4`. | `-1` |
| `win_length` | Length of the unpadded analysis window. `-1` selects `n_fft`. | `-1` |
| `window` | One-dimensional real window of exactly `win_length` elements. An undefined value selects a rectangular window of ones. | Undefined |
| `center` | Removes `n_fft / 2` samples from the leading framed padding and, without explicit `length`, the trailing padding. Must match analysis. | `true` |
| `normalized` | Uses reciprocal-square-root transform scaling instead of reciprocal-length scaling. Must match analysis. | `false` |
| `onesided` | Declares whether the spectrum stores only non-negative frequencies. Must match analysis. | `true` |
| `length` | Requested output extent. `-1` infers it from frame count and centering; a positive value crops or extends reconstruction. | `-1` |

The window, centering, normalization, and frequency-layout choices are not
cosmetic. Passing values that differ from the matching analysis changes the
inverse problem and generally prevents faithful reconstruction.

## Frequency And Length Rules

With `onesided=true`, the receiver's frequency extent must be
`n_fft / 2 + 1`. With `onesided=false`, it must be exactly `n_fft`. A mismatch
is rejected before reconstruction.

Without explicit `length`, output length follows the available frames after
center trimming. A positive shorter length crops the tail. A longer length
first returns every sample represented by the framed spectrum, including any
recoverable centered boundary samples, then appends zeros only beyond that
reconstructible extent. Therefore, “longer than the original signal” does not
necessarily mean that every lane after the original length is immediately
zero; centered analysis may have encoded reflected boundary samples there.

The window envelope must be nonzero at every returned reconstructed sample.
If the chosen window and hop leave an uncovered sample, `istft()` throws rather
than divide by zero and return infinities or NaNs.

## Differentiation And Allocation

The transform is composed from differentiable matrix products, overlap-add,
slicing, multiplication, and division. Reverse sensitivity therefore reaches
the complex spectrum and a tracked window. Cropped lanes receive no
sensitivity; appended zero padding is constant. The method is out of place and
does not modify its spectral receiver.

## Examples

### 1. Reconstruct a one-dimensional signal

```cpp
const auto signal = pluss::make_series(
    {1.0, 2.0, 0.0, -1.0, 3.0, 2.0, 1.0, 0.0});
const auto spec = signal.stft(
    4, 2, 4, pluss::series::undefined(),
    true, "reflect", false, true, true);

pluss::series::print(
    std::cout,
    spec.istft(4, 2, 4, pluss::series::undefined(),
               true, false, true, 8));
```

#### Output

```text
Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[1.0000e+00, 2.0000e+00, -1.3020e-16, -1.0000e+00, 3.0000e+00, 2.0000e+00, 1.0000e+00, -5.5511e-17]
```

The values near zero are floating transform roundoff, not missing samples.
Use a tolerance comparison when validating a spectral round trip.

### 2. Let hop, window length, and output length be inferred

```cpp
const auto signal = pluss::make_series(
    {1.0, 2.0, 0.0, -1.0, 3.0, 2.0, 1.0, 0.0});
const auto restored = signal.stft(4).istft(4);
pluss::series::print(std::cout, restored);
```

#### Output

```text
Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[1.0000e+00, 2.0000e+00, -9.2854e-17, -1.0000e+00, 3.0000e+00, 2.0000e+00, 1.0000e+00, -8.2173e-33]
```

For `n_fft=4`, the default hop is one. Both calls use a rectangular window,
centered frames, one-sided storage, and unnormalized scaling.

### 3. Match normalized analysis

```cpp
const auto signal = pluss::make_series(
    {1.0, 2.0, 0.0, -1.0, 3.0, 2.0, 1.0, 0.0});
const auto spec = signal.stft(
    4, 2, 4, pluss::series::undefined(),
    true, "reflect", true, true, true);
pluss::series::print(
    std::cout,
    spec.istft(4, 2, 4, pluss::series::undefined(),
               true, true, true, 8));
```

#### Output

```text
Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[1.0000e+00, 2.0000e+00, -1.3020e-16, -1.0000e+00, 3.0000e+00, 2.0000e+00, 1.0000e+00, -5.5511e-17]
```

Normalization cancels only when analysis and synthesis agree. Setting it on
one side but not the other rescales the reconstructed signal.

### 4. Reuse the same explicit window in both directions

```cpp
const auto signal = pluss::make_series(
    {1.0, 2.0, 0.0, -1.0, 3.0, 2.0, 1.0, 0.0});
const auto window = pluss::make_series({0.5, 1.0, 1.0, 0.5});
const auto spec = signal.stft(
    4, 2, 4, window, true, "reflect", false, true, true);
pluss::series::print(
    std::cout, spec.istft(4, 2, 4, window, true, false, true, 8));
```

#### Output

```text
Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[1.0000e+00, 2.0000e+00, 3.3647e-17, -1.0000e+00, 3.0000e+00, 2.0000e+00, 1.0000e+00, 4.4409e-17]
```

The squared-window envelope compensates for the unequal window weights. The
window must be one-dimensional, have exactly four lanes here, share the
spectrum platform, and be real-valued.

### 5. Reconstruct frames created without centering

```cpp
const auto signal = pluss::make_series(
    {1.0, 2.0, 0.0, -1.0, 3.0, 2.0, 1.0, 0.0});
const auto spec = signal.stft(
    4, 2, 4, pluss::series::undefined(),
    false, "reflect", false, true, true);
pluss::series::print(
    std::cout,
    spec.istft(4, 2, 4, pluss::series::undefined(),
               false, false, true, 8));
```

#### Output

```text
Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[1.0000e+00, 2.0000e+00, -1.3020e-16, -1.0000e+00, 3.0000e+00, 2.0000e+00, 1.0000e+00, -2.2204e-16]
```

No leading or trailing half-frame is removed because no such padding was added
during analysis.

### 6. Reconstruct a full two-sided spectrum

```cpp
const auto signal = pluss::make_series(
    {1.0, 2.0, 0.0, -1.0, 3.0, 2.0, 1.0, 0.0});
const auto spec = signal.stft(
    4, 2, 4, pluss::series::undefined(),
    true, "reflect", false, false, true);
pluss::series::print(
    std::cout,
    spec.istft(4, 2, 4, pluss::series::undefined(),
               true, false, false, 8));
```

#### Output

```text
Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[1.0000e+00, 2.0000e+00, 1.0244e-16, -1.0000e+00, 3.0000e+00, 2.0000e+00, 1.0000e+00, 1.6653e-16]
```

`onesided=false` tells the inverse to expect all `n_fft` frequency bins rather
than only `n_fft / 2 + 1` bins.

### 7. Reconstruct a batch without flattening it

```cpp
const auto batch = pluss::make_series({
    {1.0, 2.0, 3.0, 4.0, 3.0, 2.0, 1.0, 0.0},
    {0.0, 1.0, 0.0, -1.0, 0.0, 1.0, 0.0, -1.0},
});
const auto spec = batch.stft(
    4, 2, 4, pluss::series::undefined(),
    true, "reflect", false, true, true);

std::cout << "spectrum shape=[" << spec.size(0) << ','
          << spec.size(1) << ',' << spec.size(2) << "]\n";
const auto restored = spec.istft(
    4, 2, 4, pluss::series::undefined(),
    true, false, true, 8);
std::cout << "output shape=[" << restored.size(0) << ','
          << restored.size(1) << "]\n";
```

#### Output

```text
spectrum shape=[2,3,5]
output shape=[2,8]
```

The first axis remains the batch axis. `istft()` never merges batch and time
into one vector.

### 8. Crop to a requested shorter length

```cpp
const auto signal = pluss::make_series(
    {1.0, 2.0, 0.0, -1.0, 3.0, 2.0, 1.0, 0.0});
const auto spec = signal.stft(
    4, 2, 4, pluss::series::undefined(),
    true, "reflect", false, true, true);
pluss::series::print(
    std::cout,
    spec.istft(4, 2, 4, pluss::series::undefined(),
               true, false, true, 5));
```

#### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[1.0000e+00, 2.0000e+00, -1.3020e-16, -1.0000e+00, 3.0000e+00]
```

Cropping is performed after overlap-add normalization.

### 9. Extend beyond the reconstructible extent

```cpp
const auto signal = pluss::make_series(
    {1.0, 2.0, 0.0, -1.0, 3.0, 2.0, 1.0, 0.0});
const auto spec = signal.stft(
    4, 2, 4, pluss::series::undefined(),
    true, "reflect", false, true, true);
pluss::series::print(
    std::cout,
    spec.istft(4, 2, 4, pluss::series::undefined(),
               true, false, true, 11));
```

#### Output

```text
Series(shape=[11], strides=[1], dtype=float64, platform=cpu, numel=11, contiguous=true)
data=[1.0000e+00, 2.0000e+00, -1.3020e-16, -1.0000e+00, 3.0000e+00, 2.0000e+00, 1.0000e+00, -5.5511e-17, 1.0000e+00, 2.0000e+00, 0.0000e+00]
```

Centered reflected frames still encode the values at positions eight and nine.
Position ten lies beyond the framed extent and is padded with zero.

### 10. Differentiate with respect to the complex spectrum

```cpp
const auto signal = pluss::make_series(
    {1.0, 2.0, 0.0, -1.0, 3.0, 2.0, 1.0, 0.0});
auto spec = signal.stft(4, 2, 4, pluss::series::undefined(),
                        true, "reflect", false, true, true);
spec.tracks_slope_(true);
spec.istft(4, 2, 4, pluss::series::undefined(),
           true, false, true, 8).sum().reverse();
pluss::series::print(std::cout, spec.sensitivity().real());
```

#### Output

```text
Series(shape=[3,5], strides=[5,1], dtype=float64, platform=cpu, numel=15, contiguous=true)
data=
⎡  2.5000e-01  5.0000e-01  5.0000e-01  5.0000e-01 2.5000e-01 ⎤
⎢ -2.5000e-01 -4.5924e-17 -4.5924e-17 -4.5924e-17 2.5000e-01 ⎥
⎣  0.0000e+00  0.0000e+00  0.0000e+00  0.0000e+00 0.0000e+00 ⎦
```

The sensitivity has the same frequency-by-frame shape as the spectrum. The
display projects its real component; the underlying sensitivity remains
complex.

### 11. Reject an incompatible window shape

```cpp
const auto signal = pluss::make_series(
    {1.0, 2.0, 0.0, -1.0, 3.0, 2.0, 1.0, 0.0});
const auto spec = signal.stft(
    4, 2, 4, pluss::series::undefined(),
    true, "reflect", false, true, true);
try {
  (void)spec.istft(4, 2, 4, pluss::make_series({1.0, 1.0}));
} catch (const std::exception& error) {
  std::cout << error.what() << '\n';
}
```

#### Output

```text
istft: window must be one-dimensional with length win_length
```

### 12. Reject a window that leaves samples uncovered

```cpp
const auto signal = pluss::make_series(
    {1.0, 2.0, 0.0, -1.0, 3.0, 2.0, 1.0, 0.0});
const auto spec = signal.stft(
    4, 2, 4, pluss::series::undefined(),
    true, "reflect", false, true, true);
try {
  const auto zero_window =
      pluss::series::zeros({4}, pluss::ScalarType::Float64);
  (void)spec.istft(4, 2, 4, zero_window);
} catch (const std::exception& error) {
  std::cout << error.what() << '\n';
}
```

#### Output

```text
istft: window overlap-add envelope contains an uncovered sample
```

Returning a numeric result here would require division by a zero envelope. The
method rejects the reconstruction instead.

## Performance

Work scales with batch size, frame count, and transform size. Overlap-add and
window normalization are linear in reconstructed samples; inverse frame
transforms dominate for larger `n_fft`. Keep spectra contiguous, reuse the same
window Series, and reconstruct a batch in one call rather than looping over
individual signals. The operation allocates frame, envelope, and output
buffers; it is not a view.

## See Also

- [`stft()`](stft.md) creates the compatible complex spectrum.
- [`is_complex()`](is_complex.md) checks the required receiver dtype family.
- [`allclose()`](allclose.md) compares spectral round trips with a tolerance.
