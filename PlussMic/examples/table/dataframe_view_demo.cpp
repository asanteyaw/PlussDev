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
  opts.filepath = data_file("bank-full.csv").string();
  opts.sep = ';';
  opts.nrows = 32;
  opts.usecols = {"age", "job", "balance", "duration", "y"};

  auto df = pluss::table::DataFrame::read_csv(opts);

  std::cout << "=== view demo ===\n";
  std::cout << "view(5, 10) -> rows 5..9\n";
  (void)df.view(5, 10, true);

  std::cout << "\nview(-5, -1) -> last 5 rows\n";
  (void)df.view(-5, -1, true);

  std::cout << "\nview(0, 8) -> head equivalent\n";
  (void)df.view(0, 8, true);

  return 0;
}
