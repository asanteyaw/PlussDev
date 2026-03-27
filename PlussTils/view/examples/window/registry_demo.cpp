#include <filesystem>
#include <iostream>
#include <string>

#include <pluss/pluss.h>
#include <pluss/tils/view/html_window.h>

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
  // std::cout << "Auto mode uses NativeWebView first, then PlussWebView if native is unavailable.\n";
  // df.head(8).show();
  // df.describe().show();

  std::cout << "\n=== Explicit backend selection ===\n";
  pluss::table::display::DisplayOptions inhouse_table_opts;
  inhouse_table_opts.backend = pluss::table::display::DisplayBackend::PlussWebView;
  inhouse_table_opts.window_title = "DataFrame - head (InHouse)";
  df.head(20).show(inhouse_table_opts);

  pluss::view::HtmlWindowOptions inhouse_opts;
  inhouse_opts.title = "Numeric correlations (InHouse)";
  inhouse_opts.backend_preference = pluss::view::BackendPreference::PlussWebView;
  auto fig_scatter = df.plot().scatter_matrix();
  fig_scatter.headline("Numeric correlations").show(inhouse_opts);

  pluss::table::display::DisplayOptions native_opts;
  native_opts.backend = pluss::table::display::DisplayBackend::NativeWebView;
  native_opts.window_title = "DataFrame - head (NativeWebView)";
  df.head(20).show(native_opts);
  return 0;
}
