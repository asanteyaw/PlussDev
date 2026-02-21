#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
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

void test_comma_fixture_load_and_cases() {
  ReadCsvOptions options;
  options.nrows = 64;
  options.usecols = {
      "Business Case ID",
      "Agency Name",
      "Agency Project ID",
      "Lifecycle Cost",
      "Updated Date",
      "Updated Time",
  };

  const auto df = DataFrame::load_csv(fixture_path("messy_data.csv").string(), options);

  expect(df.nrows() == 64, "comma fixture: nrows mismatch");
  expect(df.ncols() == 6, "comma fixture: ncols mismatch");
  expect(df.has_column("Business Case ID"), "comma fixture: missing Business Case ID");
  expect(df.column("Business Case ID").kind() == ThreadKind::Series, "comma fixture: expected numeric Series column");
  expect(df.column("Agency Name").kind() == ThreadKind::Note, "comma fixture: expected text Note column");
  expect(df.features("Updated Date").kind() == ThreadKind::Timeline,
         "comma fixture: Updated Date should parse as Timeline");
  expect(df.features("Updated Time").kind() == ThreadKind::Timeline,
         "comma fixture: Updated Time should parse as Timeline");

  const auto agency_project = df.column("Agency Project ID");
  expect(agency_project.has_validity_mask(), "comma fixture: Agency Project ID should carry null mask");

  const auto idx = df.cases_series().to_vec<std::int64_t>();
  expect(!idx.empty(), "comma fixture: cases unexpectedly empty");
  expect(idx.front() == 0, "comma fixture: cases must start at 0");
  expect(idx.back() == 63, "comma fixture: cases last value mismatch");
}

void test_semicolon_fixture_with_case_col() {
  ReadCsvOptions options;
  options.sep = ';';
  options.nrows = 40;
  options.usecols = {"age", "job", "balance", "y"};
  options.case_col = "job";

  const auto df = DataFrame::load_csv(fixture_path("bank-full.csv").string(), options);

  expect(df.nrows() == 40, "semicolon fixture: nrows mismatch");
  expect(df.ncols() == 3, "semicolon fixture: ncols mismatch when case_col is selected");
  expect(df.cases().kind() == ThreadKind::Note, "semicolon fixture: expected text case axis");
  expect(df.column("age").kind() == ThreadKind::Series, "semicolon fixture: age should parse as Series");

  expect_throw([&]() { (void)df.cases_series(); }, "cases_series() should fail when cases are not Series-backed");

  pluss::table::display::DisplayOptions show_opts;
  show_opts.mode = pluss::table::display::DisplayMode::Compact;
  show_opts.terminal_width = 120;

  const auto shown = df.to_string(show_opts);
  expect(shown.find("management") != std::string::npos, "show output should contain case values");
  expect(shown.find("[40 cs x 3 feat]") != std::string::npos, "show output should contain shape footer");
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
  options.sep = ';';
  options.decimal = ',';
  options.thousands = '.';
  options.parse_dates = {"when"};
  options.na_values.insert("na");
  options.on_bad_lines = BadLinePolicy::Skip;

  const auto df = DataFrame::load_csv(path.string(), options);

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
  expect_throw([&]() { (void)DataFrame::load_csv(path.string(), options); },
               "BadLinePolicy::Error should throw on malformed rows");
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
  options.usecols_idx = {0, 2};
  options.nrows = 2;

  const auto first_two = DataFrame::load_csv(path.string(), options);
  expect(first_two.nrows() == 2, "nrows option should cap accepted rows");
  expect(first_two.ncols() == 2, "usecols_idx should project to two columns");

  options.nrows.reset();
  options.skipfooter = 1;
  const auto without_footer = DataFrame::load_csv(path.string(), options);
  expect(without_footer.nrows() == 3, "skipfooter should trim trailing data rows");

  options.usecols_idx = {0, 99};
  expect_throw([&]() { (void)DataFrame::load_csv(path.string(), options); }, "invalid usecols_idx must throw");

  ReadCsvOptions fixture;
  fixture.nrows = 5;
  expect_throw([&]() { (void)DataFrame::load("xlsx", fixture_path("messy_data.csv").string(), fixture); },
               "unsupported DataFrame::load source should throw");
}

void test_display_engine_width_truncation_and_info() {
  ReadCsvOptions options;
  options.sep = ';';
  options.nrows = 80;

  const auto df = DataFrame::load_csv(fixture_path("bank-full.csv").string(), options);

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
  os << df.to_string(show_opts);
  expect(os.str() == shown, "streamed to_string output should match");

  std::ostringstream os_default;
  os_default << df;
  expect(os_default.str() == df.to_string(), "operator<< should match default dataframe rendering");

  const auto info = df.info_string();
  expect(info.find("Class") != std::string::npos, "info output missing class field");
  expect(info.find("pluss::") != std::string::npos, "info output missing class value");
  expect(info.find("Case Range") != std::string::npos, "info output missing case range field");
  expect(info.find("Dtypes") != std::string::npos, "info output missing dtype summary");
  expect(info.find("Memory") != std::string::npos, "info output missing memory field");
}

