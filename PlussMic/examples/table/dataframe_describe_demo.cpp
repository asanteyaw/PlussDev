#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <pluss/pluss.h>

namespace {

std::filesystem::path data_file(const std::string& name) {
  return std::filesystem::path(PLUSSMIC_SOURCE_DIR) / "data" / name;
}

} // namespace

int main() {
  auto df = pluss::table::DataFrame::load_csv(
      data_file("messy_data.csv").string(),
      {
          {"nrows", 80},
          {"columns",
           std::vector<std::string>{
               "Business Case ID",
               "Agency Name",
               "Lifecycle Cost",
               "Schedule Variance (in days)",
               "Cost Variance (%)",
           }},
      });

  std::cout << "=== describe demo ===\n";
  std::cout << "input\n";
  df.head(8);

  std::cout << "\nsummary statistics\n";
  df.describe();

  return 0;
}
