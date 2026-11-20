// A complete file round trip: read a messy CSV, repair it, summarise it, write the clean version and read that back to check.
#include <pluss/pluss.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>

using pluss::table::DataFrame;

int main() {
  const auto dir = std::filesystem::temp_directory_path();
  const auto raw_path = dir / "pluss_guide_raw.csv";
  const auto clean_path = dir / "pluss_guide_clean.csv";
  {
    std::ofstream file(raw_path);
    file << "station,date,temperature,humidity\n"
            "north,2026-11-01,12.5,81\n"
            "north,2026-11-02,,79\n"
            "south,2026-11-01,15.0,70\n"
            ",2026-11-02,14.2,72\n"
            "south,2026-11-03,16.1,\n"
            "north,2026-11-03,11.8,85\n";
  }

  // 1. Read it. The frame knows its shape and the type of every feature.
  auto raw = DataFrame::load_csv(raw_path.string());
  std::cout << "read " << raw.ncases() << " rows, " << raw.nfeatures() << " features\n";
  std::cout << raw.to_string() << "\n";

  // 2. Repair: a row without a station is unusable and is dropped; a missing measurement becomes the feature's own mean.
  auto kept = raw.dropnull({"station"}, true);
  // aggregate skips missing values; reading a column through as_series() would see a placeholder zero where a value is missing.
  auto means = kept.aggregate({{"temperature", "temperature", DataFrame::SummaryOp::Mean}, {"humidity", "humidity", DataFrame::SummaryOp::Mean}}, false);
  const double temp_mean = means.features("temperature").as_series().items<double>()[0];
  const double hum_mean = means.features("humidity").as_series().items<double>()[0];
  // humidity was read as a whole-number feature; its mean is fractional, so filling widens the feature to floating point rather than
  // truncating the fill value.
  auto clean = kept.fillnull(temp_mean, {"temperature"}).fillnull(hum_mean, {"humidity"});
  std::cout << "after cleaning: " << clean.ncases() << " rows; filled temperature with " << temp_mean << " and humidity with " << hum_mean << "\n";
  std::cout << clean.to_string() << "\n";

  // 3. Summarise per station.
  auto report = clean.group({.dropnull = true, .sort = true})
                    .by({"station"})
                    .agg({{"days", "date", DataFrame::GroupOp::Count}, {"mean_temp", "temperature", DataFrame::GroupOp::Mean}, {"max_humidity", "humidity", DataFrame::GroupOp::Max}});
  std::cout << report.to_string() << "\n";

  // 4. Save and read back: what comes back must equal what was saved.
  clean.save_csv(clean_path.string());
  auto again = DataFrame::load_csv(clean_path.string());
  std::cout << "reloaded " << again.ncases() << " rows; humidity total " << again.features("humidity").as_series().sum().item<double>() << " vs " << clean.features("humidity").as_series().sum().item<double>() << "\n";
  std::filesystem::remove(raw_path);
  std::filesystem::remove(clean_path);
}