void test_describe_numeric_summary() {
  ReadCsvOptions options;
  options.sep = ';';
  options.nrows = 30;
  options.usecols = {"age", "balance", "duration", "y"};

  const auto df = DataFrame::load_csv(fixture_path("bank-full.csv").string(), options);
  const auto desc = df.describe(false);

  expect(desc.nrows() == 8, "describe should emit 8 statistic rows");
  expect(desc.ncols() == 4, "describe should include all numeric/bool columns");
  expect(desc.cases().kind() == ThreadKind::Note, "describe cases should be textual labels");
  expect(desc.column("age").as_series().scalar_type() == pluss::codem::ScalarType::Float64,
         "describe stats should be float64");

  const auto labels = desc.cases().as_note();
  expect(labels.at(0) == "count", "describe cases label[0] mismatch");
  expect(labels.at(1) == "mean", "describe cases label[1] mismatch");
  expect(labels.at(7) == "max", "describe cases label[7] mismatch");

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
  const auto df = DataFrame::load_csv(path.string(), options);

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

void test_render_then_stream_suppression_once() {
  const auto path = write_temp_csv(
      "stream_suppress",
      "x,name\n"
      "1,a\n"
      "2,b\n"
      "3,c\n");
  const auto df = DataFrame::load_csv(path.string());

  struct CoutCapture {
    std::ostringstream sink;
    std::streambuf* old{nullptr};
    CoutCapture() : sink(), old(std::cout.rdbuf(sink.rdbuf())) {}
    ~CoutCapture() { std::cout.rdbuf(old); }
  };

  DataFrame h;
  DataFrame t;
  DataFrame v;
  DataFrame d;
  DataFrame s;
  {
    CoutCapture capture;
    h = df.head(2, true);
    t = df.tail(2, true);
    v = df.view(0, 2, true);
    d = df.describe(true);
    s = df.summarize({{"rows", "x", DataFrame::SummaryOp::Count}}, true);
    expect(!capture.sink.str().empty(), "render=true methods should print to cout");
  }

  std::ostringstream os_h;
  os_h << h;
  expect(os_h.str().empty(), "head(render=true) followed by stream should be suppressed once");
  std::ostringstream os_h2;
  os_h2 << h;
  expect(!os_h2.str().empty(), "suppression should be single-use and allow subsequent streaming");

  std::ostringstream os_t;
  os_t << t;
  expect(os_t.str().empty(), "tail(render=true) followed by stream should be suppressed once");

  std::ostringstream os_v;
  os_v << v;
  expect(os_v.str().empty(), "view(render=true) followed by stream should be suppressed once");

  std::ostringstream os_d;
  os_d << d;
  expect(os_d.str().empty(), "describe(render=true) followed by stream should be suppressed once");

  std::ostringstream os_s;
  os_s << s;
  expect(os_s.str().empty(), "summarize(render=true) followed by stream should be suppressed once");

  const auto h_no_render = df.head(2, false);
  std::ostringstream os_normal;
  os_normal << h_no_render;
  expect(!os_normal.str().empty(), "head(render=false) should stream normally");
}

void test_simple_load_config_api_and_auto_sep() {
  const auto df = DataFrame::load_csv(
      fixture_path("bank-full.csv").string(),
      {
          {"nrows", 12},
      });

  expect(df.nrows() == 12, "config load: nrows mismatch");
  expect(df.ncols() > 5, "config load: auto separator should parse full semicolon file");
  expect(df.has_column("age"), "config load: missing expected column");
}

void test_first_class_loader_entrypoints() {
  const auto path = fixture_path("messy_data.csv").string();
  const auto df = pluss::loader::load_csv(path, {{"nrows", 7}});
  expect(df.nrows() == 7, "loader::load_csv config wrapper nrows mismatch");

  const auto df_positional = DataFrame::load_csv(
      fixture_path("bank-full.csv").string(),
      "infer",
      "job",
      "auto",
      8);
  expect(df_positional.nrows() == 8, "DataFrame::load_csv positional ncases mismatch");
  expect(df_positional.cases().kind() == ThreadKind::Note, "DataFrame::load_csv positional case feature mismatch");

  const auto df_positional_short = DataFrame::load_csv(
      fixture_path("bank-full.csv").string(),
      "none",
      static_cast<std::size_t>(6));
  expect(df_positional_short.nrows() == 6, "DataFrame::load_csv short positional ncases mismatch");
  expect(df_positional_short.has_feature("feat_0"), "DataFrame::load_csv short positional should auto-generate feat names");

  const auto df_list_config = DataFrame::load_csv(
      fixture_path("bank-full.csv").string(),
      {
          {"nrows", 10},
          {"columns", {"age", "job"}},
      });
  expect(df_list_config.nrows() == 10, "DataFrame::load_csv config list nrows mismatch");
  expect(df_list_config.ncols() == 2, "DataFrame::load_csv config list should project two features");

  expect_throw(
      [&]() { (void)pluss::loader::load_parquet("not_implemented.parquet"); },
      "loader::load_parquet should throw until parquet support is implemented");
  expect_throw(
      [&]() { (void)DataFrame::load_parquet("not_implemented.parquet"); },
      "DataFrame::load_parquet should throw until parquet support is implemented");
}

void test_col_names_none_and_save_as_csv() {
  const auto path = write_temp_csv(
      "no_header",
      "1;alice\n"
      "2;bob\n"
      "3;carol\n");

  const auto df = DataFrame::load_csv(
      path.string(),
      {
          {"col names", "none"},
      });

  expect(df.nrows() == 3, "col names none: nrows mismatch");
  expect(df.ncols() == 2, "col names none: ncols mismatch");
  expect(df.has_column("feat_0"), "col names none: expected generated feat_0");
  expect(df.has_column("feat_1"), "col names none: expected generated feat_1");

  const auto out_path = write_temp_csv("saved_out", "");
  df.save_csv(
      out_path.string(),
      {
          {"sep", ","},
          {"cases", false},
      });

  const auto back = DataFrame::load_csv(out_path.string());
  expect(back.nrows() == df.nrows(), "save_as csv: roundtrip nrows mismatch");
  expect(back.ncols() == df.ncols(), "save_as csv: roundtrip ncols mismatch");
}

void test_loader_unified_load_result_for_csv() {
  const auto result = pluss::loader::load("csv", fixture_path("messy_data.csv").string(), {{"nrows", 5}});
  expect(result.is_frame(), "loader::load csv should return DataFrame result");
  const auto& df = result.dataframe();
  expect(df.nrows() == 5, "loader::load csv nrows mismatch");
}

void test_feature_value_cases_values_and_save_defaults() {
  ReadCsvOptions options;
  options.sep = ';';
  options.nrows = 12;
  options.usecols = {"age", "job", "balance"};
  const auto df = DataFrame::load_csv(fixture_path("bank-full.csv").string(), options);

  const auto age_series = df.features("age").value().as_series();
  expect(age_series.defined(), "features(\"age\").value() should return defined Series");
  expect(age_series.scalar_type() == pluss::codem::ScalarType::Int64, "features(\"age\").value() should preserve Series dtype");
  expect(age_series.numel() == df.nrows(), "features(\"age\").value() length mismatch");

  const auto all_values = df.values();
  expect(static_cast<std::int64_t>(all_values.size()) == df.ncols(), "df.values() should return one Thread per feature");

  const auto row3 = df.cases(3);
  expect(row3.nrows() == 1, "df.cases(3) should return single-case frame");
  expect(row3.ncols() == df.ncols(), "df.cases(3) should preserve feature count");

  const auto cell_text = df.cases(0, static_cast<std::size_t>(1));
  expect(!cell_text.empty(), "df.cases(row, feature_idx) should return scalar cell text");
  const auto cell_by_name = df.cases(0, "job");
  expect(cell_by_name == cell_text, "df.cases(row, \"job\") should match index-based cell access");

  const auto range = df.cases({2, 6});
  expect(range.nrows() == 4, "df.cases({2,6}) should return 4 cases");

  const auto out_default = write_temp_csv("saved_default", "");
  df.save_csv(out_default.string());
  const auto back_default = DataFrame::load_csv(out_default.string());
  expect(back_default.ncols() == df.ncols(), "save_csv default should not add cases column");
}

void test_parallel_engine_matches_serial() {
  ReadCsvOptions serial;
  serial.sep = ';';
  serial.engine = pluss::loader::CsvEngine::Serial;
  serial.nrows = 120;

  ReadCsvOptions parallel = serial;
  parallel.engine = pluss::loader::CsvEngine::Parallel;
  parallel.parallel_threads = 4;
  parallel.parallel_min_bytes = 1;

  const auto path = fixture_path("bank-full.csv").string();
  const auto df_serial = DataFrame::load_csv(path, serial);
  const auto df_parallel = DataFrame::load_csv(path, parallel);

  expect(df_serial.nrows() == df_parallel.nrows(), "parallel engine: nrows mismatch vs serial");
  expect(df_serial.ncols() == df_parallel.ncols(), "parallel engine: ncols mismatch vs serial");
  expect(df_serial.column_names() == df_parallel.column_names(), "parallel engine: column names mismatch");

  const auto s_age = df_serial.column("age").as_series().to_vec<std::int64_t>();
  const auto p_age = df_parallel.column("age").as_series().to_vec<std::int64_t>();
  expect(s_age == p_age, "parallel engine: age column mismatch vs serial");

  const auto s_job = df_serial.column("job").as_note();
  const auto p_job = df_parallel.column("job").as_note();
  expect(s_job.at(0) == p_job.at(0), "parallel engine: first job value mismatch");
  const auto last = df_serial.nrows() - 1;
  expect(s_job.at(last) == p_job.at(last), "parallel engine: last job value mismatch");
}

void test_versatile_dataframe_families() {
  ReadCsvOptions options;
  options.sep = ';';
  options.nrows = 40;
  options.usecols = {"age", "job", "balance", "duration"};
  const auto df = DataFrame::load_csv(fixture_path("bank-full.csv").string(), options);

  const auto age = df.feature("age").as_series().to_vec<std::int64_t>();
  auto mask = pluss::Series::zeros({static_cast<std::int64_t>(age.size())}, pluss::codem::ScalarType::Bool);
  auto* keep = reinterpret_cast<bool*>(mask.raw_data());
  std::int64_t expected_rows = 0;
  for (std::size_t i = 0; i < age.size(); ++i) {
    keep[i] = age[i] >= 45;
    expected_rows += keep[i] ? 1 : 0;
  }

  const auto q1 = df.query(mask);
  expect(q1.nrows() == expected_rows, "query(mask) should filter rows from boolean mask");

  const auto q2 = df.query([&](const DataFrame&) { return mask; });
  expect(q2.nrows() == expected_rows, "query(predicate) should filter rows from predicate mask");

  std::int64_t expected_known = 0;
  const auto jobs = df.feature("job").as_note();
  for (std::size_t i = 0; i < age.size(); ++i) {
    if (age[i] >= 45 && std::string(jobs.at(static_cast<std::int64_t>(i))) != "unknown") {
      ++expected_known;
    }
  }
  const auto q3 = df.query("age >= 45 and job != 'unknown'");
  expect(q3.nrows() == expected_known, "query(string): combined numeric/string predicate mismatch");

  const auto q4 = df.query("case >= 5 and case < 10");
  expect(q4.nrows() == 5, "query(string): case-axis filtering should work");

  const auto q5 = df.query("contains(job, 'man')");
  expect(q5.nrows() > 0, "query(string): contains(feature, literal) should match rows");

  std::int64_t expected_between = 0;
  std::int64_t expected_in = 0;
  std::int64_t expected_not_in = 0;
  std::int64_t expected_feat_cmp = 0;
  const auto duration = df.feature("duration").as_series().to_vec<std::int64_t>();
  for (std::size_t i = 0; i < age.size(); ++i) {
    if (age[i] >= 45 && age[i] <= 50) {
      ++expected_between;
    }
    const auto job = std::string(jobs.at(static_cast<std::int64_t>(i)));
    const bool in_targets = (job == "management" || job == "technician");
    expected_in += in_targets ? 1 : 0;
    expected_not_in += in_targets ? 0 : 1;
    expected_feat_cmp += (age[i] < duration[i]) ? 1 : 0;
  }

  expect(df.query("age between 45 and 50").nrows() == expected_between,
         "query(string): between should filter inclusive bounds");
  expect(df.query("job in ('management', 'technician')").nrows() == expected_in,
         "query(string): in should filter matching literals");
  expect(df.query("job not in ('management', 'technician')").nrows() == expected_not_in,
         "query(string): not in should filter complement set");
  expect(df.query("age < duration").nrows() == expected_feat_cmp,
         "query(string): feature-to-feature comparison mismatch");

  const auto tmp_nulls = write_temp_csv(
      "query_nulls",
      "id,name\n"
      "1,alice\n"
      "2,\n"
      "3,bob\n");
  const auto df_nulls = DataFrame::load_csv(tmp_nulls.string());
  expect(df_nulls.query("isnull(name)").nrows() == 1, "query(string): isnull(feature) should filter null rows");
  expect(df_nulls.query("name notnull").nrows() == 2, "query(string): postfix notnull should filter non-null rows");

  const auto tmp_backtick = write_temp_csv(
      "query_backtick",
      "feat one,feat_two\n"
      "1,x\n"
      "2,y\n");
  const auto df_backtick = DataFrame::load_csv(tmp_backtick.string());
  expect(df_backtick.query("`feat one` >= 2").nrows() == 1,
         "query(string): backtick identifiers should support spaced feature names");

  const auto with_copy = df.derive({{"age_copy", df.feature("age")}});
  expect(with_copy.ncols() == df.ncols() + 1, "derive(assignments) should append a new feature");
  expect(with_copy.has_feature("age_copy"), "derive(assignments) missing derived feature");

  const auto with_flag = df.derive({{
      "is_senior",
      [&](const DataFrame& frame) {
        const auto ages = frame.feature("age").as_series().to_vec<std::int64_t>();
        auto flag = pluss::Series::zeros({static_cast<std::int64_t>(ages.size())}, pluss::codem::ScalarType::Bool);
        auto* p = reinterpret_cast<bool*>(flag.raw_data());
        for (std::size_t i = 0; i < ages.size(); ++i) {
          p[i] = ages[i] >= 50;
        }
        return pluss::table::Thread(flag);
      },
  }});
  expect(with_flag.has_feature("is_senior"), "derive(specs) missing computed feature");
  expect(with_flag.feature("is_senior").kind() == ThreadKind::Series, "derive(specs) should produce Series-backed flag");

  const auto with_alias = df.assign({{"age_alias", df.feature("age")}});
  expect(with_alias.has_feature("age_alias"), "assign alias should delegate to derive");

  const auto summary = df.summarize(
      {
          {"rows", "age", DataFrame::SummaryOp::Count},
          {"age_mean", "age", DataFrame::SummaryOp::Mean},
          {"jobs", "job", DataFrame::SummaryOp::Nunique},
      },
      false);
  expect(summary.nrows() == 1, "summarize should produce one summary row");
  expect(summary.ncols() == 3, "summarize should emit requested output features");
  expect(summary.feature("rows").as_series().to_vec<std::int64_t>(0) == df.nrows(),
         "summarize count should match frame row count");
  expect(summary.feature("jobs").as_series().to_vec<std::int64_t>(0) > 0,
         "summarize nunique should be positive for job feature");

  const auto agg_out = df.agg({{"age_min", "age", DataFrame::SummaryOp::Min}}, false);
  expect(agg_out.nrows() == 1 && agg_out.ncols() == 1, "agg alias should delegate to summarize");

  const auto numeric = df.features({"age", "balance", "duration"});
  const auto age0 = numeric.feature("age").as_series().to_vec<std::int64_t>(0);

  const auto add_scalar = numeric.add(1.0);
  expect(add_scalar.feature("age").as_series().to_vec<std::int64_t>(0) == age0 + 1,
         "add(scalar) should shift numeric Series features");
  const auto add_frame = numeric.add(numeric);
  expect(add_frame.feature("age").as_series().to_vec<std::int64_t>(0) == age0 * 2,
         "add(frame) should add feature-wise values");

  const auto sub_alias = numeric.subtract(1.0);
  expect(sub_alias.feature("age").as_series().to_vec<std::int64_t>(0) == age0 - 1,
         "subtract alias should delegate to sub");
  const auto mul_alias = numeric.multiply(2.0);
  expect(mul_alias.feature("age").as_series().to_vec<std::int64_t>(0) == age0 * 2,
         "multiply alias should delegate to mul");

  const auto reverse_sub = numeric.rsub(100.0);
  expect(reverse_sub.feature("age").as_series().to_vec<std::int64_t>(0) == 100 - age0,
         "rsub(scalar) should reverse scalar-column subtraction");

  const auto reverse_add = numeric.radd(10.0);
  const auto reverse_mul = numeric.rmul(3.0);
  const auto reverse_div = numeric.rdiv(120.0);
  const auto reverse_true_div = numeric.rtruediv(120.0);
  const auto reverse_floor_div = numeric.rfloordiv(120.0);
  const auto reverse_mod = numeric.rmod(120.0);
  const auto reverse_pow = numeric.rpow(2.0);
  expect(reverse_add.nrows() == numeric.nrows() && reverse_add.ncols() == numeric.ncols(),
         "radd should preserve frame shape");
  expect(reverse_mul.nrows() == numeric.nrows() && reverse_mul.ncols() == numeric.ncols(),
         "rmul should preserve frame shape");
  expect(reverse_div.nrows() == numeric.nrows() && reverse_div.ncols() == numeric.ncols(),
         "rdiv should preserve frame shape");
  expect(reverse_true_div.nrows() == numeric.nrows() && reverse_true_div.ncols() == numeric.ncols(),
         "rtruediv should preserve frame shape");
  expect(reverse_floor_div.nrows() == numeric.nrows() && reverse_floor_div.ncols() == numeric.ncols(),
         "rfloordiv should preserve frame shape");
  expect(reverse_mod.nrows() == numeric.nrows() && reverse_mod.ncols() == numeric.ncols(),
         "rmod should preserve frame shape");
  expect(reverse_pow.nrows() == numeric.nrows() && reverse_pow.ncols() == numeric.ncols(),
         "rpow should preserve frame shape");

  const auto div_alias = numeric.divide(2.0);
  const auto true_div = numeric.truediv(2.0);
  const auto floor_div = numeric.floordiv(2.0);
  const auto mod_alias = numeric.mod(2.0);
  const auto pow_alias = numeric.pow(2.0);
  expect(div_alias.nrows() == numeric.nrows() && div_alias.ncols() == numeric.ncols(),
         "divide alias should preserve frame shape");
  expect(true_div.nrows() == numeric.nrows() && true_div.ncols() == numeric.ncols(),
         "truediv should preserve frame shape");
  expect(floor_div.nrows() == numeric.nrows() && floor_div.ncols() == numeric.ncols(),
         "floordiv should preserve frame shape");
  expect(mod_alias.nrows() == numeric.nrows() && mod_alias.ncols() == numeric.ncols(),
         "mod should preserve frame shape");
  expect(pow_alias.nrows() == numeric.nrows() && pow_alias.ncols() == numeric.ncols(),
         "pow should preserve frame shape");

  const auto eq_self = numeric.eq(numeric);
  const auto ne_self = numeric.ne(numeric);
  const auto ge_scalar = numeric.ge(45.0);
  const auto lt_scalar = numeric.lt(45.0);
  const auto le_scalar = numeric.le(45.0);
  const auto gt_scalar = numeric.gt(45.0);
  expect(eq_self.feature("age").as_series().scalar_type() == pluss::codem::ScalarType::Bool,
         "eq should return bool Series features");
  expect(ne_self.feature("age").as_series().scalar_type() == pluss::codem::ScalarType::Bool,
         "ne should return bool Series features");
  expect(lt_scalar.feature("age").as_series().scalar_type() == pluss::codem::ScalarType::Bool,
         "lt should return bool Series features");
  expect(le_scalar.feature("age").as_series().scalar_type() == pluss::codem::ScalarType::Bool,
         "le should return bool Series features");
  expect(gt_scalar.feature("age").as_series().scalar_type() == pluss::codem::ScalarType::Bool,
         "gt should return bool Series features");
  expect(ge_scalar.feature("age").as_series().scalar_type() == pluss::codem::ScalarType::Bool,
         "ge should return bool Series features");

  const auto eq_age = eq_self.feature("age").as_series().to_vec<bool>();
  const auto ne_age = ne_self.feature("age").as_series().to_vec<bool>();
  expect(std::all_of(eq_age.begin(), eq_age.end(), [](bool v) { return v; }),
         "eq(frame) should be true for identical numeric frames");
  expect(std::none_of(ne_age.begin(), ne_age.end(), [](bool v) { return v; }),
         "ne(frame) should be false for identical numeric frames");

  const auto ge_age = ge_scalar.feature("age").as_series().to_vec<bool>();
  const auto ge_count = static_cast<std::int64_t>(std::count(ge_age.begin(), ge_age.end(), true));
  expect(ge_count == expected_rows, "ge(scalar) should match manual threshold count");

  const auto cmp_alias = numeric.compare(numeric.add(1.0));
  expect(cmp_alias.feature("age").as_series().to_vec<bool>(0),
         "compare alias should delegate to ne for changed values");

  const auto mixed_add = df.add(5.0);
  expect(mixed_add.feature("job").kind() == ThreadKind::Note,
         "add(scalar) should preserve non-numeric features");
  expect(std::string(mixed_add.feature("job").as_note().at(0)) == std::string(df.feature("job").as_note().at(0)),
         "add(scalar) should not alter non-numeric feature values");

  const auto mixed_eq = df.eq(df);
  expect(mixed_eq.feature("job").kind() == ThreadKind::Series,
         "eq(frame) should produce bool output for non-numeric features too");
  expect(mixed_eq.feature("job").as_series().to_vec<bool>(0),
         "eq(frame) should mark identical text values as true");

  ReadCsvOptions missing_opts;
  missing_opts.na_values.insert("NA");
  const auto missing_path = write_temp_csv(
      "missing_ops",
      "id,score,label\n"
      "1,10,a\n"
      "2,,NA\n"
      "3,30,c\n"
      "4,,d\n");
  const auto miss = DataFrame::load_csv(missing_path.string(), missing_opts);

  const auto miss_isnull = miss.missing(DataFrame::MissingOp::IsNull);
  expect(miss_isnull.ncols() == miss.ncols(), "missing(isnull) should retain all features by default");
  expect(miss_isnull.feature("score").as_series().scalar_type() == pluss::codem::ScalarType::Bool,
         "missing(isnull) should return bool mask features");
  expect(miss_isnull.feature("score").as_series().to_vec<bool>(1),
         "missing(isnull) should mark null score rows");

  const auto miss_notnull = miss.missing(DataFrame::MissingOp::NotNull);
  expect(!miss_notnull.feature("score").as_series().to_vec<bool>(1),
         "missing(notnull) should invert isnull mask");

  const auto miss_filled = miss.missing(
      {DataFrame::MissingOp::Fill, {"score"}, 0.0});
  expect(miss_filled.feature("score").as_series().to_vec<std::int64_t>(1) == 0,
         "missing(fill) should replace selected numeric nulls");
  expect(std::string(miss_filled.feature("label").as_note().at(1)) == std::string(miss.feature("label").as_note().at(1)),
         "missing(fill) on selected features should not mutate unselected features");

  const auto miss_ffill = miss.ffill();
  expect(miss_ffill.feature("score").as_series().to_vec<std::int64_t>(1) == 10,
         "ffill should propagate previous numeric value");
  expect(std::string(miss_ffill.feature("label").as_note().at(1)) == "a",
         "ffill should propagate previous Note value");

  const auto miss_bfill = miss.bfill();
  expect(miss_bfill.feature("score").as_series().to_vec<std::int64_t>(1) == 30,
         "bfill should propagate next numeric value");
  expect(miss_bfill.feature("score").has_validity_mask(),
         "bfill should preserve trailing null when no backward source exists");
  expect(!miss_bfill.feature("score").validity_mask()->to_vec<bool>(3),
         "bfill should keep trailing nulls unresolved");

  const auto miss_drop_any = miss.dropna();
  expect(miss_drop_any.nrows() == 2, "dropna should drop rows with any null by default");

  const auto miss_drop_all = miss.missing(DataFrame::MissingOp::DropAll, {"score", "label"});
  expect(miss_drop_all.nrows() == 3, "missing(drop all) should drop rows only when all selected features are null");

  const auto arranged_age = df.arrange({"age"});
  const auto arranged_age_values = arranged_age.feature("age").as_series().to_vec<std::int64_t>();
  for (std::size_t i = 1; i < arranged_age_values.size(); ++i) {
    expect(arranged_age_values[i - 1] <= arranged_age_values[i],
           "arrange(feature) should sort ascending by default");
  }

  const auto arranged_score = miss.arrange({"score"});
  const auto arranged_score_ids = arranged_score.feature("id").as_series().to_vec<std::int64_t>();
  expect(arranged_score_ids[0] == 1 && arranged_score_ids[1] == 3 && arranged_score_ids[2] == 2 && arranged_score_ids[3] == 4,
         "arrange(score): expected ascending values with nulls last and stable tie order");

  const auto arranged_score_desc_nulls_first = miss.arrange({{
      "score",
      false,
      false,
  }});
  const auto arranged_score_desc_ids = arranged_score_desc_nulls_first.feature("id").as_series().to_vec<std::int64_t>();
  expect(arranged_score_desc_ids[0] == 2 && arranged_score_desc_ids[1] == 4
             && arranged_score_desc_ids[2] == 3 && arranged_score_desc_ids[3] == 1,
         "arrange(spec): descending with nulls first should position null cases first");

  const auto cast_age = df.astype({{"age", pluss::codem::ScalarType::Float64}});
  expect(cast_age.feature("age").as_series().scalar_type() == pluss::codem::ScalarType::Float64,
         "astype should cast selected Series feature dtype");

  const auto cast_to_note = df.cast({{
      "age",
      pluss::table::ThreadCastTarget::Note,
      std::nullopt,
  }});
  expect(cast_to_note.feature("age").kind() == ThreadKind::Note,
         "cast(spec): Series -> Note should change logical payload type");

  const auto note_numeric_path = write_temp_csv(
      "cast_note_numeric",
      "v\n"
      "1\n"
      "2.5\n"
      "oops\n");
  ReadCsvOptions note_opts;
  note_opts.header = "none";
  const auto note_df = DataFrame::load_csv(note_numeric_path.string(), note_opts);
  const auto note_feature = note_df.feature_names().front();
  const auto note_cast = note_df.cast({{
      note_feature,
      pluss::table::ThreadCastTarget::Series,
      pluss::codem::ScalarType::Float64,
  }});
  expect(note_cast.feature(note_feature).kind() == ThreadKind::Series,
         "cast(spec): Note -> Series should convert payload kind");
  expect(note_cast.feature(note_feature).has_validity_mask(),
         "cast(spec): Note -> Series should set validity mask when parse fails");
  expect(!note_cast.feature(note_feature).validity_mask()->to_vec<bool>(2),
         "cast(spec): unparsable Note value should become null");

  ReadCsvOptions timeline_opts;
  timeline_opts.nrows = 8;
  timeline_opts.usecols = {"Updated Date"};
  const auto timeline_df = DataFrame::load_csv(fixture_path("messy_data.csv").string(), timeline_opts);
  const auto timeline_cast = timeline_df.cast({{
      "Updated Date",
      pluss::table::ThreadCastTarget::Note,
      std::nullopt,
  }});
  expect(timeline_cast.feature("Updated Date").kind() == ThreadKind::Note,
         "cast(spec): Timeline -> Note should delegate to underlying timeline string formatting");

  const auto rolling_mean = numeric.window({{
      "age_roll_mean",
      "age",
      pluss::table::ThreadWindowOp::RollingMean,
      3,
      2,
  }},
                                          false);
  expect(rolling_mean.nrows() == numeric.nrows(), "window(keep_input=false) should preserve case count");
  expect(rolling_mean.ncols() == 1, "window(keep_input=false) should emit only computed features");
  expect(rolling_mean.has_feature("age_roll_mean"), "window should create requested output feature name");
  expect(rolling_mean.feature("age_roll_mean").kind() == ThreadKind::Series,
         "window output should be Series-backed");
  expect(rolling_mean.feature("age_roll_mean").as_series().scalar_type() == pluss::codem::ScalarType::Float64,
         "window numeric outputs should be float64");
  expect(rolling_mean.feature("age_roll_mean").has_validity_mask(),
         "window with min_periods > 1 should produce a validity mask");
  expect(!rolling_mean.feature("age_roll_mean").validity_mask()->to_vec<bool>(0),
         "window first row should be null when min_periods is not met");

  const auto age_i64 = numeric.feature("age").as_series().to_vec<std::int64_t>();
  const auto mean_v = rolling_mean.feature("age_roll_mean").as_series().to_vec<double>();
  expect(std::fabs(mean_v[1] - (static_cast<double>(age_i64[0] + age_i64[1]) / 2.0)) < 1e-9,
         "window rolling mean row1 mismatch");
  expect(std::fabs(mean_v[2] - (static_cast<double>(age_i64[0] + age_i64[1] + age_i64[2]) / 3.0)) < 1e-9,
         "window rolling mean row2 mismatch");

  const auto rolling_sum_keep_input = numeric.rolling({{
      "",
      "age",
      pluss::table::ThreadWindowOp::RollingSum,
      3,
      1,
  }},
                                                       true);
  expect(rolling_sum_keep_input.ncols() == numeric.ncols() + 1,
         "rolling(keep_input=true) should append computed feature");
  expect(rolling_sum_keep_input.has_feature("age_rollsum_3"),
         "rolling default output name should use feature/op/window policy");

  const auto score_roll_max = miss.window({{
      "score_rollmax",
      "score",
      pluss::table::ThreadWindowOp::RollingMax,
      2,
      1,
  }},
                                       false);
  const auto score_roll_max_vals = score_roll_max.feature("score_rollmax").as_series().to_vec<double>();
  expect(!score_roll_max.feature("score_rollmax").has_validity_mask(),
         "rolling max(min_periods=1) should clear mask when every output row is valid");
  expect(std::fabs(score_roll_max_vals[1] - 10.0) < 1e-9,
         "rolling max should ignore null in two-row window");

  const auto score_roll_min_minp2 = miss.window({{
      "score_rollmin",
      "score",
      pluss::table::ThreadWindowOp::RollingMin,
      2,
      2,
  }},
                                              false);
  const auto score_roll_min_valid = score_roll_min_minp2.feature("score_rollmin").validity_mask()->to_vec<bool>();
  expect(!score_roll_min_valid[1] && !score_roll_min_valid[3],
         "rolling min should be null when min_periods is not met due nulls");
}

} // namespace

