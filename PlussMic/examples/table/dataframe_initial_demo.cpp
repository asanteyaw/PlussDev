#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <pluss/pluss.h>

namespace {

pluss::table::display::DisplayOptions demo_display() {
  pluss::table::display::DisplayOptions opts;
  opts.mode = pluss::table::display::DisplayMode::Auto;
  return opts;
}

std::filesystem::path data_file(const std::string& name) {
  return std::filesystem::path(PLUSSMIC_SOURCE_DIR) / "data" / name;
}

void run_comma_csv_demo() {
  pluss::loader::ReadCsvOptions opts;
  opts.filepath = data_file("messy_data.csv").string();
  opts.nrows = 32;
  opts.usecols = {
      "Business Case ID",
      "Agency Name",
      "Agency Project ID",
      "Lifecycle Cost",
      "Updated Date",
  };

  auto df = pluss::table::DataFrame::load("csv", opts);

  std::cout << "\n=== Comma CSV demo: messy_data.csv ===\n";
  df.show(demo_display());

  const auto idx = df.case_().to_vec<std::int64_t>();
  std::cout << "\ncase() first=" << idx.front() << ", last=" << idx.back() << ", n=" << idx.size() << "\n";

  std::cout << "\n.head(5) output\n";
  (void)df.head(5, true);

  std::cout << "\n.info() output\n";
  df.info();

  std::cout << "\n.describe() output\n";
  (void)df.describe(true);

  std::cout << "\n.view(5, 10) output (5th to 9th rows)\n";
  (void)df.view(5, 10, true);

  std::cout << "\n.view(-5, -1) output (last 5 rows)\n";
  (void)df.view(-5, -1, true);
}

void run_semicolon_csv_demo() {
  pluss::loader::ReadCsvOptions opts;
  opts.filepath = data_file("bank-full.csv").string();
  opts.sep = ';';
  opts.nrows = 24;
  opts.usecols = {"age", "job", "balance", "duration", "y"};
  opts.index_col = "job";

  auto df = pluss::table::DataFrame::read_csv(opts);

  std::cout << "\n=== Semicolon CSV demo: bank-full.csv ===\n";
  std::cout << "`job` is used as case index.\n";
  std::cout << "Index is textual (not 0..n-1).\n";
  df.show(demo_display());

  std::cout << "\n.info() output\n";
  df.info();

  std::cout << "\n.describe() output\n";
  (void)df.describe(true);

  std::cout << "\n.view(-5, -1) output on semicolon dataset\n";
  (void)df.view(-5, -1, true);
}

void run_atlas_demo() {
  pluss::loader::ReadCsvOptions opts;
  opts.filepath = data_file("bank-full.csv").string();
  opts.sep = ';';
  opts.nrows = 20;
  opts.usecols = {"age", "balance", "duration"};

  auto df = pluss::table::DataFrame::read_csv(opts);
  auto f0 = df.slice(0, 10);
  auto f1 = df.slice(10, 20);

  pluss::table::Atlas atlas({f0, f1});

  pluss::table::display::DisplayOptions opts_display = demo_display();
  opts_display.mode = pluss::table::display::DisplayMode::Compact;

  std::cout << "\n=== Atlas demo (2 frames) ===\n";
  atlas.show(opts_display);
}

} // namespace

int main() {
  run_comma_csv_demo();
  run_semicolon_csv_demo();
  run_atlas_demo();
  return 0;
}
