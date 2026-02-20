#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include "CodeM/core/ScalarType.h"
#include "pluss/pluss.h"

namespace {

using pluss::loader::BadLinePolicy;
using pluss::loader::ReadCsvOptions;
using pluss::table::DataFrame;
using pluss::table::ThreadKind;

[[noreturn]] void fail(std::string_view message) {
  throw std::runtime_error(std::string(message));
}

void expect(bool condition, std::string_view message) {
  if (!condition) {
    fail(message);
  }
}

template <class Fn>
void expect_throw(Fn&& fn, std::string_view message) {
  bool threw = false;
  try {
    fn();
  } catch (...) {
    threw = true;
  }
  if (!threw) {
    fail(message);
  }
}

std::filesystem::path project_root() {
  return std::filesystem::path(PLUSSMIC_SOURCE_DIR);
}

std::filesystem::path fixture_path(const std::string& name) {
  return project_root() / "data" / name;
}

std::filesystem::path write_temp_csv(const std::string& stem, const std::string& contents) {
  const auto dir = std::filesystem::temp_directory_path() / "plussmic_csv_tests";
  std::filesystem::create_directories(dir);
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  const auto filename = fmt::format("{}_{}.csv", stem, stamp);
  const auto path = dir / filename;

  std::ofstream out(path, std::ios::out | std::ios::trunc | std::ios::binary);
  expect(out.is_open(), "failed to create temporary csv");
  out << contents;
  out.close();
  return path;
}

std::vector<std::string> split_lines(const std::string& s) {
  std::vector<std::string> out;
  std::stringstream ss(s);
  std::string line;
  while (std::getline(ss, line)) {
    out.push_back(line);
  }
  return out;
}

std::size_t count_char(const std::string& s, char c) {
  return static_cast<std::size_t>(std::count(s.begin(), s.end(), c));
}

void test_comma_fixture_load_and_case_index() {
  ReadCsvOptions options;
  options.filepath = fixture_path("messy_data.csv").string();
  options.nrows = 64;
  options.usecols = {
      "Business Case ID",
      "Agency Name",
      "Agency Project ID",
      "Lifecycle Cost",
      "Updated Date",
  };

  const auto df = DataFrame::load("csv", options);

  expect(df.nrows() == 64, "comma fixture: nrows mismatch");
  expect(df.ncols() == 5, "comma fixture: ncols mismatch");
  expect(df.has_column("Business Case ID"), "comma fixture: missing Business Case ID");
  expect(df.column("Business Case ID").kind() == ThreadKind::Series, "comma fixture: expected numeric Series column");
  expect(df.column("Agency Name").kind() == ThreadKind::Note, "comma fixture: expected text Note column");

  const auto agency_project = df.column("Agency Project ID");
  expect(agency_project.has_validity_mask(), "comma fixture: Agency Project ID should carry null mask");

  const auto idx = df.case_().to_vec<std::int64_t>();
  expect(!idx.empty(), "comma fixture: case index unexpectedly empty");
  expect(idx.front() == 0, "comma fixture: case index must start at 0");
  expect(idx.back() == 63, "comma fixture: case index last value mismatch");
}

void test_semicolon_fixture_with_index_col() {
  ReadCsvOptions options;
  options.filepath = fixture_path("bank-full.csv").string();
  options.sep = ';';
  options.nrows = 40;
  options.usecols = {"age", "job", "balance", "y"};
  options.index_col = "job";

  const auto df = DataFrame::read_csv(options);

  expect(df.nrows() == 40, "semicolon fixture: nrows mismatch");
  expect(df.ncols() == 3, "semicolon fixture: ncols mismatch when index_col is selected");
  expect(df.case_index().kind() == ThreadKind::Note, "semicolon fixture: expected text case index");
  expect(df.column("age").kind() == ThreadKind::Series, "semicolon fixture: age should parse as Series");

  expect_throw([&]() { (void)df.case_(); }, "case_() should fail when index is not Series-backed");

  pluss::table::display::DisplayOptions show_opts;
  show_opts.mode = pluss::table::display::DisplayMode::Compact;
  show_opts.terminal_width = 120;

  const auto shown = df.to_string(show_opts);
  expect(shown.find("management") != std::string::npos, "show output should contain case index values");
  expect(shown.find("[40 cases x 3 columns]") != std::string::npos, "show output should contain shape footer");
}

void test_bad_line_policy_and_locale_numeric_parsing() {
  const auto path = write_temp_csv(
      "semi_bad_lines",
      "id;flag;amount;when;text\n"
      "1;yes;1.234,50;2024-01-01T00:00:00Z;\"hello;world\"\n"
      "2;no;2.000,00;2024-01-02T00:00:00Z;\"quote \"\"inner\"\"\"\n"
      "3;yes;;;na\n"
      "BROKEN;ROW\n"
      "4;no;3.500,75;2024-01-04T00:00:00Z;tail\n");

  ReadCsvOptions options;
  options.filepath = path.string();
  options.sep = ';';
  options.decimal = ',';
  options.thousands = '.';
  options.parse_dates = {"when"};
  options.na_values.insert("na");
  options.on_bad_lines = BadLinePolicy::Skip;

  const auto df = DataFrame::read_csv(options);

  expect(df.nrows() == 4, "skip bad lines: expected malformed row to be removed");
  expect(df.ncols() == 5, "skip bad lines: ncols mismatch");
  expect(df.column("id").kind() == ThreadKind::Series, "id should parse as Series");
  expect(df.column("flag").kind() == ThreadKind::Series, "flag should parse as bool Series");
  expect(df.column("when").kind() == ThreadKind::Timeline, "when should parse as Timeline");

  const auto flag = df.column("flag").as_series();
  expect(flag.scalar_type() == pluss::codem::ScalarType::Bool, "flag should infer Bool dtype");

  const auto amount = df.column("amount");
  expect(amount.has_validity_mask(), "amount should have null mask due missing value");
  const auto amount_values = amount.as_series().to_vec<double>();
  const auto amount_valid = amount.validity_mask()->to_vec<bool>();

  expect(amount_valid.size() == 4, "amount mask length mismatch");
  expect(amount_valid[0] && amount_valid[1] && !amount_valid[2] && amount_valid[3], "amount validity mask values mismatch");
  expect(std::fabs(amount_values[0] - 1234.50) < 1e-9, "amount row0 parse mismatch");
  expect(std::fabs(amount_values[1] - 2000.00) < 1e-9, "amount row1 parse mismatch");
  expect(std::fabs(amount_values[3] - 3500.75) < 1e-9, "amount row3 parse mismatch");

  const auto when = df.column("when");
  expect(when.has_validity_mask(), "when should carry null mask for empty datetime");

  const auto text = df.column("text").as_note();
  expect(text.at(0) == "hello;world", "quoted delimiter should stay inside text field");
  expect(text.at(1) == "quote \"inner\"", "double quote escaping should parse correctly");

  options.on_bad_lines = BadLinePolicy::Error;
  expect_throw([&]() { (void)DataFrame::read_csv(options); }, "BadLinePolicy::Error should throw on malformed rows");
}

void test_usecols_nrows_skipfooter_and_invalid_source() {
  const auto path = write_temp_csv(
      "limits",
      "a,b,c\n"
      "0,10,x\n"
      "1,11,y\n"
      "2,12,z\n"
      "3,13,w\n");

  ReadCsvOptions options;
  options.filepath = path.string();
  options.usecols_idx = {0, 2};
  options.nrows = 2;

  const auto first_two = DataFrame::read_csv(options);
  expect(first_two.nrows() == 2, "nrows option should cap accepted rows");
  expect(first_two.ncols() == 2, "usecols_idx should project to two columns");

  options.nrows.reset();
  options.skipfooter = 1;
  const auto without_footer = DataFrame::read_csv(options);
  expect(without_footer.nrows() == 3, "skipfooter should trim trailing data rows");

  options.usecols_idx = {0, 99};
  expect_throw([&]() { (void)DataFrame::read_csv(options); }, "invalid usecols_idx must throw");

  ReadCsvOptions fixture;
  fixture.filepath = fixture_path("messy_data.csv").string();
  fixture.nrows = 5;
  expect_throw([&]() { (void)DataFrame::load("xlsx", fixture); }, "unsupported DataFrame::load source should throw");
}

void test_display_engine_width_truncation_and_info() {
  ReadCsvOptions options;
  options.filepath = fixture_path("bank-full.csv").string();
  options.sep = ';';
  options.nrows = 80;

  const auto df = DataFrame::read_csv(options);

  pluss::table::display::DisplayOptions show_opts;
  show_opts.mode = pluss::table::display::DisplayMode::Compact;
  show_opts.terminal_width = 44;

  const auto shown = df.to_string(show_opts);
  const auto lines = split_lines(shown);
  expect(!lines.empty(), "display output should not be empty");
  for (const auto& line : lines) {
    expect(static_cast<std::int64_t>(line.size()) <= show_opts.terminal_width,
           "display output exceeded configured terminal width");
  }
  expect(shown.find("...") != std::string::npos, "display output should include ellipsis under truncation");
  expect(lines.size() >= 4, "display output should include table header/rows");
  expect(lines[1].find("case") == std::string::npos, "display output should not render `case` as a column header");
  for (std::size_t i = 1; i + 2 < lines.size(); ++i) {
    if (lines[i].find('=') != std::string::npos || lines[i].find('-') != std::string::npos) {
      continue;
    }
    expect(count_char(lines[i], '|') <= 1, "display output should render at most one vertical divider per row");
  }

  std::ostringstream os;
  df.show(os, show_opts);
  expect(os.str() == shown, "show(stream) output should match to_string");

  const auto info = df.info_string();
  expect(info.find("Class") != std::string::npos, "info output missing class field");
  expect(info.find("pluss::") != std::string::npos, "info output missing class value");
  expect(info.find("Case Range") != std::string::npos, "info output missing case range field");
  expect(info.find("Dtypes") != std::string::npos, "info output missing dtype summary");
  expect(info.find("Memory") != std::string::npos, "info output missing memory field");
}

void test_describe_numeric_summary() {
  ReadCsvOptions options;
  options.filepath = fixture_path("bank-full.csv").string();
  options.sep = ';';
  options.nrows = 30;
  options.usecols = {"age", "balance", "duration", "y"};

  const auto df = DataFrame::read_csv(options);
  const auto desc = df.describe(false);

  expect(desc.nrows() == 8, "describe should emit 8 statistic rows");
  expect(desc.ncols() == 4, "describe should include all numeric/bool columns");
  expect(desc.case_index().kind() == ThreadKind::Note, "describe index should be textual labels");
  expect(desc.column("age").as_series().scalar_type() == pluss::codem::ScalarType::Float64,
         "describe stats should be float64");

  const auto labels = desc.case_index().as_note();
  expect(labels.at(0) == "count", "describe index label[0] mismatch");
  expect(labels.at(1) == "mean", "describe index label[1] mismatch");
  expect(labels.at(7) == "max", "describe index label[7] mismatch");

  const auto age_values = df.column("age").as_series().to_vec<std::int64_t>();
  double sum_age = 0.0;
  std::int64_t min_age = age_values.front();
  std::int64_t max_age = age_values.front();
  for (const auto v : age_values) {
    sum_age += static_cast<double>(v);
    min_age = std::min(min_age, v);
    max_age = std::max(max_age, v);
  }
  const double mean_age = sum_age / static_cast<double>(age_values.size());

  const auto age_stats = desc.column("age").as_series().to_vec<double>();
  expect(std::fabs(age_stats[0] - static_cast<double>(age_values.size())) < 1e-9, "describe count mismatch");
  expect(std::fabs(age_stats[1] - mean_age) < 1e-9, "describe mean mismatch");
  expect(std::fabs(age_stats[3] - static_cast<double>(min_age)) < 1e-9, "describe min mismatch");
  expect(std::fabs(age_stats[7] - static_cast<double>(max_age)) < 1e-9, "describe max mismatch");
}

void test_view_windowing() {
  const auto path = write_temp_csv(
      "view_window",
      "x,name\n"
      "0,a\n"
      "1,b\n"
      "2,c\n"
      "3,d\n"
      "4,e\n"
      "5,f\n"
      "6,g\n"
      "7,h\n"
      "8,i\n"
      "9,j\n");

  ReadCsvOptions options;
  options.filepath = path.string();
  const auto df = DataFrame::read_csv(options);

  const auto v1 = df.view(5, 10, false);
  expect(v1.nrows() == 5, "view(5,10) should contain 5 rows");
  const auto x1 = v1.column("x").as_series().to_vec<std::int64_t>();
  expect(x1.front() == 5 && x1.back() == 9, "view(5,10) should show rows 5..9");

  const auto v2 = df.view(-5, -1, false);
  expect(v2.nrows() == 5, "view(-5,-1) should contain last 5 rows");
  const auto x2 = v2.column("x").as_series().to_vec<std::int64_t>();
  expect(x2.front() == 5 && x2.back() == 9, "view(-5,-1) should include last row");

  const auto v3 = df.view(0, 3, false);
  expect(v3.nrows() == 3, "view(0,3) should behave like head(3)");
  const auto x3 = v3.column("x").as_series().to_vec<std::int64_t>();
  expect(x3.front() == 0 && x3.back() == 2, "view(0,3) mismatch");

  const auto v4 = df.view(3, -1, false);
  expect(v4.nrows() == 7, "view(3,-1) should include row 3 to last");
  const auto x4 = v4.column("x").as_series().to_vec<std::int64_t>();
  expect(x4.front() == 3 && x4.back() == 9, "view(3,-1) mismatch");
}

} // namespace

int main() {
  using Test = std::pair<std::string, std::function<void()>>;
  const std::vector<Test> tests{
      {"comma_fixture_load_and_case_index", test_comma_fixture_load_and_case_index},
      {"semicolon_fixture_with_index_col", test_semicolon_fixture_with_index_col},
      {"bad_line_policy_and_locale_numeric_parsing", test_bad_line_policy_and_locale_numeric_parsing},
      {"usecols_nrows_skipfooter_and_invalid_source", test_usecols_nrows_skipfooter_and_invalid_source},
      {"display_engine_width_truncation_and_info", test_display_engine_width_truncation_and_info},
      {"describe_numeric_summary", test_describe_numeric_summary},
      {"view_windowing", test_view_windowing},
  };

  std::size_t passed = 0;
  for (const auto& [name, fn] : tests) {
    try {
      fn();
      ++passed;
      fmt::print("[PASS] {}\n", name);
    } catch (const std::exception& e) {
      fmt::print("[FAIL] {}\n{}\n", name, e.what());
      return 1;
    }
  }

  fmt::print("{} / {} tests passed\n", passed, tests.size());
  return 0;
}
