// Find the tones hidden in a noisy recording: sample a signal, take its spectrum and read the strongest frequencies off it.
#include <pluss/pluss.h>

#include <cmath>
#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  using pluss::ScalarType;
  const double rate = 1000.0;          // samples per second
  const std::int64_t n = 1000;         // one second of signal
  auto t = pluss::series::arange(0, n, ScalarType::Float64) / rate;
  const double two_pi = 6.283185307179586;
  // A 50 Hz tone, a weaker 120 Hz tone, and noise.
  auto signal = (t * (two_pi * 50.0)).sin() + (t * (two_pi * 120.0)).sin() * 0.5 + pluss::series::randn({n}, ScalarType::Float64, 8) * 0.3;

  // 1. rfft returns the n/2 + 1 frequency bins of a real signal; with one second of data bin k is k Hz.
  auto spectrum = signal.rfft();
  auto magnitude = spectrum.abs() * (2.0 / n);
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
  auto after = cleaned.rfft().abs() * (2.0 / n);
  std::cout << "after the filter: 50 Hz magnitude " << after.select(0, 50).item<double>() << ", 120 Hz magnitude " << after.select(0, 120).item<double>() << "\n";
  std::cout << "energy before " << signal.square().mean().item<double>() << ", after " << cleaned.square().mean().item<double>() << "\n";
}
