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
      data_file("bank-full.csv").string(),
      {
          {"nrows", 32},
          {"columns", std::vector<std::string>{"age", "job", "balance", "duration", "y"}},
      });

  std::cout << "=== view demo ===\n";
  std::cout << "view(5, 10) -> rows 5..9\n";
  df.view(5, 10);

  std::cout << "\nview(-5, -1) -> last 5 rows\n";
  df.view(-5, -1);

  std::cout << "\nview(0, 8) -> head equivalent\n";
  df.view(0, 8);

  return 0;
}
