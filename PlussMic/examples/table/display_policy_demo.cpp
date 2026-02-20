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
  opts.nrows = 120;

  auto df = pluss::table::DataFrame::read_csv(opts);

  std::cout << "=== display policy demo (dynamic terminal width) ===\n";
  df.show();

  std::cout << "\n=== info table ===\n";
  df.info();

  std::cout << "\n=== forced narrow width (60 cols) ===\n";
  pluss::table::display::DisplayOptions narrow;
  narrow.terminal_width = 60;
  df.show(narrow);

  return 0;
}
