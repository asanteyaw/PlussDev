<!-- generated from signals.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# Signals

[Back to Series by example](../series.md)

A signal is a series ordered in time. This page smooths a noisy level and catches faulty readings with a moving statistic, then finds the tones hidden in a recording with the Fourier transform.

## The mental model

Two kinds of tool apply. **Moving statistics** (`stat().movmean`, `movstd`) slide a window along the series and describe the local level and local spread. **Spectral tools** (`rfft`, `irfft`) move between time and frequency: a tone is a spike in the spectrum, and removing the spike removes the tone.

## Trend and anomalies

Sixty readings of a slowly rising level with noise of 0.2, and two injected faults at positions 25 and 41. The program computes a 7-point moving average and standard deviation, flags readings far from the trend, and repairs them.

```cpp
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  using pluss::ScalarType;
  // Sixty readings of a slowly rising level with noise, and two injected faults at positions 25 and 41.
  const std::int64_t n = 60;
  auto t = pluss::series::arange(0, n, ScalarType::Float64);
  auto level = t.mul(0.05).add(10.0).add(pluss::series::randn({n}, ScalarType::Float64, 21).mul(0.2));
  auto values = level.clone();
  auto faults = pluss::make_series({25, 41});
  values = values.index_copy(0, faults, pluss::make_series({14.0, 7.0}));

  // 1. A centred moving average over 7 samples is the trend; the moving standard deviation is the local noise level.
  auto trend = values.stat().movmean(7);
  auto spread = values.stat().movstd(7);
  show("trend, first 8 samples", trend.slice(0, 0, 8));

  // 2. A reading is anomalous when it is more than three times the known noise level (0.2, so a floor of 0.4 is generous) from the trend.
  auto distance = values.sub(trend).abs();
  auto anomaly = distance.gt(3.0 * 0.4);
  show("anomalous positions", anomaly.nonzero().squeeze(1));
  show("their values", values.masked_select(anomaly));
  show("spread at those positions", spread.masked_select(anomaly));

  // 3. Replace each anomaly by the trend value and smooth again.
  auto repaired = values.where(anomaly, trend);
  show("repaired values at the faults", repaired.index_select(0, faults));
  std::cout << "largest remaining deviation from the trend: " << repaired.sub(repaired.stat().movmean(7)).abs().max().item<double>() << "\n";
}
```

Output

```text
trend, first 8 samples: Series(shape=[8], strides=[1], dtype=float64, platform=cpu, numel=8, contiguous=true)
data=[10.4643, 10.1204, 10.0695, 10.0435, 9.9903, 10.0535, 10.1030, 10.0948]
anomalous positions: Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[25, 41]
their values: Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[14.0000, 7.0000]
spread at those positions: Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1.0211, 1.7428]
repaired values at the faults: Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[11.5765, 11.2631]
largest remaining deviation from the trend: 0.609021
```
The trend starts near 10 and rises slowly; at the edges the window shrinks, so the first value, `10.46`, is a less reliable average than those in the middle. Readings more than `0.4 x 3` from the trend are flagged, and they are exactly positions 25 and 41, with values 14 and 7. Note the spread at those positions, 1.0 and 1.7, against a noise level of 0.2: the moving standard deviation is inflated by the faults themselves, which is why the program used the known noise level for its threshold rather than the moving spread. Replacing the faults with the trend leaves a largest deviation of `0.61`, about three noise standard deviations, plausible for the largest of sixty readings.

## Finding the tones

One second of a signal sampled at 1000 Hz: a 50 Hz tone, a weaker 120 Hz tone and noise. `rfft` returns the 501 frequency bins of a real signal; with one second of data bin `k` is `k` Hz.

```cpp
#include <pluss/pluss.h>

#include <cmath>
#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  using pluss::ScalarType;
  const double rate = 1000.0;          // samples per second
  const std::int64_t n = 1000;         // one second of signal
  auto t = pluss::series::arange(0, n, ScalarType::Float64).div(rate);
  const double two_pi = 6.283185307179586;
  // A 50 Hz tone, a weaker 120 Hz tone, and noise.
  auto signal = t.mul(two_pi * 50.0).sin().add(t.mul(two_pi * 120.0).sin().mul(0.5)).add(pluss::series::randn({n}, ScalarType::Float64, 8).mul(0.3));

  // 1. rfft returns the n/2 + 1 frequency bins of a real signal; with one second of data bin k is k Hz.
  auto spectrum = signal.rfft();
  auto magnitude = spectrum.abs().mul(2.0 / n);
  std::cout << "bins " << magnitude.numel() << "\n";

  // 2. The strongest bins are the tones; topk finds them without a manual search.
  auto [peaks, bins] = magnitude.topk_pair(2);
  show("strongest magnitudes", peaks);
  show("at frequencies (Hz)", bins);

  // 3. Remove the 50 Hz tone: zero its bin (and a neighbour on each side) and transform back.
  auto cleaned_spectrum = spectrum.clone();
  auto kill = pluss::make_series({49, 50, 51});
  cleaned_spectrum = cleaned_spectrum.index_fill(0, kill, 0.0);
  auto cleaned = cleaned_spectrum.irfft(n);
  auto after = cleaned.rfft().abs().mul(2.0 / n);
  std::cout << "after the filter: 50 Hz magnitude " << after.select(0, 50).item<double>() << ", 120 Hz magnitude " << after.select(0, 120).item<double>() << "\n";
  std::cout << "energy before " << signal.square().mean().item<double>() << ", after " << cleaned.square().mean().item<double>() << "\n";
}
```

Output

```text
bins 501
strongest magnitudes: Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[0.9943, 0.4831]
at frequencies (Hz): Series(shape=[2], strides=[1], dtype=int64, platform=cpu, numel=2, contiguous=true)
data=[50, 120]
after the filter: 50 Hz magnitude 8.74754e-18, 120 Hz magnitude 0.48308
energy before 0.700794, after 0.206196
```
`topk_pair(2)` reads the two strongest bins directly: 50 Hz at magnitude `0.994` and 120 Hz at `0.483`, matching the amplitudes of 1 and 0.5 that went in (the noise adds a little). To remove the 50 Hz tone, the program zeros bins 49 to 51 and transforms back with `irfft`. The 50 Hz magnitude of the cleaned signal is of order `1e-17`, the 120 Hz tone is untouched at `0.483`, and the mean-square energy has fallen from `0.70` to `0.21`, the 0.5 of the 50 Hz tone gone.

## Decisions

- **Window length:** odd, a few times the noise correlation length, short against the features you want to keep.
- **Edges:** the first and last windows are shorter; do not read meaning into the first few values.
- **Thresholds:** use a known or robust noise level, not a moving spread the faults themselves have inflated.
- **Frequency resolution:** it is the reciprocal of the record length; one second gives 1 Hz bins, so a tone between bins leaks into its neighbours.

## Where next

Reference pages: [`stat`](../../sphinx/api/series/stat.md), [`rfft`](../../sphinx/api/series/rfft.md), [`irfft`](../../sphinx/api/series/irfft.md), [`topk_pair`](../../sphinx/api/series/topk_pair.md), [`index_copy`](../../sphinx/api/series/index_copy.md), [`masked_select`](../../sphinx/api/series/masked_select.md), [`nonzero`](../../sphinx/api/series/nonzero.md). Next: [Learning by gradient](learning_by_gradient.md).
