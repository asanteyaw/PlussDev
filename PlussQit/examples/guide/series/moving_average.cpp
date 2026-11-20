// Smooth a noisy series and catch anomalies: a moving average for the trend, a moving deviation for the band, and a flag outside it.
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Series &s) { std::cout << label << ": " << pluss::series::print(s) << "\n"; }

int main() {
  using pluss::ScalarType;
  // Sixty readings of a slowly rising level with noise, and two injected faults at positions 25 and 41.
  const std::int64_t n = 60;
  auto t = pluss::series::arange(0, n, ScalarType::Float64);
  auto level = t * 0.05 + 10.0 + pluss::series::randn({n}, ScalarType::Float64, 21) * 0.2;
  auto values = level.clone();
  auto faults = pluss::make_series({25, 41});
  values = values.index_copy(0, faults, pluss::make_series({14.0, 7.0}));

  // 1. A centred moving average over 7 samples is the trend; the moving standard deviation is the local noise level.
  auto trend = values.stat().movmean(7);
  auto spread = values.stat().movstd(7);
  show("trend, first 8 samples", trend.slice(0, 0, 8));

  // 2. A reading is anomalous when it is more than three times the known noise level (0.2, so a floor of 0.4 is generous) from the trend.
  auto distance = (values - trend).abs();
  auto anomaly = distance.gt(3.0 * 0.4);
  show("anomalous positions", anomaly.nonzero().squeeze(1));
  show("their values", values.masked_select(anomaly));
  show("spread at those positions", spread.masked_select(anomaly));

  // 3. Replace each anomaly by the trend value and smooth again.
  auto repaired = pluss::series::where(anomaly, trend, values);
  show("repaired values at the faults", repaired.index_select(0, faults));
  std::cout << "largest remaining deviation from the trend: " << (repaired - repaired.stat().movmean(7)).abs().max().item<double>() << "\n";
}