int main() {
  using Test = std::pair<std::string, std::function<void()>>;
  const std::vector<Test> tests{
      {"comma_fixture_load_and_cases", test_comma_fixture_load_and_cases},
      {"semicolon_fixture_with_case_col", test_semicolon_fixture_with_case_col},
      {"bad_line_policy_and_locale_numeric_parsing", test_bad_line_policy_and_locale_numeric_parsing},
      {"usecols_nrows_skipfooter_and_invalid_source", test_usecols_nrows_skipfooter_and_invalid_source},
      {"display_engine_width_truncation_and_info", test_display_engine_width_truncation_and_info},
      {"describe_numeric_summary", test_describe_numeric_summary},
      {"view_windowing", test_view_windowing},
      {"render_then_stream_suppression_once", test_render_then_stream_suppression_once},
      {"simple_load_config_api_and_auto_sep", test_simple_load_config_api_and_auto_sep},
      {"first_class_loader_entrypoints", test_first_class_loader_entrypoints},
      {"col_names_none_and_save_as_csv", test_col_names_none_and_save_as_csv},
      {"loader_unified_load_result_for_csv", test_loader_unified_load_result_for_csv},
      {"feature_value_cases_values_and_save_defaults", test_feature_value_cases_values_and_save_defaults},
      {"parallel_engine_matches_serial", test_parallel_engine_matches_serial},
      {"versatile_dataframe_families", test_versatile_dataframe_families},
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
