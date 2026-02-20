#include <filesystem>
#include <iostream>
#include <string>

#include <pluss/pluss.h>

namespace {

std::filesystem::path data_file(const std::string& name) {
  return std::filesystem::path(PLUSSMIC_SOURCE_DIR) / "data" / name;
}

} // namespace

int main() {
  pluss::loader::ReadCsvOptions opts;
  opts.filepath = data_file("messy_data.csv").string();
  opts.nrows = 80;
  opts.usecols = {
      "Business Case ID",
      "Agency Name",
      "Lifecycle Cost",
      "Schedule Variance (in days)",
      "Cost Variance (%)",
  };

  auto df = pluss::table::DataFrame::read_csv(opts);

  std::cout << "=== describe demo ===\n";
  std::cout << "input\n";
  df.head(8, true);

  std::cout << "\nsummary statistics\n";
  (void)df.describe(true);

  return 0;
}
