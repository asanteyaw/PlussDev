#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <fmt/format.h>

#include "pluss/pluss.h"

namespace {

pluss::table::display::DisplayOptions demo_display() {
  pluss::table::display::DisplayOptions opts;
  opts.max_rows = 10;
  opts.max_columns = 8;
  opts.edge_rows = 3;
  opts.edge_columns = 3;
  opts.max_cell_width = 24;
  opts.terminal_width = 120;
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

  fmt::print("\n=== Comma CSV demo: messy_data.csv ===\n");
  df.show(demo_display());

  const auto idx = df.case_().to_vec<std::int64_t>();
  fmt::print("\ncase() first={}, last={}, n={}\n", idx.front(), idx.back(), idx.size());

  fmt::print("\n.head(5) output\n");
  (void)df.head(5, true);

  fmt::print("\n.info() output\n");
  df.info();

  fmt::print("\n.describe() output\n");
  (void)df.describe(true);

  fmt::print("\n.view(5, 10) output (5th to 9th rows)\n");
  (void)df.view(5, 10, true);

  fmt::print("\n.view(-5, -1) output (last 5 rows)\n");
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

  fmt::print("\n=== Semicolon CSV demo: bank-full.csv ===\n");
  fmt::print("`job` is used as case index, so index is textual instead of 0..n-1.\n");
  df.show(demo_display());

  fmt::print("\n.info() output\n");
  df.info();

  fmt::print("\n.describe() output\n");
  (void)df.describe(true);

  fmt::print("\n.view(-5, -1) output on semicolon dataset\n");
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
  opts_display.max_rows = 6;

  fmt::print("\n=== Atlas demo (2 frames) ===\n");
  atlas.show(opts_display);
}

} // namespace

int main() {
  run_comma_csv_demo();
  run_semicolon_csv_demo();
  run_atlas_demo();
  return 0;
}
