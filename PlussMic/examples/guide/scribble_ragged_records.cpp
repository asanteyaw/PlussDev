// Records whose measurements are lists of different lengths: patients with a varying number of lab readings. A Scribble field holds a
// whole Series per record, so no padding and no flattening are needed until you choose to summarise.
#include <pluss/pluss.h>

#include <iostream>
#include <string>
#include <vector>

using pluss::scribble::Registry;
using pluss::scribble::Slot;
using Value = Slot::Value;

int main() {
  Registry patients({
      {"patient", Slot({Value(pluss::make_note({"P-01"})), Value(pluss::make_note({"P-02"})), Value(pluss::make_note({"P-03"})), Value(pluss::make_note({"P-04"}))})},
      {"glucose", Slot({Value(pluss::make_series({5.1, 5.4, 5.0})), Value(pluss::make_series({7.8})), Value(pluss::make_series({6.1, 6.9, 7.4, 8.2, 7.7})), Value(pluss::make_series(std::vector<double>{}))})},
  });
  std::cout << "records " << patients.sizes().at("patient") << "\n";

  // 1. Per-record summaries: each cell is a Series, so the full Series vocabulary applies to it. A record without readings stays empty.
  std::vector<double> count, mean, peak;
  for (const auto &cell : patients.get("glucose")->values()) {
    const auto &readings = std::get<pluss::Series>(cell);
    count.push_back(static_cast<double>(readings.numel()));
    mean.push_back(readings.numel() ? readings.mean().item<double>() : std::numeric_limits<double>::quiet_NaN());
    peak.push_back(readings.numel() ? readings.max().item<double>() : std::numeric_limits<double>::quiet_NaN());
  }
  std::cout << "readings per patient: " << pluss::series::print(pluss::make_series(count)) << "\n";
  std::cout << "mean glucose:         " << pluss::series::print(pluss::make_series(mean)) << "\n";

  // 2. Flag: a patient needs follow-up when the peak exceeds 7.8 mmol/L, and a patient with no readings needs a test.
  auto peaks = pluss::make_series(peak);
  std::cout << "peak above 7.8: " << pluss::series::print(peaks.gt(7.8)) << "\n";
  std::cout << "no readings:    " << pluss::series::print(pluss::make_series(count).eq(0.0)) << "\n";

  // 3. When you want a rectangular table, the summaries become typed columns: the ragged detail has been reduced to numbers.
  std::vector<std::string> names;
  for (const auto &cell : patients.get("patient")->values()) names.push_back(std::string(std::get<pluss::Note>(cell).at(0)));
  // A patient without readings has no mean and no peak to report: those cells are missing (NULL), so the columns carry a validity mask.
  const auto has_readings = pluss::make_series(count).gt(0.0);
  auto summary = pluss::table::DataFrame({{"patient", pluss::make_note(names)},
                                          {"readings", pluss::make_series(count)},
                                          {"mean", pluss::table::Shelf(pluss::make_series(mean), has_readings)},
                                          {"peak", pluss::table::Shelf(pluss::make_series(peak), has_readings)}});
  std::cout << summary.to_string() << "\n";
}
