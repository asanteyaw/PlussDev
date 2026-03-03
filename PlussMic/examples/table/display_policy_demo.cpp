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
  auto df = pluss::table::DataFrame::load_csv(
      data_file("messy_data.csv").string(),
      {
          {"nrows", 120},
      });

  std::cout << "=== display policy demo (dynamic terminal width) ===\n";
  std::cout << df << '\n';

  std::cout << "\n=== info table ===\n";
  df.info();

  std::cout << "\n=== forced narrow width (60 cols) ===\n";
  pluss::table::display::DisplayOptions narrow;
  narrow.terminal_width = 60;
  std::cout << df.to_string(narrow) << '\n';

  std::cout << "\n=== Smart .show() viewer (falls back to terminal if unavailable) ===\n";
  std::cout << "head()/describe() return a DataFrameView.\n";
  std::cout << "Leave it alone for terminal preview, or call .show() to request the popup.\n";
  df.head(8).show();
  df.describe().show();

  std::cout << "\n=== Stored preview object ===\n";
  auto preview = df.view(4, 12);
  preview.show();

  return 0;
}
