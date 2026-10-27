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
  auto df = pluss::table::DataFrame::load_csv(
      data_file("messy_data.csv").string(),
      {
          {"ncases", 32},
      });

  std::cout << "\n=== Comma CSV demo: messy_data.csv ===\n";
  std::cout << df.to_string(demo_display()) << '\n';

  const auto idx = df.labels().values().as_series().host<std::int64_t>();
  std::cout << "\ncases() first=" << idx.front() << ", last=" << idx.back() << ", n=" << idx.size() << "\n";

  std::cout << "\n.head(5) output\n";
  df.head(5);

  std::cout << "\n.tail(5) output\n";
  df.tail(5);

  std::cout << "\n.info() output\n";
  df.info();

  std::cout << "\n.describe() output\n";
  df.describe();

  std::cout << "\n.view(5, 10) output (5th to 9th rows)\n";
  df.view(5, 10);

  std::cout << "\n.view(-5, -1) output (last 5 rows)\n";
  df.view(-5, -1);
}

void run_semicolon_csv_demo() {
  auto df = pluss::table::DataFrame::load_csv(
      data_file("bank-full.csv").string(),
      {
          {"ncases", 24},
          {"case col", "job"},
      });

  std::cout << "\n=== Semicolon CSV demo: bank-full.csv ===\n";
  std::cout << "`job` is used as cases axis.\n";
  std::cout << "Index is textual (not 0..n-1).\n";
  std::cout << df.to_string(demo_display()) << '\n';

  std::cout << "\n.info() output\n";
  df.info();

  std::cout << "\n.describe() output\n";
  df.describe();

  std::cout << "\n.view(-5, -1) output on semicolon dataset\n";
  df.view(-5, -1);
}

void run_atlas_demo() {
  auto df = pluss::table::DataFrame::load_csv(
      data_file("bank-full.csv").string(),
      {
          {"ncases", 20},
          {"features", std::vector<std::string>{"age", "balance", "duration"}},
      });
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
