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

  // std::cout << "=== display policy demo (dynamic terminal width) ===\n";
  // std::cout << df << '\n';

  // std::cout << "\n=== info table ===\n";
  // df.info().show();

  // std::cout << "\n=== forced narrow width (60 cols) ===\n";
  // pluss::table::display::DisplayOptions narrow;
  // narrow.terminal_width = 60;
  // std::cout << df.to_string(narrow) << '\n';

  // std::cout << "\n=== Smart .show() viewer (falls back to terminal if unavailable) ===\n";
  // std::cout << "head()/describe() return a DataFrameView.\n";
  // std::cout << "Auto mode uses NativeWebView first, then InHouseHtml if native is unavailable.\n";
  // df.head(8).show();
  // df.describe().show();

  std::cout << "\n=== Explicit backend selection ===\n";
  pluss::table::display::DisplayOptions inhouse_opts;
  inhouse_opts.backend = pluss::table::display::DisplayBackend::InHouseHtml;
  inhouse_opts.window_title = "DataFrame - head (InHouse)";
  df.head(8).show(inhouse_opts);

  pluss::table::display::DisplayOptions native_opts;
  native_opts.backend = pluss::table::display::DisplayBackend::NativeWebView;
  native_opts.window_title = "DataFrame - head (NativeWebView)";
  df.head(20).show(native_opts);

}