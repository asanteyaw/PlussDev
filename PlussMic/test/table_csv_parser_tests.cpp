#include <algorithm>
#include <array>
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

std::filesystem::path write_temp_text_file(const std::string& stem,
                                           const std::string& ext,
                                           const std::string& contents) {
  const auto dir = std::filesystem::temp_directory_path() / "plussmic_text_tests";
  std::filesystem::create_directories(dir);
  const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  const auto filename = fmt::format("{}_{}.{}", stem, stamp, ext);
  const auto path = dir / filename;

  std::ofstream out(path, std::ios::out | std::ios::trunc | std::ios::binary);
  expect(out.is_open(), "failed to create temporary text fixture");
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
  options.ncases = 64;
  options.usefeats = {
      "Business Case ID",
      "Agency Name",
      "Agency Project ID",
      "Lifecycle Cost",
      "Updated Date",
      "Updated Time",
  };

  const auto df = DataFrame::load_csv(fixture_path("messy_data.csv").string(), options);

  expect(df.ncases() == 64, "comma fixture: nrows mismatch");
  expect(df.nfeats() == 6, "comma fixture: ncols mismatch");
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
  options.ncases = 40;
  options.usefeats = {"age", "job", "balance", "y"};
  options.case_col = "job";

  const auto df = DataFrame::load_csv(fixture_path("bank-full.csv").string(), options);

  expect(df.ncases() == 40, "semicolon fixture: nrows mismatch");
  expect(df.nfeats() == 3, "semicolon fixture: ncols mismatch when case_col is selected");
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

  expect(df.ncases() == 4, "skip bad lines: expected malformed row to be removed");
  expect(df.nfeats() == 5, "skip bad lines: ncols mismatch");
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

void test_usefeats_ncases_skipfooter_and_invalid_source() {
  const auto path = write_temp_csv(
      "limits",
      "a,b,c\n"
      "0,10,x\n"
      "1,11,y\n"
      "2,12,z\n"
      "3,13,w\n");

  ReadCsvOptions options;
  options.usefeats_idx = {0, 2};
  options.ncases = 2;

  const auto first_two = DataFrame::load_csv(path.string(), options);
  expect(first_two.ncases() == 2, "ncases option should cap accepted cases");
  expect(first_two.nfeats() == 2, "usefeats_idx should project to two features");

  options.ncases.reset();
  options.skipfooter = 1;
  const auto without_footer = DataFrame::load_csv(path.string(), options);
  expect(without_footer.ncases() == 3, "skipfooter should trim trailing data rows");

  options.usefeats_idx = {0, 99};
  expect_throw([&]() { (void)DataFrame::load_csv(path.string(), options); }, "invalid usefeats_idx must throw");

  ReadCsvOptions fixture;
  fixture.ncases = 5;
  expect_throw([&]() { (void)DataFrame::load("xlsx", fixture_path("messy_data.csv").string(), fixture); },
               "unsupported DataFrame::load source should throw");
}

void test_display_engine_width_truncation_and_info() {
  ReadCsvOptions options;
  options.sep = ';';
  options.ncases = 80;

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

void test_html_display_renderers_and_backend_config() {
  const auto df = DataFrame({
      {"city", pluss::table::Thread(pluss::make_note({"Edmonton", "Calgary", "Banff"}))},
      {"count", pluss::table::Thread(pluss::make_series(std::vector<std::int64_t>{12, 8, 3}))},
  });

  pluss::table::display::DisplayOptions opts;
  opts.window_title = "prices - describe";

  const auto html = pluss::table::display::render_html(df, opts);
  expect(html.find("<table id=\"plussmic-table\"") != std::string::npos, "html display should render a table");
  expect(html.find("Search visible rows") != std::string::npos, "html display should include search control");
  expect(html.find("Copy visible") != std::string::npos, "html display should include copy control");
  expect(html.find("sortable") != std::string::npos, "html display should mark sortable headers");
  expect(html.find("Edmonton") != std::string::npos, "html display should include cell values");
  expect(html.find("prices - describe") != std::string::npos,
         "html display should surface the resolved view title in the document");
  expect(html.find("PlussMic DataFrame") == std::string::npos,
         "html display should not keep the old hard-coded header title");

  const auto info_html = pluss::table::display::render_info_html(df, opts);
  expect(info_html.find("Feature inventory") != std::string::npos, "info html should include feature inventory");
  expect(info_html.find("Null Case Rows") != std::string::npos, "info html should include summary cards");
  expect(info_html.find("pluss::table::DataFrame") != std::string::npos, "info html should include class name");
  expect(info_html.find("prices - describe") != std::string::npos,
         "info html should honor the resolved display title");

  expect(pluss::table::display::default_backend() == pluss::table::display::DisplayBackend::Auto,
         "display backend should default to smart auto mode");
}

void test_atlas_html_display_renderer() {
  const auto atlas = pluss::table::Atlas(
      {
          DataFrame({
              {"city", pluss::table::Thread(pluss::make_note({"Edmonton", "Calgary"}))},
              {"count", pluss::table::Thread(pluss::make_series(std::vector<std::int64_t>{12, 8}))},
          }),
          DataFrame({
              {"city", pluss::table::Thread(pluss::make_note({"Banff", "Jasper"}))},
              {"count", pluss::table::Thread(pluss::make_series(std::vector<std::int64_t>{3, 2}))},
          }),
      },
      pluss::table::Thread(pluss::make_note({"weekdays", "weekends"})));

  pluss::table::display::DisplayOptions opts;
  const auto html = pluss::table::display::render_html(atlas, opts);

  expect(html.find("Frames") != std::string::npos, "atlas html should include frame navigation label");
  expect(html.find("atlas-tab active") != std::string::npos, "atlas html should mark the initial frame tab active");
  expect(html.find("weekdays") != std::string::npos, "atlas html should surface depth labels");
  expect(html.find("weekends") != std::string::npos, "atlas html should include all frame selectors");
  expect(html.find("atlas-panel active") != std::string::npos, "atlas html should include a visible frame panel");
  expect(html.find("plussmic-table-0") != std::string::npos, "atlas html should embed per-frame tables");
  expect(html.find("Edmonton") != std::string::npos, "atlas html should include the first frame contents");
  expect(html.find("Banff") != std::string::npos, "atlas html should include the second frame contents");
}

void test_describe_numeric_summary() {
  ReadCsvOptions options;
  options.sep = ';';
  options.ncases = 30;
  options.usefeats = {"age", "balance", "duration", "y"};

  const auto df = DataFrame::load_csv(fixture_path("bank-full.csv").string(), options);
  const auto desc = df.describe(false);

  expect(desc.ncases() == 8, "describe should emit 8 statistic rows");
  expect(desc.nfeats() == 4, "describe should include all numeric/bool columns");
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
  expect(v1.ncases() == 5, "view(5,10) should contain 5 rows");
  const auto x1 = v1.column("x").as_series().to_vec<std::int64_t>();
  expect(x1.front() == 5 && x1.back() == 9, "view(5,10) should show rows 5..9");

  const auto v2 = df.view(-5, -1, false);
  expect(v2.ncases() == 5, "view(-5,-1) should contain last 5 rows");
  const auto x2 = v2.column("x").as_series().to_vec<std::int64_t>();
  expect(x2.front() == 5 && x2.back() == 9, "view(-5,-1) should include last row");

  const auto v3 = df.view(0, 3, false);
  expect(v3.ncases() == 3, "view(0,3) should behave like head(3)");
  const auto x3 = v3.column("x").as_series().to_vec<std::int64_t>();
  expect(x3.front() == 0 && x3.back() == 2, "view(0,3) mismatch");

  const auto v4 = df.view(3, -1, false);
  expect(v4.ncases() == 7, "view(3,-1) should include row 3 to last");
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

  {
    CoutCapture capture;
    df.head(2);
    df.tail(2);
    df.view(0, 2);
    df.describe();
    df.aggregate({{"cases", "x", DataFrame::SummaryOp::Count}});
    expect(!capture.sink.str().empty(), "default display methods should render to cout");
  }

  const auto head_text = df.head(2, false).to_string() + "\n";
  const auto view_text = df.view(0, 2, false).to_string() + "\n";

  {
    CoutCapture capture;
    std::ostringstream os;
    os << df.head(2);
    expect(capture.sink.str().empty(), "streaming a view should consume pending auto-render");
    expect(os.str() == head_text.substr(0, head_text.size() - 1), "streamed head view mismatch");
  }

  {
    CoutCapture capture;
    pluss::table::display::DisplayOptions opts;
    opts.backend = pluss::table::display::DisplayBackend::Terminal;
    df.head(2).show(opts);
    expect(capture.sink.str() == head_text, "head().show(terminal) should print exactly once");
  }

  {
    CoutCapture capture;
    pluss::table::display::DisplayOptions opts;
    opts.backend = pluss::table::display::DisplayBackend::Terminal;
    df.view(0, 2).show(opts);
    expect(capture.sink.str() == view_text, "view().show(terminal) should print exactly once");
  }

  std::ostringstream os_normal;
  os_normal << df.head(2, false);
  expect(!os_normal.str().empty(), "head(render=false) should stream normally");
}

void test_dataframe_titles_mutable_and_immutable() {
  const auto path = write_temp_csv(
      "titles",
      "x,y\n"
      "1,10\n"
      "2,20\n"
      "3,30\n");
  const auto df = DataFrame::load_csv(path.string());

  expect(df.title().empty(), "untitled DataFrame should start with an empty title");

  const auto prices = df.title("prices");
  expect(df.title().empty(), "immutable title() should not mutate the source frame");
  expect(prices.title() == "prices", "immutable title() should stamp the returned frame");

  auto mutable_prices = prices;
  mutable_prices.title_("features");
  expect(prices.title() == "prices", "title_() should not mutate sibling copies");
  expect(mutable_prices.title() == "features", "title_() should overwrite the current frame title");

  const auto renamed = mutable_prices.title("derived");
  expect(mutable_prices.title() == "features", "immutable title() should preserve the current frame");
  expect(renamed.title() == "derived", "immutable title() should support chaining from a titled frame");
}

void test_simple_load_config_api_and_auto_sep() {
  const auto df = DataFrame::load_csv(
      fixture_path("bank-full.csv").string(),
      {
          {"ncases", 12},
      });

  expect(df.ncases() == 12, "config load: nrows mismatch");
  expect(df.nfeats() > 5, "config load: auto separator should parse full semicolon file");
  expect(df.has_column("age"), "config load: missing expected column");
}

void test_first_class_loader_entrypoints() {
  const auto path = fixture_path("messy_data.csv").string();
  const auto df = pluss::loader::load_csv(path, {{"ncases", 7}});
  expect(df.ncases() == 7, "loader::load_csv config wrapper nrows mismatch");

  const auto df_positional = DataFrame::load_csv(
      fixture_path("bank-full.csv").string(),
      "infer",
      "job",
      "auto",
      8);
  expect(df_positional.ncases() == 8, "DataFrame::load_csv positional ncases mismatch");
  expect(df_positional.cases().kind() == ThreadKind::Note, "DataFrame::load_csv positional case feature mismatch");

  const auto df_positional_short = DataFrame::load_csv(
      fixture_path("bank-full.csv").string(),
      "none",
      static_cast<std::size_t>(6));
  expect(df_positional_short.ncases() == 6, "DataFrame::load_csv short positional ncases mismatch");
  expect(df_positional_short.has_feature("feat_0"), "DataFrame::load_csv short positional should auto-generate feat names");

  const auto df_list_config = DataFrame::load_csv(
      fixture_path("bank-full.csv").string(),
      {
          {"ncases", 10},
          {"features", {"age", "job"}},
      });
  expect(df_list_config.ncases() == 10, "DataFrame::load_csv config list nrows mismatch");
  expect(df_list_config.nfeats() == 2, "DataFrame::load_csv config list should project two features");

  expect_throw(
      [&]() { (void)pluss::loader::load_parquet("not_implemented.parquet"); },
      "loader::load_parquet should throw on invalid parquet path");
  expect_throw(
      [&]() { (void)DataFrame::load_parquet("not_implemented.parquet"); },
      "DataFrame::load_parquet should throw on invalid parquet path");
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

  expect(df.ncases() == 3, "col names none: nrows mismatch");
  expect(df.nfeats() == 2, "col names none: ncols mismatch");
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
  expect(back.ncases() == df.ncases(), "save_as csv: roundtrip nrows mismatch");
  expect(back.nfeats() == df.nfeats(), "save_as csv: roundtrip ncols mismatch");
}

void test_loader_unified_load_result_for_csv() {
  const auto result = pluss::loader::load("csv", fixture_path("messy_data.csv").string(), {{"ncases", 5}});
  expect(result.is_frame(), "loader::load csv should return DataFrame result");
  const auto& df = result.dataframe();
  expect(df.ncases() == 5, "loader::load csv nrows mismatch");
}

void test_txt_loader_modes() {
  const auto whitespace_path = write_temp_text_file(
      "whitespace",
      "txt",
      "id city score when\n"
      "1 Accra 10.5 2024-01-01T00:00:00Z\n"
      "2 Kumasi 11.0 2024-01-02T00:00:00Z\n");

  const auto whitespace_df = DataFrame::load_txt(
      whitespace_path.string(),
      {
          {"txt_mode", "whitespace"},
          {"header", true},
      });
  expect(whitespace_df.ncases() == 2, "txt whitespace: nrows mismatch");
  expect(whitespace_df.nfeats() == 4, "txt whitespace: ncols mismatch");
  expect(whitespace_df.features("id").kind() == ThreadKind::Series, "txt whitespace: id should infer Series");
  expect(whitespace_df.features("city").kind() == ThreadKind::Note, "txt whitespace: city should infer Note");
  expect(whitespace_df.features("when").kind() == ThreadKind::Timeline, "txt whitespace: when should infer Timeline");

  const auto fixed_width_path = write_temp_text_file(
      "fixed_width",
      "txt",
      "id  city         score\n"
      "1   Cape Coast   10.5\n"
      "2   Tema         12.0\n");
  const auto fixed_df = DataFrame::load(
      "txt",
      fixed_width_path.string(),
      {
          {"txt_mode", "fixed_width"},
          {"header", true},
      });
  expect(fixed_df.ncases() == 2, "txt fixed-width: nrows mismatch");
  expect(fixed_df.nfeats() == 3, "txt fixed-width: ncols mismatch");
  expect(std::string(fixed_df.features("city").as_note().at(0)) == "Cape Coast",
         "txt fixed-width: city tokenization mismatch");

  const auto auto_path = write_temp_text_file(
      "auto_tab",
      "txt",
      "id\tcity\tflag\n"
      "1\tAccra\ttrue\n"
      "2\tTema\tfalse\n");
  const auto auto_df = DataFrame::load_txt(auto_path.string(), {{"header", true}});
  expect(auto_df.ncases() == 2, "txt auto: nrows mismatch");
  expect(auto_df.nfeats() == 3, "txt auto: ncols mismatch");
  expect(auto_df.features("flag").kind() == ThreadKind::Series, "txt auto: bool should infer Series");

  const auto txt_result = pluss::loader::load("txt", auto_path.string(), {{"header", true}});
  expect(txt_result.is_frame(), "loader::load(txt) should return frame");
  expect(txt_result.dataframe().nfeats() == 3, "loader::load(txt) feature count mismatch");
}

void test_json_loader_entrypoints() {
  const auto json_path = write_temp_text_file(
      "records",
      "json",
      R"([
  {"id": 1, "city": "Accra", "when": "2024-01-01T00:00:00Z", "score": 1.5},
  {"id": 2, "city": "Kumasi", "score": 2.0},
  {"id": 3, "city": "Tema", "when": "2024-01-03T00:00:00Z"}
])");

  const auto df = DataFrame::load_json(json_path.string());
  expect(df.ncases() == 3, "json loader: nrows mismatch");
  expect(df.nfeats() == 4, "json loader: ncols mismatch");
  expect(df.features("id").kind() == ThreadKind::Series, "json loader: id should infer Series");
  expect(df.features("city").kind() == ThreadKind::Note, "json loader: city should infer Note");
  expect(df.features("when").kind() == ThreadKind::Timeline, "json loader: when should infer Timeline");
  expect(df.features("score").has_validity_mask(), "json loader: score should carry validity mask");

  const auto two_rows = DataFrame::load("json", json_path.string(), {{"ncases", 2}});
  expect(two_rows.ncases() == 2, "DataFrame::load(json, nrows) should honor nrows");

  const auto raw = pluss::loader::read_json_raw_batch(
      json_path.string(),
      {
          {"usefeats", {"id", "city"}},
      });
  expect(raw.features.size() == 2, "read_json_raw_batch(usefeats) should project feature count");
  expect(raw.features[0].name == "id" && raw.features[1].name == "city",
         "read_json_raw_batch(usefeats) should preserve requested order");

  const auto loaded = pluss::loader::load("json", json_path.string());
  expect(loaded.is_frame(), "loader::load(json) should return frame");
  expect(loaded.dataframe().ncases() == 3, "loader::load(json) nrows mismatch");
}

void test_save_targets_and_save_as_parity() {
  const auto df = DataFrame::load_csv(
      fixture_path("bank-full.csv").string(),
      {
          {"ncases", 6},
          {"features", {"age", "job", "balance"}},
      });
  expect(df.ncases() == 6, "save parity fixture nrows mismatch");

  const auto txt_path = write_temp_text_file("save_parity_df", "txt", "");
  df.save_txt(txt_path.string());
  const auto txt_back = DataFrame::load_txt(txt_path.string(), {{"header", true}});
  expect(txt_back.ncases() == df.ncases(), "save_txt -> load_txt row count mismatch");
  expect(txt_back.nfeats() == df.nfeats(), "save_txt -> load_txt col count mismatch");

  const auto json_path = write_temp_text_file("save_parity_df", "json", "");
  df.save_json(json_path.string());
  const auto json_back = DataFrame::load_json(json_path.string());
  expect(json_back.ncases() == df.ncases(), "save_json -> load_json row count mismatch");
  expect(json_back.nfeats() == df.nfeats(), "save_json -> load_json col count mismatch");

  const auto txt_as_path = write_temp_text_file("save_as_txt_df", "txt", "");
  df.save_as("txt", txt_as_path.string(), {{"header", true}});
  const auto txt_as_back = DataFrame::load_txt(txt_as_path.string(), {{"header", true}});
  expect(txt_as_back.ncases() == df.ncases(), "save_as(txt) row count mismatch");

  const auto json_as_path = write_temp_text_file("save_as_json_df", "json", "");
  df.save_as("json", json_as_path.string(), {{"orient", "split"}});
  const auto json_as_back = DataFrame::load_json(json_as_path.string());
  expect(json_as_back.ncases() == df.ncases(), "save_as(json) row count mismatch");
}

void test_parquet_save_and_load_parity() {
  const auto df = DataFrame::load_csv(
      fixture_path("bank-full.csv").string(),
      {
          {"ncases", 6},
          {"features", {"age", "job", "balance"}},
      });

  const auto parquet_path = write_temp_text_file("save_parity_df", "parquet", "");
  try {
    df.save_parquet(parquet_path.string());
  } catch (const std::exception& e) {
    const std::string msg = e.what();
    if (msg.find("built without parquet-cpp support") != std::string::npos) {
      return; // Expected in builds without native parquet support.
    }
    throw std::runtime_error("save_parquet failed unexpectedly: " + msg);
  }

  const auto back = DataFrame::load_parquet(parquet_path.string());
  expect(back.ncases() == df.ncases(), "save_parquet -> load_parquet row count mismatch");
  expect(back.nfeats() == df.nfeats(), "save_parquet -> load_parquet col count mismatch");
}

void test_feature_value_cases_values_and_save_defaults() {
  ReadCsvOptions options;
  options.sep = ';';
  options.ncases = 12;
  options.usefeats = {"age", "job", "balance"};
  const auto df = DataFrame::load_csv(fixture_path("bank-full.csv").string(), options);

  const auto age_series = df.features("age").value().as_series();
  expect(age_series.defined(), "features(\"age\").value() should return defined Series");
  expect(age_series.scalar_type() == pluss::codem::ScalarType::Int64, "features(\"age\").value() should preserve Series dtype");
  expect(age_series.numel() == df.ncases(), "features(\"age\").value() length mismatch");

  const auto all_values = df.values();
  expect(static_cast<std::int64_t>(all_values.size()) == df.nfeats(), "df.values() should return one Thread per feature");

  const auto row3 = df.cases(3);
  expect(row3.ncases() == 1, "df.cases(3) should return single-case frame");
  expect(row3.nfeats() == df.nfeats(), "df.cases(3) should preserve feature count");

  const auto cell_text = df.cases(0, static_cast<std::size_t>(1));
  expect(!cell_text.empty(), "df.cases(row, feature_idx) should return scalar cell text");
  const auto cell_by_name = df.cases(0, "job");
  expect(cell_by_name == cell_text, "df.cases(row, \"job\") should match index-based cell access");

  const auto range = df.cases({2, 6});
  expect(range.ncases() == 4, "df.cases({2,6}) should return 4 cases");

  const auto out_default = write_temp_csv("saved_default", "");
  df.save_csv(out_default.string());
  const auto back_default = DataFrame::load_csv(out_default.string());
  expect(back_default.nfeats() == df.nfeats(), "save_csv default should not add cases column");
}

void test_transpose_shape_and_axes() {
  const auto path = write_temp_csv(
      "transpose_basic",
      "a,b,c\n"
      "1,2,x\n"
      "3,4,y\n"
      "5,6,z\n");
  const auto df = DataFrame::load_csv(path.string());
  const auto t = df.transpose(false);

  expect(t.ncases() == df.nfeats(), "transpose should swap row/feature dimensions (rows)");
  expect(t.nfeats() == df.ncases(), "transpose should swap row/feature dimensions (features)");
  expect(t.cases().kind() == ThreadKind::Note, "transpose case axis should be Note-backed feature labels");

  const auto tcases = t.cases().as_note();
  expect(std::string(tcases.at(0)) == "a", "transpose cases[0] should equal first source feature name");
  expect(std::string(tcases.at(2)) == "c", "transpose cases[last] should equal last source feature name");

  expect(t.has_feature("0"), "transpose should expose source-case labels as output feature names");
  const auto feature0 = t.features("0").as_note();
  expect(std::fabs(std::stod(std::string(feature0.at(0))) - 1.0) < 1e-9,
         "transpose cell mismatch at [feature=0, case=a]");
  expect(std::string(feature0.at(2)) == "x", "transpose should preserve text values via Note casting");
}

void test_squeeze_unsqueeze_bridge() {
  const auto path = write_temp_csv(
      "squeeze_unsqueeze",
      "a,b\n"
      "1,x\n"
      "2,y\n"
      "3,z\n");
  const auto df = DataFrame::load_csv(path.string());

  const auto atlas = df.unsqueeze();
  expect(atlas.depth() == 1, "unsqueeze should produce depth-1 atlas");
  expect(atlas.ncases() == df.ncases(), "unsqueeze should preserve case count");
  expect(atlas.nfeats() == df.nfeats(), "unsqueeze should preserve feature count");
  expect(atlas.squeeze().equals(df), "squeeze(unsqueeze(df)) should roundtrip");

  const auto label = pluss::table::Thread(
      pluss::make_note(std::vector<std::string>{"batch_0"}, std::array<std::int64_t, 1>{1}));
  const auto labeled = df.unsqueeze(label);
  expect(labeled.axis0_labels().has_value(), "unsqueeze(depth_labels) should attach labels");
  expect(labeled.squeeze().equals(df), "squeeze should preserve frame contents");

  const auto atlas2 = pluss::table::Atlas({df, df});
  expect_throw([&]() { (void)atlas2.squeeze(); }, "squeeze should throw when depth != 1");
}

void test_parallel_engine_matches_serial() {
  ReadCsvOptions serial;
  serial.sep = ';';
  serial.engine = pluss::loader::CsvEngine::Serial;
  serial.ncases = 120;

  ReadCsvOptions parallel = serial;
  parallel.engine = pluss::loader::CsvEngine::Parallel;
  parallel.parallel_threads = 4;
  parallel.parallel_min_bytes = 1;

  const auto path = fixture_path("bank-full.csv").string();
  const auto df_serial = DataFrame::load_csv(path, serial);
  const auto df_parallel = DataFrame::load_csv(path, parallel);

  expect(df_serial.ncases() == df_parallel.ncases(), "parallel engine: nrows mismatch vs serial");
  expect(df_serial.nfeats() == df_parallel.nfeats(), "parallel engine: ncols mismatch vs serial");
  expect(df_serial.column_names() == df_parallel.column_names(), "parallel engine: column names mismatch");

  const auto s_age = df_serial.column("age").as_series().to_vec<std::int64_t>();
  const auto p_age = df_parallel.column("age").as_series().to_vec<std::int64_t>();
  expect(s_age == p_age, "parallel engine: age column mismatch vs serial");

  const auto s_job = df_serial.column("job").as_note();
  const auto p_job = df_parallel.column("job").as_note();
  expect(s_job.at(0) == p_job.at(0), "parallel engine: first job value mismatch");
  const auto last = df_serial.ncases() - 1;
  expect(s_job.at(last) == p_job.at(last), "parallel engine: last job value mismatch");
}

void test_dataframe_sql_accessor() {
  ReadCsvOptions options;
  options.sep = ';';
  options.ncases = 80;
  options.usefeats = {"age", "job", "balance"};
  const auto df = DataFrame::load_csv(fixture_path("bank-full.csv").string(), options);

  const auto filtered = df.sql().execute(
      "SELECT age, job FROM df WHERE age >= 45 ORDER BY age DESC LIMIT 5");
  expect(filtered.nfeats() == 2, "sql().execute: projected feature count mismatch");
  expect(filtered.has_feature("age") && filtered.has_feature("job"),
         "sql().execute: projection should keep requested features");
  expect(filtered.ncases() <= 5, "sql().execute: LIMIT should cap row count");
  const auto ages = filtered.features("age").as_series().to_vec<std::int64_t>();
  for (std::size_t i = 1; i < ages.size(); ++i) {
    expect(ages[i - 1] >= ages[i], "sql().execute: ORDER BY age DESC mismatch");
  }

  const auto aliased = df.sql().execute(
      "SELECT age AS age_years FROM df LIMIT 3");
  expect(aliased.ncases() == 3, "sql().execute: LIMIT should apply to aliased projections");
  expect(aliased.nfeats() == 1 && aliased.has_feature("age_years"),
         "sql().execute: SELECT alias should rename output feature");

  const auto with_case = df.sql().execute(
      "SELECT case, age FROM df ORDER BY age ASC LIMIT 4 OFFSET 2");
  expect(with_case.ncases() == 4, "sql().execute: LIMIT/OFFSET row count mismatch");
  expect(with_case.nfeats() == 2 && with_case.has_feature("case") && with_case.has_feature("age"),
         "sql().execute: case projection should materialize as output feature");
  const auto sorted = df.sort_values({DataFrame::ArrangeSpec{"age", true, true}});
  const auto expected = sorted.slice(2, 6).features("age").as_series().to_vec<std::int64_t>();
  const auto actual = with_case.features("age").as_series().to_vec<std::int64_t>();
  expect(expected == actual, "sql().execute: LIMIT/OFFSET should match native slice over sorted data");

  expect_throw([&]() { (void)df.sql().execute("DELETE FROM df"); },
               "sql().execute: non-SELECT statements should throw");

  const auto star_selected = df.sql().execute("SELECT * FROM df LIMIT 2");
  expect(star_selected.ncases() == 2,
         "sql().execute: SQL LIMIT should control result size directly");
  expect(star_selected.nfeats() == df.nfeats(),
         "sql().execute: SELECT * should project all dataframe features");

  const auto aliased_primary = df.sql().execute("SELECT prices.age FROM df AS prices LIMIT 1");
  expect(aliased_primary.ncases() == 1 && aliased_primary.has_feature("prices.age"),
         "sql().execute: FROM alias should rename the calling dataframe source");

  expect_throw([&]() { (void)df.sql().execute("SELECT idx, age FROM df LIMIT 1"); },
               "sql().execute: unknown projection should throw when not a feature/case alias");

  const auto with_case_alias = df.sql(DataFrame::SqlOptions{
      .case_axis_aliases = {"case", "idx"},
  }).execute("SELECT idx, age FROM df ORDER BY age DESC LIMIT 3");
  expect(with_case_alias.nfeats() == 2 && with_case_alias.has_feature("idx") && with_case_alias.has_feature("age"),
         "sql(options).execute: case_axis_aliases should expose case axis via configured alias");
  expect(with_case_alias.ncases() == 3,
         "sql(options).execute: SQL LIMIT should still apply with case alias projections");

  const auto with_named_case_alias = df.sql(DataFrame::SqlOptions{
      .case_axis_aliases = {"row_id"},
  }).execute("SELECT row_id FROM df LIMIT 1");
  expect(with_named_case_alias.nfeats() == 1 && with_named_case_alias.has_feature("row_id"),
         "sql(options).execute: case_axis_aliases should support custom case-axis name");

  DataFrame::SqlOptions param_options;
  param_options.params["min_age"] = std::int64_t{45};
  param_options.params["limit_n"] = std::int64_t{4};
  param_options.params["blocked_job"] = std::string{"unknown"};
  const auto parametrized = df.sql(std::move(param_options)).execute(
      "SELECT age, job FROM df WHERE age >= {min_age} and job != {blocked_job} ORDER BY age DESC LIMIT {limit_n}");
  expect(parametrized.ncases() <= 4 && parametrized.has_feature("age") && parametrized.has_feature("job"),
         "sql(options).execute: params should interpolate scalar placeholders");

  DataFrame::SqlOptions identifier_options;
  identifier_options.identifier_params["col"] = "age";
  identifier_options.params["limit_n"] = std::int64_t{2};
  const auto identifier_selected = df.sql(std::move(identifier_options))
                                       .execute("SELECT {@col} FROM df LIMIT {limit_n}");
  expect(identifier_selected.ncases() == 2 && identifier_selected.has_feature("age"),
         "sql(options).execute: identifier params should bind projected feature names");

  const auto duplicate_projection = "SELECT age as v, balance as v FROM df LIMIT 3";
  expect_throw([&]() {
                (void)df.sql().execute(duplicate_projection);
              },
              "sql().execute: duplicate projection aliases should error by default");
  const auto suffix_dups = df.sql(DataFrame::SqlOptions{
      .collision_policy = DataFrame::SqlOptions::CollisionPolicy::Suffix,
  }).execute(duplicate_projection);
  expect(suffix_dups.has_feature("v") && suffix_dups.has_feature("v_2"),
         "sql(options).execute: CollisionPolicy::Suffix should disambiguate duplicate aliases");

  const auto overwrite_dups = df.sql(DataFrame::SqlOptions{
      .collision_policy = DataFrame::SqlOptions::CollisionPolicy::Overwrite,
  }).execute(duplicate_projection);
  expect(overwrite_dups.nfeats() == 1 && overwrite_dups.has_feature("v"),
         "sql(options).execute: CollisionPolicy::Overwrite should keep one alias");

  const auto metrics = DataFrame({
      {"age", df.features("age")},
      {"score", df.features("balance")},
  });
  const auto joined = df.sql(DataFrame::SqlOptions{
      .sources = {{"m", &metrics}},
  }).execute("SELECT df.age, m.score FROM df JOIN m ON df.age = m.age LIMIT 6");
  expect(joined.nfeats() == 2 && joined.has_feature("df.age") && joined.has_feature("m.score"),
         "sql(options).execute: JOIN should project columns from bound sources");
  expect(joined.ncases() > 0 && joined.ncases() <= 6,
         "sql(options).execute: JOIN result row count should respect LIMIT");

  expect_throw([&]() {
                (void)df.sql().execute("SELECT age FROM missing_source");
              },
              "sql().execute: unknown source should throw");

  expect_throw([&]() {
                DataFrame::SqlOptions bad_params;
                bad_params.params["limit_n"] = std::int64_t{2};
                (void)df.sql(std::move(bad_params)).execute("SELECT age FROM df LIMIT {missing}");
              },
              "sql(options).execute: missing params key should throw");

  expect_throw([&]() {
                DataFrame::SqlOptions strict_params;
                strict_params.reject_unused_params = true;
                strict_params.params["unused"] = std::int64_t{1};
                (void)df.sql(std::move(strict_params)).execute("SELECT age FROM df LIMIT 1");
              },
              "sql(options).execute: reject_unused_params should validate unused bindings");

  const auto prepared = df.prepare_sql(
      "SELECT age FROM df WHERE age >= {min_age} ORDER BY age DESC LIMIT {limit_n}");
  const auto prepared_out = prepared.execute(
      {{"min_age", std::int64_t{40}}, {"limit_n", std::int64_t{3}}});
  expect(prepared_out.ncases() <= 3 && prepared_out.has_feature("age"),
         "prepare_sql().execute(params): prepared query should execute with late-bound params");

  expect_throw([&]() {
                (void)df.sql(DataFrame::SqlOptions{
                    .sources = {{"m", &metrics}},
                }).execute("SELECT m.score FROM m LIMIT 1");
              },
              "sql(options).execute: calling dataframe source must be part of FROM/JOIN");
}

void test_versatile_dataframe_families() {
  ReadCsvOptions options;
  options.sep = ';';
  options.ncases = 40;
  options.usefeats = {"age", "job", "balance", "duration"};
  const auto df = DataFrame::load_csv(fixture_path("bank-full.csv").string(), options);

  const auto age = df.features("age").as_series().to_vec<std::int64_t>();
  auto mask = pluss::Series::zeros({static_cast<std::int64_t>(age.size())}, pluss::codem::ScalarType::Bool);
  auto* keep = reinterpret_cast<bool*>(mask.raw_data());
  std::int64_t expected_rows = 0;
  for (std::size_t i = 0; i < age.size(); ++i) {
    keep[i] = age[i] >= 45;
    expected_rows += keep[i] ? 1 : 0;
  }

  const auto q1 = df.filter(mask);
  expect(q1.ncases() == expected_rows, "filter(mask) should filter rows from boolean mask");

  std::int64_t expected_known = 0;
  const auto jobs = df.features("job").as_note();
  for (std::size_t i = 0; i < age.size(); ++i) {
    if (age[i] >= 45 && std::string(jobs.at(static_cast<std::int64_t>(i))) != "unknown") {
      ++expected_known;
    }
  }
  const auto q3 = df.query("age >= 45 and job != 'unknown'");
  expect(q3.ncases() == expected_known, "query(string): combined numeric/string predicate mismatch");

  const auto q4 = df.query("case >= 5 and case < 10");
  expect(q4.ncases() == 5, "query(string): case-axis filtering should work");

  const auto q5 = df.query("contains(job, 'man')");
  expect(q5.ncases() > 0, "query(string): contains(feature, literal) should match rows");

  const auto iloc_one = df.iloc(0);
  expect(iloc_one.ncases() == 1, "iloc(row) should return exactly one case");
  const auto iloc_span = df.iloc(2, 7);
  expect(iloc_span.ncases() == 5, "iloc(start, stop) should return half-open row slice");
  expect(!df.iat(0, 1).empty(), "iat should return a scalar cell string");
  expect(!df.at(0, "job").empty(), "at should return a scalar cell string by feature name");
  expect(df.get("job").has_value() && !df.get("not_a_feature").has_value(),
         "get(feature) should return optional thread only when feature exists");

  const auto sampled = df.sample(5, 7);
  expect(sampled.ncases() == 5 && sampled.nfeats() == df.nfeats(),
         "sample(n) should return n sampled cases with same feature count");

  const auto sorted_age = df.sort_values({"age"});
  const auto sorted_age_values = sorted_age.features("age").as_series().to_vec<std::int64_t>();
  for (std::size_t i = 1; i < sorted_age_values.size(); ++i) {
    expect(sorted_age_values[i - 1] <= sorted_age_values[i],
           "sort_values(feature) should sort ascending by default");
  }
  const auto sorted_cases = df.sort_cases(false, true);
  expect(sorted_cases.ncases() == df.ncases(), "sort_cases should preserve shape");

  const auto top_age = df.top("age", 3);
  const auto bot_age = df.bottom("age", 3);
  expect(top_age.ncases() == 3 && bot_age.ncases() == 3, "top/bottom should return requested row count");
  const auto top_vals = top_age.features("age").as_series().to_vec<std::int64_t>();
  const auto bot_vals = bot_age.features("age").as_series().to_vec<std::int64_t>();
  expect(top_vals.front() >= top_vals.back(), "top should sort descending by feature");
  expect(bot_vals.front() <= bot_vals.back(), "bottom should sort ascending by feature");

  const auto dup_mask = df.repeated_cases({"job"});
  expect(dup_mask.numel() == df.ncases(), "repeated_cases should emit bool mask over cases");
  const auto unique_job = df.unique_cases({"job"});
  expect(unique_job.ncases() > 0 && unique_job.ncases() <= df.ncases(),
         "unique_cases should retain only first case per key");

  const auto aggregated = df.aggregate({{"cases", "age", DataFrame::SummaryOp::Count}}, false);
  expect(aggregated.ncases() == 1 && aggregated.nfeats() == 1,
         "aggregate should produce one-row summary output");
  const auto grouped_alias = df.group_by(
      {"job"},
      {
          {"cases", "", DataFrame::GroupOp::Count},
      },
      true,
      false);
  expect(grouped_alias.nfeats() == 2 && grouped_alias.has_feature("cases"),
         "group_by should perform grouped aggregations");

  const auto pivot_path = write_temp_csv(
      "pivot_base",
      "state,severity,distance\n"
      "CA,1,10\n"
      "CA,2,20\n"
      "TX,1,30\n"
      "TX,2,40\n");
  const auto pivot_df = DataFrame::load_csv(pivot_path.string());
  const auto pivoted = pivot_df.pivot("state", "severity", "distance", true, false);
  expect(pivoted.ncases() == 2 && pivoted.nfeats() == 2, "pivot should reshape rows x unique feature labels");
  const auto pivot_features = pivoted.feature_names();
  expect(pivot_features.size() == 2, "pivot output should expose two pivoted features");
  expect(pivoted.cases().kind() == ThreadKind::Note, "pivot should set cases to the pivot case feature values");
  const auto pivot_cases = pivoted.cases().as_note();
  std::int64_t row_ca = -1;
  std::int64_t row_tx = -1;
  for (std::int64_t i = 0; i < pivoted.ncases(); ++i) {
    const auto key = std::string(pivot_cases.at(i));
    if (key == "CA") {
      row_ca = i;
    } else if (key == "TX") {
      row_tx = i;
    }
  }
  expect(row_ca >= 0 && row_tx >= 0, "pivot should retain all case keys");
  const auto c0 = pivoted.features(pivot_features[0]).as_series().to_vec<double>();
  const auto c1 = pivoted.features(pivot_features[1]).as_series().to_vec<double>();
  const auto ca0 = c0[static_cast<std::size_t>(row_ca)];
  const auto ca1 = c1[static_cast<std::size_t>(row_ca)];
  const auto tx0 = c0[static_cast<std::size_t>(row_tx)];
  const auto tx1 = c1[static_cast<std::size_t>(row_tx)];
  const bool layout_a =
      (std::fabs(ca0 - 10.0) < 1e-9 && std::fabs(ca1 - 20.0) < 1e-9 && std::fabs(tx0 - 30.0) < 1e-9 &&
       std::fabs(tx1 - 40.0) < 1e-9);
  const bool layout_b =
      (std::fabs(ca0 - 20.0) < 1e-9 && std::fabs(ca1 - 10.0) < 1e-9 && std::fabs(tx0 - 40.0) < 1e-9 &&
       std::fabs(tx1 - 30.0) < 1e-9);
  expect(layout_a || layout_b, "pivot should place values into correct case/feature cells");

  const auto pivoted_mean = pivot_df.pivot_table("state", "severity", "distance", DataFrame::GroupOp::Mean, true, false);
  expect(pivoted_mean.ncases() == 2 && pivoted_mean.nfeats() == 2,
         "pivot_table(mean) should reshape with aggregated values");

  const auto pivot_dup_path = write_temp_csv(
      "pivot_dup",
      "state,severity,distance\n"
      "CA,1,10\n"
      "CA,1,12\n");
  const auto pivot_dup = DataFrame::load_csv(pivot_dup_path.string());
  expect_throw([&]() { (void)pivot_dup.pivot("state", "severity", "distance", true, false); },
               "pivot should throw on duplicate case/feature pairs");
  const auto pivot_dup_mean =
      pivot_dup.pivot_table("state", "severity", "distance", DataFrame::GroupOp::Mean, true, false);
  expect(pivot_dup_mean.ncases() == 1 && pivot_dup_mean.nfeats() == 1, "pivot_table should aggregate duplicate pairs");
  const auto d = pivot_dup_mean.features(pivot_dup_mean.feature_names().front()).as_series().to_vec<double>();
  expect(std::fabs(d[0] - 11.0) < 1e-9, "pivot_table(mean) should aggregate duplicates correctly");

  const auto case_df = df.features({"job", "age"}).set_cases("job", true);
  expect(case_df.nfeats() == 1 && case_df.feature_names()[0] == "age",
         "set_cases(drop_feature=true) should move feature to case axis");
  const auto case_reset = case_df.reset_cases("job_case");
  expect(case_reset.has_feature("job_case"), "reset_cases should restore previous case axis as feature");
  const auto re_aligned = case_reset.align_cases(case_reset.cases());
  expect(re_aligned.ncases() == case_reset.ncases(), "align_cases should reorder by provided case axis");

  const auto jleft = df.features({"age", "job"}).head(6, false);
  const auto jright = df.features({"age", "balance"}).head(6, false);
  const auto joined = jleft.join(jright, "_r");
  expect(joined.has_feature("age_r") && joined.has_feature("balance"),
         "join should append right features and resolve collisions");
  const auto patched = jleft.update(jright, true);
  expect(patched.has_feature("age") && patched.has_feature("job"),
         "update should preserve left schema and update overlapping features");

  const auto renamed_bulk = jleft.rename_features({{"age", "age_years"}, {"job", "role"}});
  expect(renamed_bulk.has_feature("age_years") && renamed_bulk.has_feature("role"),
         "rename_features should apply batch feature renames");
  const auto relabeled = jleft.relabel_features({"feat_age", "feat_job"});
  expect(relabeled.feature_names()[0] == "feat_age" && relabeled.feature_names()[1] == "feat_job",
         "relabel_features should set full feature axis names");
  const auto set_axis_alias = jleft.set_feature_axis({"a0", "a1"});
  expect(set_axis_alias.feature_names()[0] == "a0" && set_axis_alias.feature_names()[1] == "a1",
         "set_feature_axis should alias relabel_features");

  const auto reordered = jleft.reorder({"job", "age"});
  const auto conformed = jleft.conform_to(reordered);
  expect(conformed.feature_names() == reordered.feature_names(),
         "conform_to should reorder features to match target frame");
  expect(conformed.cases().size() == reordered.cases().size(),
         "conform_to should align case axis to target frame");
  const auto reindexed_like = jleft.reindex_like(reordered);
  expect(reindexed_like.feature_names() == reordered.feature_names(),
         "reindex_like should alias conform_to");
  const auto reindexed_cases = jleft.reindex_cases(jleft.cases());
  expect(reindexed_cases.ncases() == jleft.ncases(),
         "reindex_cases should alias align_cases");

  const auto sleft = df.features({"job", "age"}).head(12, false);
  const auto sright = df.features({"job", "duration"}).head(12, false);
  const auto stitched = sleft.stitch_on("job", sright, "job", "_rhs");
  expect(stitched.ncases() > 0 && stitched.has_feature("duration"),
         "stitch_on should inner-match on key and append right payload features");
  const auto sleft_multi = df.features({"job", "age", "duration"}).head(20, false);
  const auto sright_multi = df.features({"job", "age", "balance"}).head(20, false);
  const auto stitched_multi = sleft_multi.stitch_on({"job", "age"}, sright_multi, {"job", "age"}, "_rhs");
  expect(stitched_multi.ncases() > 0 && stitched_multi.has_feature("balance"),
         "stitch_on multi-key should inner-match on all key features");

  const auto f0 = df.head(5, false);
  const auto f1 = df.tail(5, false);
  auto depth_series = pluss::Series::empty({2}, pluss::codem::ScalarType::Int64);
  auto* depth_ptr = reinterpret_cast<std::int64_t*>(depth_series.raw_data());
  depth_ptr[0] = 0;
  depth_ptr[1] = 1;
  const auto atlas = pluss::table::Atlas({f0, f1}, pluss::table::Thread(depth_series));
  expect(atlas.depth() == 2 && atlas.ncases() == 5, "Atlas should track depth and frame shape");
  const auto atlas_head = atlas.head(1);
  const auto atlas_tail = atlas.tail(1);
  expect(atlas_head.depth() == 1 && atlas_tail.depth() == 1, "Atlas head/tail should clip depth");
  const auto atlas_loc = atlas.loc(static_cast<std::int64_t>(1));
  expect(atlas_loc.ncases() == f1.ncases(), "Atlas loc(label) should retrieve labeled frame");
  const auto atlas_sorted = atlas.sort_depth(false, true);
  expect(atlas_sorted.depth() == atlas.depth(), "Atlas sort_depth should preserve depth");
  auto relabel_series = pluss::Series::empty({2}, pluss::codem::ScalarType::Int64);
  auto* relabel_ptr = reinterpret_cast<std::int64_t*>(relabel_series.raw_data());
  relabel_ptr[0] = 10;
  relabel_ptr[1] = 11;
  const auto atlas_relabeled = atlas.relabel_depth(pluss::table::Thread(relabel_series));
  expect(atlas_relabeled.loc(static_cast<std::int64_t>(10)).ncases() == f0.ncases(),
         "Atlas relabel_depth should replace depth labels");
  auto align_series = pluss::Series::empty({2}, pluss::codem::ScalarType::Int64);
  auto* align_ptr = reinterpret_cast<std::int64_t*>(align_series.raw_data());
  align_ptr[0] = 1;
  align_ptr[1] = 0;
  const auto atlas_aligned = atlas.align_depth(pluss::table::Thread(align_series));
  expect(atlas_aligned.iloc(0).ncases() == f1.ncases(), "Atlas align_depth should reorder by requested labels");
  expect(!atlas.empty() && atlas.frames().size() == static_cast<std::size_t>(atlas.depth()),
         "Atlas should expose empty()/frames() with consistent depth");
  const auto atlas_empty = pluss::table::Atlas();
  expect(atlas_empty.empty(), "default Atlas should be empty");
  const auto atlas_roundtrip = pluss::kindle::Archive::kindelize(atlas).tabelize();
  expect(atlas_roundtrip.depth() == atlas.depth() && atlas_roundtrip.ncases() == atlas.ncases(),
         "Archive::kindelize(atlas).tabelize() should reconstruct atlas shape");

  std::int64_t expected_between = 0;
  std::int64_t expected_in = 0;
  std::int64_t expected_not_in = 0;
  std::int64_t expected_feat_cmp = 0;
  const auto duration = df.features("duration").as_series().to_vec<std::int64_t>();
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

  expect(df.query("age between 45 and 50").ncases() == expected_between,
         "query(string): between should filter inclusive bounds");
  expect(df.query("job in ('management', 'technician')").ncases() == expected_in,
         "query(string): in should filter matching literals");
  expect(df.query("job not in ('management', 'technician')").ncases() == expected_not_in,
         "query(string): not in should filter complement set");
  expect(df.query("age < duration").ncases() == expected_feat_cmp,
         "query(string): feature-to-feature comparison mismatch");

  const auto tmp_nulls = write_temp_csv(
      "query_nulls",
      "id,name\n"
      "1,alice\n"
      "2,\n"
      "3,bob\n");
  const auto df_nulls = DataFrame::load_csv(tmp_nulls.string());
  expect(df_nulls.query("isnull(name)").ncases() == 1, "query(string): isnull(feature) should filter null rows");
  expect(df_nulls.query("name notnull").ncases() == 2, "query(string): postfix notnull should filter non-null rows");

  const auto tmp_backtick = write_temp_csv(
      "query_backtick",
      "feat one,feat_two\n"
      "1,x\n"
      "2,y\n");
  const auto df_backtick = DataFrame::load_csv(tmp_backtick.string());
  expect(df_backtick.query("`feat one` >= 2").ncases() == 1,
         "query(string): backtick identifiers should support spaced feature names");

  const auto with_copy = df.assign({{"age_copy", df.features("age")}});
  expect(with_copy.nfeats() == df.nfeats() + 1, "derive(assignments) should append a new feature");
  expect(with_copy.has_feature("age_copy"), "derive(assignments) missing derived feature");

  const auto with_flag = df.assign({{
      "is_senior",
      [&](const DataFrame& frame) {
        const auto ages = frame.features("age").as_series().to_vec<std::int64_t>();
        auto flag = pluss::Series::zeros({static_cast<std::int64_t>(ages.size())}, pluss::codem::ScalarType::Bool);
        auto* p = reinterpret_cast<bool*>(flag.raw_data());
        for (std::size_t i = 0; i < ages.size(); ++i) {
          p[i] = ages[i] >= 50;
        }
        return pluss::table::Thread(flag);
      },
  }});
  expect(with_flag.has_feature("is_senior"), "derive(specs) missing computed feature");
  expect(with_flag.features("is_senior").kind() == ThreadKind::Series, "derive(specs) should produce Series-backed flag");

  const auto with_alias = df.assign({{"age_alias", df.features("age")}});
  expect(with_alias.has_feature("age_alias"), "assign alias should delegate to derive");

  const auto transformed_all = df.transform([](const pluss::table::Thread& feature) {
    if (feature.kind() == ThreadKind::Series) {
      return feature + 1.0;
    }
    return feature;
  });
  expect(transformed_all.ncases() == df.ncases() && transformed_all.nfeats() == df.nfeats(),
         "transform(all) should preserve frame shape");
  expect(transformed_all.features("age").as_series().to_vec<std::int64_t>()[0] == age[0] + 1,
         "transform(all) should transform Series-backed features");
  expect(std::string(transformed_all.features("job").as_note().at(0)) == std::string(df.features("job").as_note().at(0)),
         "transform(all) should allow passthrough for Note-backed features");

  const auto transformed_age = df.transform("age", [](const pluss::table::Thread& feature) {
    return feature * 2.0;
  });
  expect(transformed_age.features("age").as_series().to_vec<std::int64_t>()[0] == age[0] * 2,
         "transform(feature) should update target feature");
  expect(std::string(transformed_age.features("job").as_note().at(0)) == std::string(df.features("job").as_note().at(0)),
         "transform(feature) should keep non-target features unchanged");

  const auto transformed_specs = df.transform({
      {"age", [](const pluss::table::Thread& feature) { return feature + 3.0; }},
      {"duration", [](const pluss::table::Thread& feature) { return feature - 2.0; }},
  });
  expect(transformed_specs.features("age").as_series().to_vec<std::int64_t>()[0] == age[0] + 3,
         "transform(specs) should apply per-feature transform");
  expect(transformed_specs.features("duration").as_series().to_vec<std::int64_t>()[0] == duration[0] - 2,
         "transform(specs) should apply second transform");
  expect_throw([&]() {
                (void)df.transform("age", [](const pluss::table::Thread& feature) { return feature.head(2); });
              },
              "transform should reject transformed feature length mismatches");

  const auto selected_vec = df.features({"age", "job"});
  expect(selected_vec.nfeats() == 2 && selected_vec.has_feature("age") && selected_vec.has_feature("job"),
         "features({...}) should project requested features");
  const auto dropped_vec = df.drop({"balance"});
  expect(!dropped_vec.has_feature("balance") && dropped_vec.nfeats() == df.nfeats() - 1,
         "drop({...}) should remove requested features");

  const auto renamed_map = selected_vec.rename_features({{"age", "age_new"}, {"job", "job_new"}});
  expect(renamed_map.has_feature("age_new") && renamed_map.has_feature("job_new"),
         "rename_features(...) should rename mapped features");

  const auto sorted_single = df.sort_values({"age"}, false);
  const auto sorted_single_age = sorted_single.features("age").as_series().to_vec<std::int64_t>();
  expect(sorted_single_age.size() >= 2 && sorted_single_age[0] >= sorted_single_age[1],
         "sort_values(feature, descending) should sort as requested");

  const auto sorted_multi = df.sort_values({
      DataFrame::ArrangeSpec{"job", true, true},
      DataFrame::ArrangeSpec{"age", false, true},
  });
  expect(sorted_multi.ncases() == df.ncases() && sorted_multi.nfeats() == df.nfeats(),
         "sort_values(ArrangeSpec[]) should preserve shape");

  const auto masked = df.filter(mask);
  expect(masked.ncases() == q1.ncases(), "filter(mask) should select matching rows");

  const auto grouped_frame = df.group_by(
      {"job"},
      {DataFrame::GroupSpec{"cases", "", DataFrame::GroupOp::Count}},
      true,
      false);
  expect(grouped_frame.has_feature("job") && grouped_frame.has_feature("cases"),
         "group_by(keys,specs) should return grouped aggregate DataFrame");

  const auto applied = df.transform("age", [](const pluss::table::Thread& feature) { return feature + 5.0; });
  expect(applied.features("age").as_series().to_vec<std::int64_t>()[0] == age[0] + 5,
         "transform(feature, fn) should transform selected feature");
  auto applied_inplace = df;
  applied_inplace = applied_inplace.transform("age", [](const pluss::table::Thread& feature) { return feature + 4.0; });
  expect(applied_inplace.features("age").as_series().to_vec<std::int64_t>()[0] == age[0] + 4,
         "transform(feature, fn) should support assignment-back update");

  const auto merged = jleft.stitch_on("age", jright, "age", "_m");
  expect(merged.ncases() > 0 && merged.has_feature("balance"),
         "stitch_on should join on key and include right payload features");

  const auto text_path = write_temp_csv(
      "df_text_wrappers",
      "txt,txt2\n"
      "heavy RAIN on road,rain on road\n"
      "clear sky and sun,clear sky\n"
      "fog near junction,fog near school\n");
  const auto text_df = DataFrame::load_csv(text_path.string());
  const auto text_catalog = pluss::kindle::Catalog::kindelize(text_df);
  const auto text_clean = text_catalog.text_preprocess("txt").tabelize();
  expect(text_clean.ncases() == text_df.ncases(), "text_preprocess should preserve case count");
  const auto keywords = text_catalog.extract_keywords("txt", 3).tabelize();
  expect(keywords.has_feature("keyword") && keywords.has_feature("count"),
         "extract_keywords should return keyword/count features");
  const auto sim_df = text_catalog.compute_similarity("txt", "txt2").tabelize();
  expect(sim_df.ncases() == text_df.ncases(), "compute_similarity should emit one score per case");
  const auto filtered_text = text_catalog.filter_by_text("txt", "rain").tabelize();
  expect(filtered_text.ncases() == 1, "filter_by_text should keep matching text rows");
  const auto enriched_text = text_catalog.add_text_features("txt").tabelize();
  expect(enriched_text.has_feature("txt_char_len") && enriched_text.has_feature("txt_word_count"),
         "add_text_features should append generated text features");
  const auto text_stats = text_catalog.aggregate_text_stats("txt", "count").tabelize();
  expect(text_stats.ncases() == 1 && text_stats.has_feature("value"),
         "aggregate_text_stats should emit one-row stat table");
  const auto text_tfidf = text_catalog.tfidf("txt").tabelize();
  expect(text_tfidf.has_feature("txt_tfidf_norm"), "tfidf should append tfidf norm feature");
  const auto from_catalog = pluss::kindle::Catalog::kindelize(df.head(5, false)).tabelize();
  expect(from_catalog.ncases() == 5, "Catalog::kindelize(df).tabelize() should reconstruct frame rows");

  const auto summary = df.aggregate(
      {
          {"cases", "age", DataFrame::SummaryOp::Count},
          {"age_mean", "age", DataFrame::SummaryOp::Mean},
          {"jobs", "job", DataFrame::SummaryOp::Nunique},
      },
      false);
  expect(summary.ncases() == 1, "summarize should produce one summary row");
  expect(summary.nfeats() == 3, "summarize should emit requested output features");
  expect(summary.features("cases").as_series().to_vec<std::int64_t>(0) == df.ncases(),
         "summarize count should match frame row count");
  expect(summary.features("jobs").as_series().to_vec<std::int64_t>(0) > 0,
         "summarize nunique should be positive for job feature");

  const auto text_only = df.note_features();
  expect(text_only.nfeats() == 1 && text_only.has_feature("job"),
         "note_features() should retain only Note-backed features");

  const auto numeric_only = df.series_features();
  expect(numeric_only.nfeats() == 3 && numeric_only.has_feature("age")
             && numeric_only.has_feature("balance") && numeric_only.has_feature("duration"),
         "series_features() should retain Series-backed features");

  const auto age_thread = df.features("age");
  const auto mean_scalar = age_thread.mean();
  expect(mean_scalar.defined(), "mean() should return a defined scalar Series");
  expect(mean_scalar.numel() == 1, "mean() should return scalar-sized Series");
  const auto mean_vals = mean_scalar.to_vec<double>();
  expect(mean_vals.size() == 1, "mean() scalar payload should contain a single value");
  const auto logged = mean_scalar.log();
  expect(logged.defined(), "log() over reduction scalar should stay defined");
  const auto expd = logged.exp();
  expect(expd.defined(), "exp() over reduction scalar should stay defined");

  const auto age_plus_2 = age_thread + 2.0;
  const auto age_minus_2 = age_thread - 2.0;
  const auto age_times_2 = age_thread * 2.0;
  const auto age_div_2 = age_thread / 2.0;
  const auto age_mod_2 = age_thread % 2.0;
  expect(age_plus_2.kind() == ThreadKind::Series && age_minus_2.kind() == ThreadKind::Series,
         "Thread arithmetic operators should return Series-backed Thread");
  expect(age_times_2.kind() == ThreadKind::Series && age_div_2.kind() == ThreadKind::Series
             && age_mod_2.kind() == ThreadKind::Series,
         "Thread arithmetic operators should remain in Thread domain");

  const auto age_gt = age_thread > 40.0;
  const auto age_le = age_thread <= 50.0;
  const auto age_eq_self = age_thread == age_thread;
  expect(age_gt.kind() == ThreadKind::Series && age_gt.as_series().scalar_type() == pluss::codem::ScalarType::Bool,
         "Thread comparison operators should return bool Series-backed Thread");
  expect(age_le.kind() == ThreadKind::Series && age_eq_self.kind() == ThreadKind::Series,
         "Thread comparison operators should produce Thread outputs");

  auto df_assign = df;
  const auto age_before = df_assign.features("age").as_series().to_vec<std::int64_t>();
  df_assign.with_column_("age", df_assign.features("age") + 7.0);
  const auto age_after = df_assign.features("age").as_series().to_vec<std::int64_t>();
  expect(age_after.size() == age_before.size(), "feature assignment should preserve case count");
  expect(age_after[0] == age_before[0] + 7, "with_column_(\"age\", feature(\"age\") + 7) should update in place");

  df_assign.with_column_("age_plus_1", df_assign.features("age") + 1.0);
  expect(df_assign.has_feature("age_plus_1"),
         "with_column_(\"new\", feature(\"curr\") + 1) should create a new feature");
  const auto age_plus = df_assign.features("age_plus_1").as_series().to_vec<std::int64_t>();
  expect(age_plus[0] == age_after[0] + 1, "new feature assignment should store RHS values");

  auto df_bulk = df;
  const auto age_before_bulk = df_bulk.features("age").as_series().to_vec<std::int64_t>();
  const auto dur_before_bulk = df_bulk.features("duration").as_series().to_vec<std::int64_t>();
  df_bulk.with_column_("age", df_bulk.features("age") + 2.0);
  df_bulk.with_column_("duration", df_bulk.features("duration") + 3.0);
  const auto age_after_bulk = df_bulk.features("age").as_series().to_vec<std::int64_t>();
  const auto dur_after_bulk = df_bulk.features("duration").as_series().to_vec<std::int64_t>();
  expect(age_after_bulk[0] == age_before_bulk[0] + 2 && dur_after_bulk[0] == dur_before_bulk[0] + 3,
         "with_column_ should update multiple existing features in place");

  df_bulk.with_column_("age_copy2", df_bulk.features("age"));
  df_bulk.with_column_("dur_copy2", df_bulk.features("duration"));
  expect(df_bulk.has_feature("age_copy2") && df_bulk.has_feature("dur_copy2"),
         "with_column_ should create missing features");

  bool threw_note_add = false;
  try {
    [[maybe_unused]] auto bad = df_assign.features("job") + 1.0;
  } catch (const std::exception&) {
    threw_note_add = true;
  }
  expect(threw_note_add, "Thread + scalar should reject non-numeric payloads (Note)");

  const auto note_cmp = df_assign.features("job") == df_assign.features("job");
  expect(note_cmp.kind() == ThreadKind::Series
             && note_cmp.as_series().scalar_type() == pluss::codem::ScalarType::Bool,
         "Thread Note comparison should delegate to Note and return bool Series-backed Thread");

  const auto copied = df_bulk.features({"age", "duration"});
  df_bulk.with_column_("age_copy3", copied.features("age"));
  df_bulk.with_column_("dur_copy3", copied.features("duration"));
  expect(df_bulk.has_feature("age_copy3") && df_bulk.has_feature("dur_copy3"),
         "feature({..}) should return DataFrame for multi-feature copy workflows");

  const auto temporal_only = df.timeline_features();
  expect(temporal_only.nfeats() == 0, "timeline_features() should be empty when no temporal features exist");

  expect(df.features("age").as_series().numel() == df.ncases(), "as_series should expose Series payload");
  expect(df.features("job").as_note().numel() == df.ncases(), "as_note should expose Note payload");

  const auto cast_age_to_f64 = df.astype({"age"}, pluss::codem::ScalarType::Float64);
  expect(cast_age_to_f64.features("age").as_series().scalar_type() == pluss::codem::ScalarType::Float64,
         "astype(features, dtype) should cast selected Series features");

  const auto age_ge = df.assign({{
      "age_ge_45",
      [](const DataFrame& frame) {
        return pluss::table::Thread(frame.features("age").as_series().to(pluss::codem::ScalarType::Float64).ge(45.0));
      },
  }});
  expect(age_ge.features("age_ge_45").kind() == ThreadKind::Series
             && age_ge.features("age_ge_45").as_series().scalar_type() == pluss::codem::ScalarType::Bool,
         "derive should add a bool Series comparison feature");

  const auto age_in = df.assign({{
      "age_in_set",
      [](const DataFrame& frame) {
        const auto ages = frame.features("age").as_series().to_vec<std::int64_t>();
        auto out = pluss::Series::zeros({static_cast<std::int64_t>(ages.size())}, pluss::codem::ScalarType::Bool);
        auto* p = reinterpret_cast<bool*>(out.raw_data());
        for (std::size_t i = 0; i < ages.size(); ++i) {
          p[i] = (ages[i] == 30 || ages[i] == 31);
        }
        return pluss::table::Thread(out);
      },
  }});
  expect(age_in.features("age_in_set").kind() == ThreadKind::Series
             && age_in.features("age_in_set").as_series().scalar_type() == pluss::codem::ScalarType::Bool,
         "derive should add a bool Series membership feature");

  const auto tmp_time = write_temp_csv(
      "time_feature_helpers",
      "start,end\n"
      "2020-01-01 00:00:00,2020-01-01 01:30:00\n"
      "2020-01-02 08:00:00,2020-01-02 09:00:00\n");
  const auto tdf = DataFrame::load_csv(tmp_time.string())
                       .cast_timeline({"start", "end"})
                       .assign({
                           {"Start_Year", [](const DataFrame& frame) {
                              return pluss::table::Thread(frame.features("start").as_timeline().year());
                            }},
                           {"Start_Hour", [](const DataFrame& frame) {
                              return pluss::table::Thread(frame.features("start").as_timeline().hour());
                            }},
                           {"Duration_min", [](const DataFrame& frame) {
                              auto end_ns = frame.features("end")
                                                .cast(pluss::table::ThreadCastTarget::Series, pluss::codem::ScalarType::Int64)
                                                .as_series()
                                                .to(pluss::codem::ScalarType::Float64);
                              auto start_ns = frame.features("start")
                                                  .cast(pluss::table::ThreadCastTarget::Series, pluss::codem::ScalarType::Int64)
                                                  .as_series()
                                                  .to(pluss::codem::ScalarType::Float64);
                              return pluss::table::Thread(end_ns.sub(start_ns).div(6.0e10));
                            }},
                           {"Is_RushHour", [](const DataFrame& frame) {
                              const auto h = frame.features("Start_Hour").as_series().to_vec<std::int64_t>();
                              auto out = pluss::Series::zeros({static_cast<std::int64_t>(h.size())}, pluss::codem::ScalarType::Bool);
                              auto* p = reinterpret_cast<bool*>(out.raw_data());
                              for (std::size_t i = 0; i < h.size(); ++i) {
                                const auto hh = h[i];
                                p[i] = ((hh >= 7 && hh <= 9) || (hh >= 16 && hh <= 18));
                              }
                              return pluss::table::Thread(out);
                            }},
                       });

  expect(tdf.timeline_features().nfeats() == 2,
         "timeline_features() should retain Timeline features after cast_timeline");
  expect(tdf.features("start").as_timeline().numel() == 2, "as_timeline should expose Timeline payload");
  const auto tdf_timeindex = tdf.cast_timeindex({"start"});
  expect(tdf_timeindex.timeline_features().nfeats() == 2 && tdf_timeindex.has_feature("start")
             && tdf_timeindex.has_feature("end"),
         "timeline_features() should retain Timeline/TimeIndex-backed features");
  const auto shifted_tdf = tdf.shift_time({"start"}, 1, "D");
  expect(shifted_tdf.ncases() == tdf.ncases() && shifted_tdf.has_feature("start"),
         "shift_time should preserve shape and update selected temporal feature");
  const auto floored_tdf = tdf.floor_time({"start"}, "D");
  expect(floored_tdf.ncases() == tdf.ncases(), "floor_time should preserve shape");
  const auto localized_tdf = tdf.tz_localize({"start"}, "UTC");
  const auto utc_tdf = localized_tdf.to_utc({"start"});
  expect(utc_tdf.ncases() == tdf.ncases(), "tz_localize/to_utc should preserve shape");
  const auto daily_mean = tdf.resample("start", {"Duration_min"}, "D", "mean", 1, false);
  expect(daily_mean.ncases() == 2 && daily_mean.has_feature("Duration_min"),
         "resample should aggregate values onto time bins");

  const auto years = tdf.features("Start_Year").as_series().to_vec<std::int64_t>();
  expect(years.size() == 2 && years[0] == 2020 && years[1] == 2020,
         "derive should expose Timeline year extraction");

  const auto mins = tdf.features("Duration_min").as_series().to_vec<double>();
  expect(mins.size() == 2 && std::fabs(mins[0] - 90.0) < 1e-6 && std::fabs(mins[1] - 60.0) < 1e-6,
         "derive should compute End_Time - Start_Time in minutes");

  const auto rush = tdf.features("Is_RushHour").as_series().to_vec<bool>();
  expect(rush.size() == 2 && !rush[0] && rush[1],
         "derive should mark Start_Hour membership against provided values");

  const auto job_counts = df.count_by("job", 5, true, false);
  expect(job_counts.nfeats() == 2, "count_by should emit key + count features");
  expect(job_counts.feature_names()[0] == "job" && job_counts.feature_names()[1] == "count",
         "count_by feature naming mismatch");
  expect(job_counts.ncases() <= 5, "count_by(top_n) should cap row count");

  const auto job_age_mean = df.mean_by("job", "age", 5, 1, true, false);
  expect(job_age_mean.nfeats() == 3, "mean_by should emit group + mean + count features");
  expect(job_age_mean.feature_names()[0] == "job"
             && job_age_mean.feature_names()[1] == "mean_age"
             && job_age_mean.feature_names()[2] == "count",
         "mean_by feature naming mismatch");
  expect(job_age_mean.ncases() <= 5, "mean_by(top_n) should cap row count");

  const auto name_counts_keep_na = df_nulls.count_by("name", 0, false, false);
  expect(name_counts_keep_na.ncases() == 3, "count_by(dropna=false) should include NA bucket");

  const auto grouped_job = df.group_by(
      {"job"},
      {
          {"cases", "", DataFrame::GroupOp::Count},
          {"age_mean", "age", DataFrame::GroupOp::Mean},
          {"job_nunique", "job", DataFrame::GroupOp::Nunique},
      },
      true,
      false);
  expect(grouped_job.nfeats() == 4, "group(single key) should emit key + requested aggregates");
  expect(grouped_job.has_feature("job") && grouped_job.has_feature("cases")
             && grouped_job.has_feature("age_mean") && grouped_job.has_feature("job_nunique"),
         "group(single key) missing expected output features");

  const auto grouped_multi = df.group_by(
      {"job", "age"},
      {
          {"cases", "", DataFrame::GroupOp::Count},
      },
      true,
      false);
  expect(grouped_multi.nfeats() == 3, "group(multi key) should emit all keys + aggregates");
  expect(grouped_multi.ncases() > 0, "group(multi key) should produce grouped rows");

  const auto left_small = df.features({"age", "job"}).head(6, false);
  const auto right_small = df.features({"age", "duration"}).head(6, false);
  const auto combined_features = left_small.combine(
      right_small,
      {DataFrame::CombineOp::Features, "_r", true});
  expect(combined_features.nfeats() == 4,
         "combine(features) should append right-hand features and resolve collisions");
  expect(combined_features.has_feature("age") && combined_features.has_feature("job")
             && combined_features.has_feature("age_r") && combined_features.has_feature("duration"),
         "combine(features) expected feature names missing");

  const auto bump_age = left_small.features("age").as_series().add(100.0);
  const auto right_update = left_small.with_column("age", pluss::table::Thread(bump_age));
  const auto combined_update = left_small.combine(
      right_update,
      {DataFrame::CombineOp::Update, "_r", true});
  const auto updated_age = combined_update.features("age").as_series().to(pluss::codem::ScalarType::Float64).to_vec<double>();
  const auto left_age = left_small.features("age").as_series().to(pluss::codem::ScalarType::Float64).to_vec<double>();
  expect(std::fabs(updated_age[0] - (left_age[0] + 100.0)) < 1e-9,
         "combine(update, overwrite=true) should replace overlapping features");

  const auto combined_update_keep = left_small.combine(
      right_update,
      {DataFrame::CombineOp::Update, "_r", false});
  const auto kept_age = combined_update_keep.features("age").as_series().to(pluss::codem::ScalarType::Float64).to_vec<double>();
  expect(std::fabs(kept_age[0] - left_age[0]) < 1e-9,
         "combine(update, overwrite=false) should keep left feature values");

  const auto agg_out = df.aggregate({{"age_min", "age", DataFrame::SummaryOp::Min}}, false);
  expect(agg_out.ncases() == 1 && agg_out.nfeats() == 1, "aggregate should summarize requested specs");

  auto let_df = df.features({"age", "duration", "balance"});
  let_df.set("age_copy") = let_df.features("age");
  expect(let_df.has_feature("age_copy"), "set(single)=Thread should create/update feature");
  expect(let_df.features("age_copy").as_series().to_vec<std::int64_t>() == let_df.features("age").as_series().to_vec<std::int64_t>(),
         "set(single)=Thread should preserve assigned values");

  let_df.set("age_plus_duration") = let_df.features("age") + let_df.features("duration");
  expect(let_df.has_feature("age_plus_duration"), "set(single)=expression should assign computed Thread");

  let_df.set("age") = let_df.features("duration");
  expect(let_df.features("age").as_series().to_vec<std::int64_t>() ==
             let_df.features("duration").as_series().to_vec<std::int64_t>(),
         "set(single) should allow assigning into an existing feature");

  let_df.set({"age_rhs", "dur_rhs"}) = let_df.features({"age", "duration"});
  expect(let_df.has_feature("age_rhs") && let_df.has_feature("dur_rhs"),
         "set(multi)=DataFrame should assign multiple features");

  let_df.set({"age_rhs2", "dur_rhs2"}) = {let_df.features("age"), let_df.features("duration")};
  expect(let_df.has_feature("age_rhs2") && let_df.has_feature("dur_rhs2"),
         "set(multi)=initializer_list<Thread> should assign multiple features");

  const auto short_age = let_df.head(3, false).features("age");
  expect_throw([&]() { let_df.set("bad_len") = short_age; },
               "set(single) should reject rhs length mismatch");
  expect_throw([&]() {
    let_df.set({"bad_a", "bad_b"}) = {
        let_df.features("age"),
        short_age,
    };
  }, "set(multi) should reject any rhs length mismatch");

  const auto numeric = df.features({"age", "balance", "duration"});
  const auto age0 = numeric.features("age").as_series().to_vec<std::int64_t>(0);

  const auto add_scalar = numeric.add(1.0);
  expect(add_scalar.features("age").as_series().to_vec<std::int64_t>(0) == age0 + 1,
         "add(scalar) should shift numeric Series features");
  const auto add_frame = numeric.add(numeric);
  expect(add_frame.features("age").as_series().to_vec<std::int64_t>(0) == age0 * 2,
         "add(frame) should add feature-wise values");

  const auto sub_alias = numeric.subtract(1.0);
  expect(sub_alias.features("age").as_series().to_vec<std::int64_t>(0) == age0 - 1,
         "subtract alias should delegate to sub");
  const auto mul_alias = numeric.multiply(2.0);
  expect(mul_alias.features("age").as_series().to_vec<std::int64_t>(0) == age0 * 2,
         "multiply alias should delegate to mul");

  const auto reverse_sub = numeric.rsub(100.0);
  expect(reverse_sub.features("age").as_series().to_vec<std::int64_t>(0) == 100 - age0,
         "rsub(scalar) should reverse scalar-column subtraction");

  const auto reverse_add = numeric.radd(10.0);
  const auto reverse_mul = numeric.rmul(3.0);
  const auto reverse_div = numeric.rdiv(120.0);
  const auto reverse_true_div = numeric.rtruediv(120.0);
  const auto reverse_floor_div = numeric.rfloordiv(120.0);
  const auto reverse_mod = numeric.rmod(120.0);
  const auto reverse_pow = numeric.rpow(2.0);
  expect(reverse_add.ncases() == numeric.ncases() && reverse_add.nfeats() == numeric.nfeats(),
         "radd should preserve frame shape");
  expect(reverse_mul.ncases() == numeric.ncases() && reverse_mul.nfeats() == numeric.nfeats(),
         "rmul should preserve frame shape");
  expect(reverse_div.ncases() == numeric.ncases() && reverse_div.nfeats() == numeric.nfeats(),
         "rdiv should preserve frame shape");
  expect(reverse_true_div.ncases() == numeric.ncases() && reverse_true_div.nfeats() == numeric.nfeats(),
         "rtruediv should preserve frame shape");
  expect(reverse_floor_div.ncases() == numeric.ncases() && reverse_floor_div.nfeats() == numeric.nfeats(),
         "rfloordiv should preserve frame shape");
  expect(reverse_mod.ncases() == numeric.ncases() && reverse_mod.nfeats() == numeric.nfeats(),
         "rmod should preserve frame shape");
  expect(reverse_pow.ncases() == numeric.ncases() && reverse_pow.nfeats() == numeric.nfeats(),
         "rpow should preserve frame shape");

  const auto div_alias = numeric.divide(2.0);
  const auto true_div = numeric.truediv(2.0);
  const auto floor_div = numeric.floordiv(2.0);
  const auto mod_alias = numeric.mod(2.0);
  const auto pow_alias = numeric.pow(2.0);
  expect(div_alias.ncases() == numeric.ncases() && div_alias.nfeats() == numeric.nfeats(),
         "divide alias should preserve frame shape");
  expect(true_div.ncases() == numeric.ncases() && true_div.nfeats() == numeric.nfeats(),
         "truediv should preserve frame shape");
  expect(floor_div.ncases() == numeric.ncases() && floor_div.nfeats() == numeric.nfeats(),
         "floordiv should preserve frame shape");
  expect(mod_alias.ncases() == numeric.ncases() && mod_alias.nfeats() == numeric.nfeats(),
         "mod should preserve frame shape");
  expect(pow_alias.ncases() == numeric.ncases() && pow_alias.nfeats() == numeric.nfeats(),
         "pow should preserve frame shape");

  const auto eq_self = numeric.eq(numeric);
  const auto ne_self = numeric.ne(numeric);
  const auto ge_scalar = numeric.ge(45.0);
  const auto lt_scalar = numeric.lt(45.0);
  const auto le_scalar = numeric.le(45.0);
  const auto gt_scalar = numeric.gt(45.0);
  expect(eq_self.features("age").as_series().scalar_type() == pluss::codem::ScalarType::Bool,
         "eq should return bool Series features");
  expect(ne_self.features("age").as_series().scalar_type() == pluss::codem::ScalarType::Bool,
         "ne should return bool Series features");
  expect(lt_scalar.features("age").as_series().scalar_type() == pluss::codem::ScalarType::Bool,
         "lt should return bool Series features");
  expect(le_scalar.features("age").as_series().scalar_type() == pluss::codem::ScalarType::Bool,
         "le should return bool Series features");
  expect(gt_scalar.features("age").as_series().scalar_type() == pluss::codem::ScalarType::Bool,
         "gt should return bool Series features");
  expect(ge_scalar.features("age").as_series().scalar_type() == pluss::codem::ScalarType::Bool,
         "ge should return bool Series features");

  const auto eq_age = eq_self.features("age").as_series().to_vec<bool>();
  const auto ne_age = ne_self.features("age").as_series().to_vec<bool>();
  expect(std::all_of(eq_age.begin(), eq_age.end(), [](bool v) { return v; }),
         "eq(frame) should be true for identical numeric frames");
  expect(std::none_of(ne_age.begin(), ne_age.end(), [](bool v) { return v; }),
         "ne(frame) should be false for identical numeric frames");

  const auto ge_age = ge_scalar.features("age").as_series().to_vec<bool>();
  const auto ge_count = static_cast<std::int64_t>(std::count(ge_age.begin(), ge_age.end(), true));
  expect(ge_count == expected_rows, "ge(scalar) should match manual threshold count");

  const auto cmp_alias = numeric.compare(numeric.add(1.0));
  expect(cmp_alias.features("age").as_series().to_vec<bool>(0),
         "compare alias should delegate to ne for changed values");

  const auto mixed_add = df.add(5.0);
  expect(mixed_add.features("job").kind() == ThreadKind::Note,
         "add(scalar) should preserve non-numeric features");
  expect(std::string(mixed_add.features("job").as_note().at(0)) == std::string(df.features("job").as_note().at(0)),
         "add(scalar) should not alter non-numeric feature values");

  const auto mixed_eq = df.eq(df);
  expect(mixed_eq.features("job").kind() == ThreadKind::Series,
         "eq(frame) should produce bool output for non-numeric features too");
  expect(mixed_eq.features("job").as_series().to_vec<bool>(0),
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

  const auto miss_isnull = miss.isna();
  expect(miss_isnull.nfeats() == miss.nfeats(), "missing(isnull) should retain all features by default");
  expect(miss_isnull.features("score").as_series().scalar_type() == pluss::codem::ScalarType::Bool,
         "missing(isnull) should return bool mask features");
  expect(miss_isnull.features("score").as_series().to_vec<bool>(1),
         "missing(isnull) should mark null score rows");

  const auto miss_notnull = miss.notna();
  expect(!miss_notnull.features("score").as_series().to_vec<bool>(1),
         "missing(notnull) should invert isnull mask");

  const auto miss_isna = miss.isna({"score"});
  const auto miss_notna = miss.notna({"score"});
  expect(miss_isna.nfeats() == 1 && miss_notna.nfeats() == 1,
         "isna/notna should support selected feature subsets");
  expect(miss_isna.features("score").as_series().to_vec<bool>(1),
         "isna should mark null rows");
  expect(!miss_notna.features("score").as_series().to_vec<bool>(1),
         "notna should invert isna mask");

  const auto miss_filled = miss.fillna(0.0, {"score"});
  expect(miss_filled.features("score").as_series().to_vec<std::int64_t>(1) == 0,
         "fillna(value, features) should replace selected numeric nulls");
  expect(std::string(miss_filled.features("label").as_note().at(1)) == std::string(miss.features("label").as_note().at(1)),
         "missing(fill) on selected features should not mutate unselected features");

  const auto miss_note_filled = miss.fillna("missing", {"label"});
  expect(std::string(miss_note_filled.features("label").as_note().at(1)) == "missing",
         "fillna(string, features) should replace null Note values");
  expect(!miss_note_filled.features("label").has_validity_mask(),
         "fillna(string, features) should clear Note validity mask when all nulls are filled");

  auto validity = pluss::Series::zeros({4}, pluss::codem::ScalarType::Bool);
  auto* valid_ptr = reinterpret_cast<bool*>(validity.raw_data());
  valid_ptr[0] = true;
  valid_ptr[1] = false;
  valid_ptr[2] = true;
  valid_ptr[3] = false;
  const std::array<std::int64_t, 1> shape4{4};

  const auto mixed_missing = DataFrame({
      {"label", pluss::table::Thread(pluss::make_note(std::vector<std::string>{"x", "", "z", ""}, shape4), validity)},
      {"ts", pluss::table::Thread(
                 pluss::make_timeline({
                     "2024-01-01T00:00:00Z",
                     "2024-01-02T00:00:00Z",
                     "2024-01-03T00:00:00Z",
                     "2024-01-04T00:00:00Z",
                 }),
                 validity)},
      {"ti", pluss::table::Thread(
                 pluss::make_timeindex({
                     "2024-01-01T00:00:00Z",
                     "2024-01-02T00:00:00Z",
                     "2024-01-03T00:00:00Z",
                     "2024-01-04T00:00:00Z",
                 }),
                 validity)},
  });

  const auto ts_fill = pluss::make_timeline("2024-01-31T00:00:00Z");
  const auto mixed_ts_filled = mixed_missing.fillna(ts_fill, {"ts"});
  const auto ts_ticks = mixed_ts_filled.features("ts").as_timeline().to_epoch_nanos_vector();
  const auto ts_fill_tick = ts_fill.to_epoch_nanos_vector()[0];
  expect(ts_ticks[1] == ts_fill_tick && ts_ticks[3] == ts_fill_tick,
         "fillna(Timeline, features) should replace null Timeline values");
  expect(!mixed_ts_filled.features("ts").has_validity_mask(),
         "fillna(Timeline, features) should clear Timeline validity mask when all nulls are filled");
  expect(mixed_ts_filled.features("ti").has_validity_mask(),
         "fillna(Timeline, features) should not mutate unselected TimeIndex features");

  const auto ti_fill = pluss::make_timeindex("2024-02-01T00:00:00Z", false);
  const auto mixed_ti_filled = mixed_missing.fillna(ti_fill, {"ti"});
  const auto ti_ticks = mixed_ti_filled.features("ti").as_timeindex().timeline().to_epoch_nanos_vector();
  const auto ti_fill_tick = ti_fill.timeline().to_epoch_nanos_vector()[0];
  expect(ti_ticks[1] == ti_fill_tick && ti_ticks[3] == ti_fill_tick,
         "fillna(TimeIndex, features) should replace null TimeIndex values");
  expect(!mixed_ti_filled.features("ti").has_validity_mask(),
         "fillna(TimeIndex, features) should clear TimeIndex validity mask when all nulls are filled");

  const auto miss_ffill = miss.ffill();
  expect(miss_ffill.features("score").as_series().to_vec<std::int64_t>(1) == 10,
         "ffill should propagate previous numeric value");
  expect(std::string(miss_ffill.features("label").as_note().at(1)) == "a",
         "ffill should propagate previous Note value");

  const auto miss_bfill = miss.bfill();
  expect(miss_bfill.features("score").as_series().to_vec<std::int64_t>(1) == 30,
         "bfill should propagate next numeric value");
  expect(miss_bfill.features("score").has_validity_mask(),
         "bfill should preserve trailing null when no backward source exists");
  expect(!miss_bfill.features("score").validity_mask()->to_vec<bool>(3),
         "bfill should keep trailing nulls unresolved");

  const auto miss_drop_any = miss.dropna();
  expect(miss_drop_any.ncases() == 2, "dropna should drop rows with any null by default");

  const auto miss_drop_all = miss.dropna_all({"score", "label"});
  expect(miss_drop_all.ncases() == 3, "missing(drop all) should drop rows only when all selected features are null");

  const auto arranged_age = df.sort_values({"age"});
  const auto arranged_age_values = arranged_age.features("age").as_series().to_vec<std::int64_t>();
  for (std::size_t i = 1; i < arranged_age_values.size(); ++i) {
    expect(arranged_age_values[i - 1] <= arranged_age_values[i],
           "arrange(feature) should sort ascending by default");
  }

  const auto arranged_score = miss.sort_values({"score"});
  const auto arranged_score_ids = arranged_score.features("id").as_series().to_vec<std::int64_t>();
  expect(arranged_score_ids[0] == 1 && arranged_score_ids[1] == 3 && arranged_score_ids[2] == 2 && arranged_score_ids[3] == 4,
         "arrange(score): expected ascending values with nulls last and stable tie order");

  const auto arranged_score_desc_nulls_first = miss.sort_values({{
      "score",
      false,
      false,
  }});
  const auto arranged_score_desc_ids = arranged_score_desc_nulls_first.features("id").as_series().to_vec<std::int64_t>();
  expect(arranged_score_desc_ids[0] == 2 && arranged_score_desc_ids[1] == 4
             && arranged_score_desc_ids[2] == 3 && arranged_score_desc_ids[3] == 1,
         "arrange(spec): descending with nulls first should position null cases first");

  const auto cast_age = df.astype({{"age", pluss::codem::ScalarType::Float64}});
  expect(cast_age.features("age").as_series().scalar_type() == pluss::codem::ScalarType::Float64,
         "astype should cast selected Series feature dtype");

  const auto cast_to_note = df.cast({{
      "age",
      pluss::table::ThreadCastTarget::Note,
      std::nullopt,
  }});
  expect(cast_to_note.features("age").kind() == ThreadKind::Note,
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
  expect(note_cast.features(note_feature).kind() == ThreadKind::Series,
         "cast(spec): Note -> Series should convert payload kind");
  expect(note_cast.features(note_feature).has_validity_mask(),
         "cast(spec): Note -> Series should set validity mask when parse fails");
  expect(!note_cast.features(note_feature).validity_mask()->to_vec<bool>(2),
         "cast(spec): unparsable Note value should become null");

  ReadCsvOptions timeline_opts;
  timeline_opts.ncases = 8;
  timeline_opts.usefeats = {"Updated Date"};
  const auto timeline_df = DataFrame::load_csv(fixture_path("messy_data.csv").string(), timeline_opts);
  const auto timeline_cast = timeline_df.cast({{
      "Updated Date",
      pluss::table::ThreadCastTarget::Note,
      std::nullopt,
  }});
  expect(timeline_cast.features("Updated Date").kind() == ThreadKind::Note,
         "cast(spec): Timeline -> Note should delegate to underlying timeline string formatting");

  const auto rolling_mean = numeric.rolling({{
      "age_roll_mean",
      "age",
      pluss::table::ThreadWindowOp::RollingMean,
      3,
      2,
  }},
                                          false);
  expect(rolling_mean.ncases() == numeric.ncases(), "window(keep_input=false) should preserve case count");
  expect(rolling_mean.nfeats() == 1, "window(keep_input=false) should emit only computed features");
  expect(rolling_mean.has_feature("age_roll_mean"), "window should create requested output feature name");
  expect(rolling_mean.features("age_roll_mean").kind() == ThreadKind::Series,
         "window output should be Series-backed");
  expect(rolling_mean.features("age_roll_mean").as_series().scalar_type() == pluss::codem::ScalarType::Float64,
         "window numeric outputs should be float64");
  expect(rolling_mean.features("age_roll_mean").has_validity_mask(),
         "window with min_periods > 1 should produce a validity mask");
  expect(!rolling_mean.features("age_roll_mean").validity_mask()->to_vec<bool>(0),
         "window first row should be null when min_periods is not met");

  const auto age_i64 = numeric.features("age").as_series().to_vec<std::int64_t>();
  const auto mean_v = rolling_mean.features("age_roll_mean").as_series().to_vec<double>();
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
  expect(rolling_sum_keep_input.nfeats() == numeric.nfeats() + 1,
         "rolling(keep_input=true) should append computed feature");
  expect(rolling_sum_keep_input.has_feature("age_rollsum_3"),
         "rolling default output name should use feature/op/window policy");

  const auto score_roll_max = miss.rolling({{
      "score_rollmax",
      "score",
      pluss::table::ThreadWindowOp::RollingMax,
      2,
      1,
  }},
                                       false);
  const auto score_roll_max_vals = score_roll_max.features("score_rollmax").as_series().to_vec<double>();
  expect(!score_roll_max.features("score_rollmax").has_validity_mask(),
         "rolling max(min_periods=1) should clear mask when every output row is valid");
  expect(std::fabs(score_roll_max_vals[1] - 10.0) < 1e-9,
         "rolling max should ignore null in two-row window");

  const auto score_roll_min_minp2 = miss.rolling({{
      "score_rollmin",
      "score",
      pluss::table::ThreadWindowOp::RollingMin,
      2,
      2,
  }},
                                              false);
  const auto score_roll_min_valid = score_roll_min_minp2.features("score_rollmin").validity_mask()->to_vec<bool>();
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
      {"usefeats_ncases_skipfooter_and_invalid_source", test_usefeats_ncases_skipfooter_and_invalid_source},
      {"display_engine_width_truncation_and_info", test_display_engine_width_truncation_and_info},
      {"html_display_renderers_and_backend_config", test_html_display_renderers_and_backend_config},
      {"atlas_html_display_renderer", test_atlas_html_display_renderer},
      {"describe_numeric_summary", test_describe_numeric_summary},
      {"view_windowing", test_view_windowing},
      {"render_then_stream_suppression_once", test_render_then_stream_suppression_once},
      {"dataframe_titles_mutable_and_immutable", test_dataframe_titles_mutable_and_immutable},
      {"simple_load_config_api_and_auto_sep", test_simple_load_config_api_and_auto_sep},
      {"first_class_loader_entrypoints", test_first_class_loader_entrypoints},
      {"col_names_none_and_save_as_csv", test_col_names_none_and_save_as_csv},
      {"loader_unified_load_result_for_csv", test_loader_unified_load_result_for_csv},
      {"txt_loader_modes", test_txt_loader_modes},
      {"json_loader_entrypoints", test_json_loader_entrypoints},
      {"save_targets_and_save_as_parity", test_save_targets_and_save_as_parity},
      {"parquet_save_and_load_parity", test_parquet_save_and_load_parity},
      {"feature_value_cases_values_and_save_defaults", test_feature_value_cases_values_and_save_defaults},
      {"transpose_shape_and_axes", test_transpose_shape_and_axes},
      {"squeeze_unsqueeze_bridge", test_squeeze_unsqueeze_bridge},
      {"parallel_engine_matches_serial", test_parallel_engine_matches_serial},
      {"dataframe_sql_accessor", test_dataframe_sql_accessor},
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
