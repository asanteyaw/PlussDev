// A missing value is not a zero. Table reductions skip it; reading the raw numbers underneath does not.
#include <pluss/pluss.h>

#include <filesystem>
#include <fstream>
#include <iostream>

using pluss::table::DataFrame;

int main() {
  // Write a four-row file with one missing temperature and read it back.
  const auto path = std::filesystem::temp_directory_path() / "pluss_guide_nulls.csv";
  {
    std::ofstream file(path);
    file << "station,temperature\nnorth,12.5\nnorth,\nsouth,15.0\nsouth,16.1\n";
  }
  auto readings = DataFrame::load_csv(path.string());
  std::filesystem::remove(path);
  std::cout << readings.to_string() << "\n";
  auto table_mean = readings.aggregate({{"mean", "temperature", DataFrame::SummaryOp::Mean}}, false);
  std::cout << "mean through the table (skips the missing row): " << table_mean.features("mean").as_series().items<double>()[0] << "\n";
  std::cout << "mean of the raw series underneath (counts it as 0): " << readings.features("temperature").as_series().mean().item<double>() << "\n";
}
