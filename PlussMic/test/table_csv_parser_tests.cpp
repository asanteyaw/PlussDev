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
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include <fmt/format.h>

#include "CodeM/core/ScalarType.h"
#include "pluss/pluss.h"
#include "pluss/tils.h"

namespace {

using pluss::loader::BadLinePolicy;
using pluss::loader::ReadCsvOptions;
using pluss::table::Axis;
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
  expect(df.nfeatures() == 6, "comma fixture: ncols mismatch");
  expect(df.get("Business Case ID").has_value(), "comma fixture: missing Business Case ID");
  expect(df.features("Business Case ID").kind() == ThreadKind::Series, "comma fixture: expected numeric Series column");
  expect(df.features("Agency Name").kind() == ThreadKind::Note, "comma fixture: expected text Note column");
  expect(df.features("Updated Date").kind() == ThreadKind::Timeline,
         "comma fixture: Updated Date should parse as Timeline");
  expect(df.features("Updated Time").kind() == ThreadKind::Timeline,
         "comma fixture: Updated Time should parse as Timeline");

  const auto agency_project = df.features("Agency Project ID");
  expect(pluss::table::detail::has_validity_mask(agency_project), "comma fixture: Agency Project ID should carry null mask");

  const auto idx = df.label().values().as_series().to_vec<std::int64_t>();
  expect(!idx.empty(), "comma fixture: cases unexpectedly empty");
  expect(idx.front() == 0, "comma fixture: cases must start at 0");
  expect(idx.back() == 63, "comma fixture: cases last value mismatch");

  pluss::table::display::DisplayOptions show_opts;
  show_opts.mode = pluss::table::display::DisplayMode::Compact;
  show_opts.terminal_width = 120;
  const auto date_only = df.features({"Updated Date"}).head(3, false).to_string(show_opts);
  const auto first_date = std::string(df.features("Updated Date").as_timeline().to_note_date().at(0));
  expect(date_only.find(first_date) != std::string::npos,
         "timeline display should include plain date for date-only values");
  expect(date_only.find("T00:00:00") == std::string::npos,
         "timeline display should omit synthetic midnight time for date-only values");
}

void test_utf8_bom_is_removed_from_first_header() {
  const auto path = write_temp_csv(
      "utf8_bom_header",
      "\xEF\xBB\xBF" "OrgDefinedId,First Name\n"
      "30277231,Jamiat\n");

  for (const auto engine : {pluss::loader::CsvEngine::Serial, pluss::loader::CsvEngine::Parallel}) {
    ReadCsvOptions options;
    options.engine = engine;

    const auto df = DataFrame::load_csv(path.string(), options);
    expect(df.get("OrgDefinedId").has_value(),
           "UTF-8 BOM must not become part of the first CSV header");
    expect(df.features("OrgDefinedId").as_series().item<std::int64_t>() == 30277231,
           "BOM-normalized first column should retain its value");
  }
}

void test_auto_initializer_list_assignment_and_row_sum() {
  auto df = DataFrame({
      {"a", pluss::table::Thread(pluss::make_series(std::vector<double>{1.0, 2.0}))},
      {"b", pluss::table::Thread(pluss::make_series(std::vector<double>{10.0, 20.0}))},
  });
  auto names = {"a", "b"};

  df.set(names) = df.features(names).fillnull(0.0);
  const auto totals = df.features(names).sum(Axis::Feature).as_series().to_vec<double>();

  expect(totals == std::vector<double>({11.0, 22.0}),
         "auto initializer-list selection should support assignment and row-wise sum");
}

void test_dataframe_axis_reductions() {
  using pluss::table::Thread;

  const auto numeric_valid = pluss::make_series(std::vector<bool>{true, true, false, true});
  const auto text_valid = pluss::make_series(std::vector<bool>{true, false, true, true});
  const auto df = DataFrame(
      {
          {"a", Thread(pluss::make_series(std::vector<double>{1.0, 2.0, 99.0, 4.0}), numeric_valid)},
          {"b", pluss::make_series({10.0, 20.0, 30.0, 40.0})},
          {"c", pluss::make_series(std::vector<std::int64_t>{2, 4, 6, 8})},
          {"label", Thread(pluss::make_note({"one", "two", "three", "four"}), text_valid)},
      },
      Thread(pluss::make_note({"r0", "r1", "r2", "r3"})));

  const auto expect_close = [](const std::vector<double>& actual,
                               const std::vector<double>& expected,
                               std::string_view message) {
    if (actual.size() != expected.size()) {
      fail(message);
    }
    for (std::size_t i = 0; i < actual.size(); ++i) {
      if (std::fabs(actual[i] - expected[i]) > 1e-10) {
        fail(fmt::format("{} at {}: expected {}, got {}", message, i, expected[i], actual[i]));
      }
    }
  };

  expect_close(df.sum(Axis::Case).as_series().to_vec<double>(), {7.0, 100.0, 20.0},
               "DataFrame::sum(axis=0) values mismatch");
  expect_close(df.sum(Axis::Feature).as_series().to_vec<double>(), {13.0, 26.0, 36.0, 52.0},
               "DataFrame::sum(axis=1) values mismatch");
  expect_close(df.min(Axis::Case).as_series().to_vec<double>(), {1.0, 10.0, 2.0},
               "DataFrame::min(axis=0) values mismatch");
  expect_close(df.min(Axis::Feature).as_series().to_vec<double>(), {1.0, 2.0, 6.0, 4.0},
               "DataFrame::min(axis=1) values mismatch");
  expect_close(df.max(Axis::Case).as_series().to_vec<double>(), {4.0, 40.0, 8.0},
               "DataFrame::max(axis=0) values mismatch");
  expect_close(df.max(Axis::Feature).as_series().to_vec<double>(), {10.0, 20.0, 30.0, 40.0},
               "DataFrame::max(axis=1) values mismatch");
  expect_close(df.mean(Axis::Case).as_series().to_vec<double>(), {7.0 / 3.0, 25.0, 5.0},
               "DataFrame::mean(axis=0) values mismatch");
  expect_close(df.mean(Axis::Feature).as_series().to_vec<double>(), {13.0 / 3.0, 26.0 / 3.0, 18.0, 52.0 / 3.0},
               "DataFrame::mean(axis=1) values mismatch");
  expect_close(df.median(Axis::Case).as_series().to_vec<double>(), {2.0, 25.0, 5.0},
               "DataFrame::median(axis=0) values mismatch");
  expect_close(df.median(Axis::Feature).as_series().to_vec<double>(), {2.0, 4.0, 18.0, 8.0},
               "DataFrame::median(axis=1) values mismatch");
  expect_close(df.prod(Axis::Case).as_series().to_vec<double>(), {8.0, 240000.0, 384.0},
               "DataFrame::prod(axis=0) values mismatch");
  expect_close(df.prod(Axis::Feature).as_series().to_vec<double>(), {20.0, 160.0, 180.0, 1280.0},
               "DataFrame::prod(axis=1) values mismatch");
  expect_close(df.var(Axis::Case).as_series().to_vec<double>(), {7.0 / 3.0, 500.0 / 3.0, 20.0 / 3.0},
               "DataFrame::var(axis=0) sample variance mismatch");
  expect_close(df.var(Axis::Feature).as_series().to_vec<double>(), {73.0 / 3.0, 292.0 / 3.0, 288.0, 1168.0 / 3.0},
               "DataFrame::var(axis=1) sample variance mismatch");
  expect_close(df.std(Axis::Case).as_series().to_vec<double>(),
               {std::sqrt(7.0 / 3.0), std::sqrt(500.0 / 3.0), std::sqrt(20.0 / 3.0)},
               "DataFrame::std(axis=0) sample standard deviation mismatch");
  expect_close(df.std(Axis::Feature).as_series().to_vec<double>(),
               {std::sqrt(73.0 / 3.0), std::sqrt(292.0 / 3.0), std::sqrt(288.0),
                std::sqrt(1168.0 / 3.0)},
               "DataFrame::std(axis=1) sample standard deviation mismatch");

  expect(df.count(Axis::Case).as_series().to_vec<std::int64_t>()
             == std::vector<std::int64_t>({3, 4, 4, 3}),
         "DataFrame::count(axis=0) should include numeric and nonnumeric features");
  expect(df.count(Axis::Feature).as_series().to_vec<std::int64_t>()
             == std::vector<std::int64_t>({4, 3, 3, 4}),
         "DataFrame::count(axis=1) should count non-missing values across all features");

  const auto sum_strict_columns = df.sum(Axis::Case, false);
  expect(pluss::table::detail::validity_mask(sum_strict_columns)->to_vec<bool>()
             == std::vector<bool>({false, true, true}),
         "DataFrame::sum(axis=0, skipnull=false) should invalidate nullable reductions");
  const auto sum_strict_rows = df.sum(Axis::Feature, false);
  expect(pluss::table::detail::validity_mask(sum_strict_rows)->to_vec<bool>()
             == std::vector<bool>({true, true, false, true}),
         "DataFrame::sum(axis=1, skipnull=false) should invalidate rows containing numeric nulls");

  const auto idxmin_columns = df.csmin();
  const auto idxmax_columns = df.csmax();
  expect(std::string(idxmin_columns.as_note().at(0)) == "r0"
             && std::string(idxmin_columns.as_note().at(1)) == "r0"
             && std::string(idxmin_columns.as_note().at(2)) == "r0",
         "DataFrame::csmin(axis=0) should return row labels");
  expect(std::string(idxmax_columns.as_note().at(0)) == "r3"
             && std::string(idxmax_columns.as_note().at(1)) == "r3"
             && std::string(idxmax_columns.as_note().at(2)) == "r3",
         "DataFrame::csmax(axis=0) should return row labels");
  const auto idxmin_rows = df.csmin(Axis::Feature);
  const auto idxmax_rows = df.csmax(Axis::Feature);
  expect(std::string(idxmin_rows.as_note().at(0)) == "a"
             && std::string(idxmin_rows.as_note().at(1)) == "a"
             && std::string(idxmin_rows.as_note().at(2)) == "c"
             && std::string(idxmin_rows.as_note().at(3)) == "a",
         "DataFrame::csmin(axis=1) should return feature names");
  expect(std::string(idxmax_rows.as_note().at(0)) == "b"
             && std::string(idxmax_rows.as_note().at(1)) == "b"
             && std::string(idxmax_rows.as_note().at(2)) == "b"
             && std::string(idxmax_rows.as_note().at(3)) == "b",
         "DataFrame::csmax(axis=1) should return feature names");
  expect(idxmin_columns.label().has() && idxmin_rows.label().has(),
         "DataFrame index reductions should preserve the unreduced axis as case labels");

  const auto all_missing = DataFrame(
      {{"x", Thread(pluss::make_series({9.0, 9.0}),
                    pluss::make_series(std::vector<bool>{false, false}))}},
      Thread(pluss::make_note({"left", "right"})));
  expect(all_missing.sum(Axis::Case).as_series().to_vec<double>() == std::vector<double>({0.0}),
         "sum of an all-missing feature should use the additive identity");
  expect(all_missing.prod(Axis::Case).as_series().to_vec<double>() == std::vector<double>({1.0}),
         "product of an all-missing feature should use the multiplicative identity");
  expect(!pluss::table::detail::validity_mask(all_missing.min(Axis::Case))->to_vec<bool>(0),
         "minimum of an all-missing feature should be missing");
  expect(!pluss::table::detail::validity_mask(all_missing.csmin())->to_vec<bool>(0),
         "idxmin of an all-missing feature should be missing");

  const auto bools = DataFrame(
      {
          {"p", pluss::make_series(std::vector<bool>{true, false, true})},
          {"q", Thread(pluss::make_series(std::vector<bool>{true, false, true}),
                       pluss::make_series(std::vector<bool>{true, false, true}))},
      },
      Thread(pluss::make_note({"u", "v", "w"})));
  expect(bools.any().as_series().to_vec<bool>() == std::vector<bool>({true, true}),
         "DataFrame::any() should default to axis=0");
  expect(bools.all().as_series().to_vec<bool>() == std::vector<bool>({false, true}),
         "DataFrame::all() should default to axis=0 and skip missing values");
  expect(bools.any(Axis::Feature).as_series().to_vec<bool>() == std::vector<bool>({true, false, true}),
         "DataFrame::any(axis=1) values mismatch");
  expect(bools.all(Axis::Feature).as_series().to_vec<bool>() == std::vector<bool>({true, false, true}),
         "DataFrame::all(axis=1) values mismatch");
  expect(pluss::table::detail::validity_mask(bools.any(Axis::Case, false))->to_vec<bool>()
             == std::vector<bool>({true, false}),
         "DataFrame::any(axis=0, skipnull=false) should propagate missing validity");
  expect(pluss::table::detail::validity_mask(bools.all(Axis::Feature, false))->to_vec<bool>()
             == std::vector<bool>({true, false, true}),
         "DataFrame::all(axis=1, skipnull=false) should propagate missing validity");

  expect_throw([&]() { (void)df.mean(static_cast<Axis>(2)); },
               "DataFrame numeric reductions should reject unsupported axes");

  const auto duplicates = df.features({"c"}).duplicated();
  expect(duplicates.size() == df.ncases(), "DataFrame::duplicated should return a case-length Thread");
  const auto duplicate_count = duplicates.sum();
  expect(duplicate_count.kind() == pluss::table::ThreadKind::Series,
         "Thread::sum chained after duplicated should return a scalar ThreadValue");
  expect(duplicate_count.as_series().numel() == 1,
         "Thread reduction scalar payload should contain exactly one value");

  const auto dates = Thread(pluss::make_timeline({
      "2024-02-03T00:00:00Z",
      "2024-01-01T00:00:00Z",
      "2024-03-05T00:00:00Z"}));
  expect(dates.min().as_timeline().numel() == 1,
         "Timeline Thread::min should return a scalar 1D Timeline payload");
  expect(dates.max().as_timeline().numel() == 1,
         "Timeline Thread::max should return a scalar 1D Timeline payload");
}

void test_axis_feature_assignment_alignment_and_shape() {
  using pluss::table::Thread;

  auto grades = DataFrame(
      {
          {"student", Thread(pluss::make_note({"Alice", "Bob", "Carol", "David"}))},
          {"A1", Thread(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0}))},
          {"A2", Thread(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}))},
      },
      Thread(pluss::make_series(std::vector<std::int64_t>{1001, 1002, 1003, 1004})));

  const auto shape = grades.shape();
  expect(shape.first == 4 && shape.second == 3, "DataFrame::shape should return (cases, features)");
  expect(grades.features("A1").ncases() == 4, "Thread::ncases should mirror Thread::size");
  const auto thread_shape = grades.features("A1").shape();
  expect(thread_shape.first == 4 && thread_shape.second == 1,
         "Thread::shape should return (cases, 1)");

  const auto assignment_cols = {"A1", "A2"};
  const auto row_totals = grades.features(assignment_cols).sum(Axis::Feature);
  expect(row_totals.size() == grades.ncases(),
         "DataFrame::sum(Axis::Feature) should reduce row-wise to case length");
  grades.set("Assignments") = row_totals;
  expect(grades.features("Assignments").as_series().to_vec<double>()
             == std::vector<double>({11.0, 22.0, 33.0, 44.0}),
         "row-wise assignment total should align to original cases");

  auto scholarships = Thread(pluss::make_series(std::vector<double>{1500.0, 2500.0, 1000.0}))
                          .label().set(Thread(pluss::make_series(
                              std::vector<std::int64_t>{1003, 1001, 1004})));
  bool scholarship_default_threw = false;
  try {
    grades.set("scholarship") = scholarships;
  } catch (const std::exception&) {
    scholarship_default_threw = true;
  }
  expect(scholarship_default_threw,
         "default assignment should reject partial labeled rhs");
  grades.set("scholarship", {.align = pluss::table::AlignBy::Caselbl}) = scholarships;
  const auto awarded = grades.features("scholarship").as_series().to_vec<double>();
  const auto awarded_valid =
      pluss::table::detail::validity_mask(grades.features("scholarship"))->to_vec<bool>();
  expect(awarded_valid == std::vector<bool>({true, false, true, true}),
         "labeled partial assignment should mark unmatched cases null");
  expect(awarded[0] == 2500.0 && awarded[2] == 1500.0 && awarded[3] == 1000.0,
         "labeled partial assignment should align by case label, not position");

  grades.set("flat") = pluss::make_series(7.0);
  expect(grades.features("flat").as_series().to_vec<double>()
             == std::vector<double>({7.0, 7.0, 7.0, 7.0}),
         "0D Series assignment should broadcast over DataFrame cases");
  bool one_row_default_threw = false;
  try {
    grades.set("one_row_series") = Thread(pluss::make_series(std::vector<double>{7.0}));
  } catch (const std::exception&) {
    one_row_default_threw = true;
  }
  expect(one_row_default_threw,
         "1D one-element Thread assignment should not broadcast or align by default");

  const auto wrong_axis = grades.features(assignment_cols).sum(Axis::Case);
  bool wrong_axis_default_threw = false;
  try {
    grades.set("wrong_axis") = wrong_axis;
  } catch (const std::exception&) {
    wrong_axis_default_threw = true;
  }
  expect(wrong_axis_default_threw,
         "default assignment should reject feature-labeled reduction length mismatch");
  grades.set("wrong_axis", {.align = pluss::table::AlignBy::Caselbl}) = wrong_axis;
  const auto wrong_valid =
      pluss::table::detail::validity_mask(grades.features("wrong_axis"))->to_vec<bool>();
  expect(wrong_axis.size() == 2,
         "DataFrame::sum(Axis::Case) should reduce column-wise to feature length");
  expect(wrong_valid == std::vector<bool>({false, false, false, false}),
         "feature-labeled assignment into case axis should align to no cases and become null");
}

void test_semicolon_fixture_with_case_col() {
  ReadCsvOptions options;
  options.sep = ';';
  options.ncases = 40;
  options.usefeats = {"age", "job", "balance", "y"};
  options.case_col = "job";

  const auto df = DataFrame::load_csv(fixture_path("bank-full.csv").string(), options);

  expect(df.ncases() == 40, "semicolon fixture: nrows mismatch");
  expect(df.nfeatures() == 3, "semicolon fixture: ncols mismatch when case_col is selected");
  expect(df.label().values().kind() == ThreadKind::Note, "semicolon fixture: expected text case axis");
  expect(df.features("age").kind() == ThreadKind::Series, "semicolon fixture: age should parse as Series");

  expect(df.label().values().kind() == ThreadKind::Note,
         "text-backed case axes should remain note-backed and require explicit conversion");

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
  options.na_values.insert({"na", "missing", "not collected", "--"});
  options.on_bad_lines = BadLinePolicy::Skip;

  const auto df = DataFrame::load_csv(path.string(), options);

  expect(df.ncases() == 4, "skip bad lines: expected malformed row to be removed");
  expect(df.nfeatures() == 5, "skip bad lines: ncols mismatch");
  expect(df.features("id").kind() == ThreadKind::Series, "id should parse as Series");
  expect(df.features("flag").kind() == ThreadKind::Series, "flag should parse as bool Series");
  expect(df.features("when").kind() == ThreadKind::Timeline, "when should parse as Timeline");

  const auto flag = df.features("flag").as_series();
  expect(flag.scalar_type() == pluss::codem::ScalarType::Bool, "flag should infer Bool dtype");

  const auto amount = df.features("amount");
  expect(pluss::table::detail::has_validity_mask(amount), "amount should have null mask due missing value");
  const auto amount_values = amount.as_series().to_vec<double>();
  const auto amount_valid = pluss::table::detail::validity_mask(amount)->to_vec<bool>();

  expect(amount_valid.size() == 4, "amount mask length mismatch");
  expect(amount_valid[0] && amount_valid[1] && !amount_valid[2] && amount_valid[3], "amount validity mask values mismatch");
  expect(std::fabs(amount_values[0] - 1234.50) < 1e-9, "amount row0 parse mismatch");
  expect(std::fabs(amount_values[1] - 2000.00) < 1e-9, "amount row1 parse mismatch");
  expect(std::fabs(amount_values[3] - 3500.75) < 1e-9, "amount row3 parse mismatch");

  const auto when = df.features("when");
  expect(pluss::table::detail::has_validity_mask(when), "when should carry null mask for empty datetime");

  const auto text = df.features("text").as_note();
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
  expect(first_two.nfeatures() == 2, "usefeats_idx should project to two features");

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
  expect(html.find("reset-table-view") != std::string::npos, "html display should include reset control");
  expect(html.find("Copy TSV") == std::string::npos, "html display should not expose a redundant copy control");
  expect(html.find("toggle-theme") == std::string::npos, "html display should not expose theme switching");
  expect(html.find("save-png") == std::string::npos,
         "PNG export should live in native viewer chrome, not the HTML document");
  expect(html.find("sortable") != std::string::npos, "html display should mark sortable headers");
  expect(html.find("Edmonton") != std::string::npos, "html display should include cell values");
  expect(html.find("prices - describe") != std::string::npos,
         "html display should surface the resolved view title in the document");
  expect(html.find("PlussMic DataFrame") == std::string::npos,
         "html display should not keep the old hard-coded header title");

  auto inhouse_opts = opts;
  inhouse_opts.html_desktop_table_viewport = true;
  const auto inhouse_html = pluss::table::display::render_html(df, inhouse_opts);
  expect(inhouse_html == html,
         "native WebKit and the in-house renderer should receive identical table HTML");

  const auto info_html = pluss::table::display::render_info_html(df, opts);
  expect(info_html.find("Feature inventory") != std::string::npos, "info html should include feature inventory");
  expect(info_html.find("Null Case Rows") != std::string::npos, "info html should include summary cards");
  expect(info_html.find("pluss::table::DataFrame") != std::string::npos, "info html should include class name");
  expect(info_html.find("prices - describe") != std::string::npos,
         "info html should honor the resolved display title");

  expect(pluss::table::display::default_backend() == pluss::table::display::DisplayBackend::Auto,
         "automatic display policy should prefer PlussWebView while retaining platform fallback");
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
  expect(html.find("atlas-previous") != std::string::npos, "atlas html should include previous-frame navigation");
  expect(html.find("atlas-next") != std::string::npos, "atlas html should include next-frame navigation");
  expect(html.find("atlas-show-all") == std::string::npos, "atlas html should not include the all-frames button");
  expect(html.find("atlas-current-frame") == std::string::npos,
         "atlas html should not duplicate the current frame label in the navigation");
  expect(html.find("weekdays") != std::string::npos, "atlas html should surface depth labels");
  expect(html.find("weekends") != std::string::npos, "atlas html should include all frame contents");
  expect(html.find("atlas-panel active") != std::string::npos, "atlas html should include a visible frame panel");
  expect(html.find("plussmic-table-0") != std::string::npos, "atlas html should embed per-frame tables");
  expect(html.find("Edmonton") != std::string::npos, "atlas html should include the first frame contents");
  expect(html.find("Banff") != std::string::npos, "atlas html should include the second frame contents");
  expect(html.find("table-scroll") != std::string::npos, "atlas html should wrap tables in a clipping scroll container");
  expect(html.find("border-right: 2px solid var(--success);") != std::string::npos,
         "atlas html should color the sticky case divider with the success accent");
}

void test_describe_numeric_summary() {
  ReadCsvOptions options;
  options.sep = ';';
  options.ncases = 30;
  options.usefeats = {"age", "balance", "duration", "y"};

  const auto df = DataFrame::load_csv(fixture_path("bank-full.csv").string(), options);
  const auto desc = df.describe(false);

  expect(desc.ncases() == 8, "describe should emit 8 statistic rows");
  expect(desc.nfeatures() == 4, "describe should include all numeric/bool columns");
  expect(desc.label().values().kind() == ThreadKind::Note, "describe cases should be textual labels");
  expect(desc.features("age").as_series().scalar_type() == pluss::codem::ScalarType::Float64,
         "describe stats should be float64");

  const auto labels = desc.label().values().as_note();
  expect(labels.at(0) == "count", "describe cases label[0] mismatch");
  expect(labels.at(1) == "mean", "describe cases label[1] mismatch");
  expect(labels.at(7) == "max", "describe cases label[7] mismatch");

  const auto age_values = df.features("age").as_series().to_vec<std::int64_t>();
  double sum_age = 0.0;
  std::int64_t min_age = age_values.front();
  std::int64_t max_age = age_values.front();
  for (const auto v : age_values) {
    sum_age += static_cast<double>(v);
    min_age = std::min(min_age, v);
    max_age = std::max(max_age, v);
  }
  const double mean_age = sum_age / static_cast<double>(age_values.size());

  const auto age_stats = desc.features("age").as_series().to_vec<double>();
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
  const auto x1 = v1.features("x").as_series().to_vec<std::int64_t>();
  expect(x1.front() == 5 && x1.back() == 9, "view(5,10) should show rows 5..9");

  const auto v2 = df.view(-5, -1, false);
  expect(v2.ncases() == 4, "view(-5,-1) should follow half-open slice semantics");
  const auto x2 = v2.features("x").as_series().to_vec<std::int64_t>();
  expect(x2.front() == 5 && x2.back() == 8, "view(-5,-1) should exclude the last row");
  expect(v2.equals(df.slice(-5, -1)), "view(-5,-1) should match slice(-5,-1)");

  const auto v3 = df.view(0, 3, false);
  expect(v3.ncases() == 3, "view(0,3) should behave like head(3)");
  const auto x3 = v3.features("x").as_series().to_vec<std::int64_t>();
  expect(x3.front() == 0 && x3.back() == 2, "view(0,3) mismatch");

  const auto v4 = df.view(3, -1, false);
  expect(v4.ncases() == 6, "view(3,-1) should follow half-open slice semantics");
  const auto x4 = v4.features("x").as_series().to_vec<std::int64_t>();
  expect(x4.front() == 3 && x4.back() == 8, "view(3,-1) mismatch");
  expect(v4.equals(df.slice(3, -1)), "view(3,-1) should match slice(3,-1)");
}

void test_dataframe_cast_accessor_surface() {
  const auto raw = DataFrame({
      {"opened_at", pluss::table::Thread(pluss::make_note({"2024-01-01 08:00:00", "bad-date"}))},
      {"closed_at", pluss::table::Thread(pluss::make_note({"2024-01-02 09:30:00", "2024-01-03 10:45:00"}))},
      {"amount_text", pluss::table::Thread(pluss::make_note({"10.5", "oops"}))},
      {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema"}))},
  });

  pluss::table::ThreadCastOptions timeline_options;
  timeline_options.format = "%Y-%m-%d %H:%M:%S";
  timeline_options.invalid = pluss::table::ParseInvalid::Null;
  const auto temporal = raw.features({"opened_at", "closed_at"}).cast(timeline_options).timeline();

  expect(temporal.nfeatures() == 2, "features(...).cast(options).timeline() should preserve the selected feature set");
  expect(temporal.features("opened_at").kind() == ThreadKind::Timeline
             && temporal.features("closed_at").kind() == ThreadKind::Timeline,
         "cast accessor timeline() should cast every bound feature to Timeline");
  expect(pluss::table::detail::has_validity_mask(temporal.features("opened_at"))
             && !pluss::table::detail::validity_mask(temporal.features("opened_at"))->to_vec<bool>(1),
         "cast accessor timeline() should honor invalid=Null on unparseable text");

  pluss::table::ThreadCastOptions series_options;
  series_options.series_dtype = pluss::codem::ScalarType::Float64;
  const auto amounts = raw.features({"amount_text"}).cast(series_options).series();
  expect(amounts.nfeatures() == 1 && amounts.features("amount_text").kind() == ThreadKind::Series,
         "features(...).cast(options).series() should cast the selected Note feature to Series");
  expect(pluss::table::detail::has_validity_mask(amounts.features("amount_text"))
             && !pluss::table::detail::validity_mask(amounts.features("amount_text"))->to_vec<bool>(1),
         "cast accessor series() should preserve parse-failure nulls from Note -> Series conversion");

  const auto note_back = amounts.cast().note();
  expect(note_back.features("amount_text").kind() == ThreadKind::Note,
         "cast().note() should allow no-options roundtrips through the cast accessor");
}

void test_dataframe_equals_and_reflected_logicals() {
  const auto exact = DataFrame(
      {{"x", pluss::table::Thread(pluss::make_series(std::vector<double>{1.0, 2.0}))}},
      pluss::table::Thread(pluss::make_note({"r0", "r1"})));
  const auto rounded = DataFrame(
      {{"x", pluss::table::Thread(pluss::make_series(std::vector<double>{1.0 + 1e-7, 2.0 - 5e-7}))}},
      pluss::table::Thread(pluss::make_note({"r0", "r1"})));

  expect(exact.eq(rounded).features("x").as_series().to_vec<bool>() == std::vector<bool>({false, false}),
         "eq should remain elementwise exact even for tiny floating differences");
  expect((exact == rounded).features("x").as_series().to_vec<bool>() == std::vector<bool>({false, false}),
         "operator==(DataFrame, DataFrame) should delegate to eq(frame)");
  expect(!exact.equals(rounded), "equals(frame) should remain strict whole-frame equality");
  expect(exact.equals(rounded, 1e-6, 1e-6),
         "equals(frame, atol, rtol) should tolerate tiny floating differences");
  expect((exact < 1.5).features("x").as_series().to_vec<bool>() == std::vector<bool>({true, false}),
         "operator<(DataFrame, scalar) should delegate to lt(scalar)");
  expect((1.5 <= exact).features("x").as_series().to_vec<bool>() == std::vector<bool>({false, true}),
         "reflected scalar/DataFrame compare should invert through the dataframe surface");

  const auto feature_threshold = pluss::table::Thread(pluss::make_series(std::vector<double>{1.5}));
  expect((exact >= feature_threshold).features("x").as_series().to_vec<bool>() == std::vector<bool>({false, true}),
         "operator>=(DataFrame, Thread) should delegate to ge(thread) on the default feature axis");
  expect((feature_threshold < exact).features("x").as_series().to_vec<bool>() == std::vector<bool>({false, true}),
         "reflected Thread/DataFrame compare should preserve the dataframe feature-axis contract");

  const auto dates_left = DataFrame(
      {
          {"Date", pluss::table::Thread(pluss::make_timeline({
              "2024-01-01",
              "2024-01-02",
          }))},
          {"Date2", pluss::table::Thread(pluss::make_timeline({
              "2024-02-01",
              "2024-02-03",
          }))},
      },
      pluss::table::Thread(pluss::make_note({"r0", "r1"})));
  const auto dates_right = DataFrame(
      {
          {"StockDate", pluss::table::Thread(pluss::make_timeline({
              "2024-01-01",
              "2024-01-05",
          }))},
          {"RequestDate", pluss::table::Thread(pluss::make_timeline({
              "2024-02-01",
              "2024-02-02",
          }))},
      },
      pluss::table::Thread(pluss::make_note({"r0", "r1"})));
  const auto temporal_cmp = dates_left == dates_right;
  expect(temporal_cmp.features("Date").as_series().to_vec<bool>() == std::vector<bool>({true, false}),
         "operator==(DataFrame, DataFrame) should support Timeline feature comparisons");
  expect(temporal_cmp.features("Date2").as_series().to_vec<bool>() == std::vector<bool>({true, false}),
         "operator==(DataFrame, DataFrame) should compare each Timeline feature positionally");

  const auto left = DataFrame(
                        {{"flag", pluss::table::Thread(pluss::make_series(std::vector<bool>{true, false}))}},
                        pluss::table::Thread(pluss::make_note({"left_0", "left_1"})))
                        .title("lhs");
  const auto right = DataFrame(
                         {{"flag", pluss::table::Thread(pluss::make_series(std::vector<bool>{false, true}))}},
                         pluss::table::Thread(pluss::make_note({"right_0", "right_1"})))
                         .title("rhs");

  const auto reflected_and = left.rand(right);
  const auto reflected_or = left.ror(right);
  const auto reflected_xor = left.rxor(right);

  expect(reflected_and.label().values().as_note().vector() == left.label().values().as_note().vector(),
         "rand(frame) should preserve caller case labels");
  expect(reflected_or.label().values().as_note().vector() == left.label().values().as_note().vector(),
         "ror(frame) should preserve caller case labels");
  expect(reflected_xor.label().values().as_note().vector() == left.label().values().as_note().vector(),
         "rxor(frame) should preserve caller case labels");
  expect(reflected_and.title() == "lhs", "rand(frame) should preserve caller title");
  expect(reflected_or.title() == "lhs", "ror(frame) should preserve caller title");
  expect(reflected_xor.title() == "lhs", "rxor(frame) should preserve caller title");
  expect(reflected_and.equals(left.land(right)), "rand(frame) values should match land(frame)");
  expect(reflected_or.equals(left.lor(right)), "ror(frame) values should match lor(frame)");
  expect(reflected_xor.equals(left.lxor(right)), "rxor(frame) values should match lxor(frame)");
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
    expect(capture.sink.str().empty(), "default display methods should not render to cout");
  }

  {
    std::ostringstream os;
    df.head(2).show(os);
    df.tail(2).show(os);
    df.view(0, 2).show(os);
    df.describe().show(os);
    df.aggregate({{"cases", "x", DataFrame::SummaryOp::Count}}).show(os);
    expect(!os.str().empty(), "explicit show(stream) display methods should render");
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
  expect(df.nfeatures() > 5, "config load: auto separator should parse full semicolon file");
  expect(df.get("age").has_value(), "config load: missing expected column");
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
  expect(df_positional.label().values().kind() == ThreadKind::Note, "DataFrame::load_csv positional case feature mismatch");

  const auto df_positional_short = DataFrame::load_csv(
      fixture_path("bank-full.csv").string(),
      "none",
      static_cast<std::size_t>(6));
  expect(df_positional_short.ncases() == 6, "DataFrame::load_csv short positional ncases mismatch");
  expect(df_positional_short.get("feat_0").has_value(), "DataFrame::load_csv short positional should auto-generate feat names");

  const auto df_list_config = DataFrame::load_csv(
      fixture_path("bank-full.csv").string(),
      {
          {"ncases", 10},
          {"features", {"age", "job"}},
      });
  expect(df_list_config.ncases() == 10, "DataFrame::load_csv config list nrows mismatch");
  expect(df_list_config.nfeatures() == 2, "DataFrame::load_csv config list should project two features");

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
  expect(df.nfeatures() == 2, "col names none: ncols mismatch");
  expect(df.get("feat_0").has_value(), "col names none: expected generated feat_0");
  expect(df.get("feat_1").has_value(), "col names none: expected generated feat_1");

  const auto out_path = write_temp_csv("saved_out", "");
  df.save_csv(
      out_path.string(),
      {
          {"sep", ","},
          {"cases", false},
      });

  const auto back = DataFrame::load_csv(out_path.string());
  expect(back.ncases() == df.ncases(), "save_as csv: roundtrip nrows mismatch");
  expect(back.nfeatures() == df.nfeatures(), "save_as csv: roundtrip ncols mismatch");
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
  expect(whitespace_df.nfeatures() == 4, "txt whitespace: ncols mismatch");
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
  expect(fixed_df.nfeatures() == 3, "txt fixed-width: ncols mismatch");
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
  expect(auto_df.nfeatures() == 3, "txt auto: ncols mismatch");
  expect(auto_df.features("flag").kind() == ThreadKind::Series, "txt auto: bool should infer Series");

  const auto txt_result = pluss::loader::load("txt", auto_path.string(), {{"header", true}});
  expect(txt_result.is_frame(), "loader::load(txt) should return frame");
  expect(txt_result.dataframe().nfeatures() == 3, "loader::load(txt) feature count mismatch");
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
  expect(df.nfeatures() == 4, "json loader: ncols mismatch");
  expect(df.features("id").kind() == ThreadKind::Series, "json loader: id should infer Series");
  expect(df.features("city").kind() == ThreadKind::Note, "json loader: city should infer Note");
  expect(df.features("when").kind() == ThreadKind::Timeline, "json loader: when should infer Timeline");
  expect(pluss::table::detail::has_validity_mask(df.features("score")), "json loader: score should carry validity mask");

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
  expect(txt_back.nfeatures() == df.nfeatures(), "save_txt -> load_txt col count mismatch");

  const auto json_path = write_temp_text_file("save_parity_df", "json", "");
  df.save_json(json_path.string());
  const auto json_back = DataFrame::load_json(json_path.string());
  expect(json_back.ncases() == df.ncases(), "save_json -> load_json row count mismatch");
  expect(json_back.nfeatures() == df.nfeatures(), "save_json -> load_json col count mismatch");

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
  expect(back.nfeatures() == df.nfeatures(), "save_parquet -> load_parquet col count mismatch");
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

  const auto all_features = df.features();
  expect(static_cast<std::int64_t>(all_features.size()) == df.nfeatures(),
         "df.features() should return one name per feature");

  const auto out_default = write_temp_csv("saved_default", "");
  df.save_csv(out_default.string());
  const auto back_default = DataFrame::load_csv(out_default.string());
  expect(back_default.nfeatures() == df.nfeatures(), "save_csv default should not add cases column");
}

void test_temporal_save_csv_preserves_human_readable_dates() {
  DataFrame raw({
      {"date_text", pluss::table::Thread(pluss::make_note({"19990924", "20000105"}))},
      {"amount", pluss::table::Thread(pluss::make_series(std::vector<double>{10.0, 20.0}))},
  });

  DataFrame typed = raw;
  typed.set("date_text") =
      raw.features({"date_text"})
          .cast(pluss::table::ThreadCastOptions{
              .format = "%Y%m%d",
              .invalid = pluss::table::ParseInvalid::Null,
          })
          .timeline();

  const auto out_path = write_temp_csv("saved_timeline_dates", "");
  typed.save_csv(out_path.string());

  std::ifstream in(out_path, std::ios::in | std::ios::binary);
  expect(in.is_open(), "failed to reopen saved timeline csv");
  const std::string saved((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  expect(saved.find("1999-09-24") != std::string::npos
             && saved.find("2000-01-05") != std::string::npos,
         fmt::format("save_csv should persist formatted timeline text; got [{}]", saved));
  expect(saved.find("5395830287000004") == std::string::npos,
         "save_csv should not persist raw timeline epoch nanos");
}

void test_temporal_string_compare_uses_timeline_semantics() {
  const DataFrame typed({
      {"Date", pluss::table::Thread(pluss::make_timeline({
          "1995-12-31",
          "1996-01-01",
          "1996-01-02",
          "1997-06-30",
      }))},
      {"value", pluss::table::Thread(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}))},
  });

  const auto by_string = typed.filter(typed.features("Date") >= "1996-01-01");
  const auto by_timeline = typed.filter(typed.features("Date") >= pluss::make_timeline("1996-01-01"));
  expect(by_string.ncases() == 3,
         "Timeline >= ISO date string should use temporal semantics rather than lexicographic text compare");
  expect(by_string.ncases() == by_timeline.ncases(),
         "Timeline >= ISO date string should match Timeline >= scalar Timeline");

  expect_throw(
      [&]() {
        (void)(typed.features("Date") >= "not-a-date");
      },
      "Timeline compare against invalid string literal should fail instead of silently comparing text");
}

void test_temporal_save_reload_filter_matches_scalar_timeline() {
  DataFrame raw({
      {"Date", pluss::table::Thread(pluss::make_note({
                   "19951231",
                   "19960101",
                   "19960102",
                   "19960201",
                   "19970101",
               }))},
      {"ret", pluss::table::Thread(pluss::make_series(std::vector<double>{0.1, 0.2, 0.3, 0.4, 0.5}))},
  });

  DataFrame typed = raw;
  typed.set("Date") =
      raw.features({"Date"})
          .cast(pluss::table::ThreadCastOptions{
              .format = "%Y%m%d",
              .invalid = pluss::table::ParseInvalid::Null,
          })
          .timeline();

  const auto out_path = write_temp_csv("saved_timeline_filter_compare", "");
  typed.save_csv(out_path.string());

  const auto reloaded = DataFrame::load_csv(out_path.string());
  expect(reloaded.features("Date").kind() == ThreadKind::Timeline,
         "reloaded saved ISO date column should infer back as Timeline");

  const auto by_string = reloaded.filter(reloaded.features("Date") >= "1996-01-01");
  const auto by_timeline = reloaded.filter(reloaded.features("Date") >= pluss::make_timeline("1996-01-01"));
  expect(by_string.ncases() == 4,
         "reloaded Timeline >= ISO date string should retain the expected row count");
  expect(by_string.ncases() == by_timeline.ncases(),
         "reloaded Timeline >= ISO date string should match scalar Timeline filtering");
}

void test_dataframe_lvalue_name_range_surfaces() {
  const auto value_valid = pluss::make_series(std::vector<bool>{true, false, true, true});
  const DataFrame df(
      {
          {"grp", pluss::table::Thread(pluss::make_note({"a", "a", "b", "b"}))},
          {"city", pluss::table::Thread(pluss::make_note({"x", "x", "y", "z"}))},
          {"value", pluss::table::Thread(pluss::make_series(std::vector<double>{1.0, 0.0, 3.0, 4.0}), value_valid)},
          {"other", pluss::table::Thread(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0}))},
      },
      pluss::table::Thread(pluss::make_note({"r0", "r1", "r2", "r3"})));

  const std::vector<std::string> drop_cols{"other"};
  const auto dropped = df.drop(drop_cols);
  expect(dropped.nfeatures() == 3 && !dropped.get("other").has_value(),
         "drop(vector<string>) should accept lvalue feature-name ranges");

  const std::vector<std::string> value_cols{"value"};
  const auto null_mask = df.isnull(value_cols).features("value").as_series().to_vec<bool>();
  expect(null_mask == std::vector<bool>({false, true, false, false}),
         "isnull(vector<string>) should project selected features from an lvalue range");

  const auto filled = df.fillnull(9.0, value_cols);
  expect(filled.features("value").as_series().to_vec<double>() == std::vector<double>({1.0, 9.0, 3.0, 4.0}),
         "fillnull(vector<string>) should accept lvalue feature-name ranges");

  const auto replaced = df.replace(4.0, 7.0, value_cols);
  expect(replaced.features("value").as_series().to_vec<double>() == std::vector<double>({1.0, 0.0, 3.0, 7.0}),
         "replace(vector<string>) should accept lvalue feature-name ranges");

  const auto dropped_null = df.dropnull(value_cols);
  expect(dropped_null.ncases() == 3,
         "dropnull(vector<string>) should accept lvalue feature-name ranges");

  const std::vector<std::string> group_keys{"grp"};
  const auto grouped = df.group().by(group_keys).agg({
      {"sum_value", "value", DataFrame::GroupOp::Sum},
      {"count_value", "value", DataFrame::GroupOp::Count},
  });
  expect(grouped.ncases() == 2 && grouped.nfeatures() == 3,
         "group().by(vector<string>) should accept lvalue key ranges");
  expect(grouped.features("sum_value").as_series().to_vec<double>() == std::vector<double>({1.0, 7.0}),
         "group().by(vector<string>) should preserve grouped aggregation semantics");
}

void test_atlas_and_nexus_lvalue_name_range_surfaces() {
  const auto loan = DataFrame({
      {"id", pluss::table::Thread(pluss::make_series(std::vector<std::int64_t>{1, 2}))},
      {"amount", pluss::table::Thread(pluss::make_series(std::vector<double>{100.0, 200.0}))},
      {"status", pluss::table::Thread(pluss::make_note({"good", "bad"}))},
  });
  const auto account = DataFrame({
      {"id", pluss::table::Thread(pluss::make_series(std::vector<std::int64_t>{1, 2}))},
      {"balance", pluss::table::Thread(pluss::make_series(std::vector<double>{500.0, 300.0}))},
      {"opened", pluss::table::Thread(pluss::make_timeline({"2024-01-01", "2024-01-02"}))},
  });

  const auto atlas = pluss::table::Atlas(
      std::vector<DataFrame>{loan, loan.title("loan_copy")},
      pluss::table::Thread(pluss::make_note({"loan", "loan_copy"})));
  const std::vector<std::string> atlas_features{"id", "amount"};
  const auto atlas_selected = atlas.features(atlas_features);
  expect(atlas_selected.nfeatures() == 2,
         "Atlas::features(vector<string>) should accept lvalue feature-name ranges");
  expect(atlas_selected.frame(0).get("amount").has_value() && !atlas_selected.frame(0).get("status").has_value(),
         "Atlas::features(vector<string>) should project selected features");

  pluss::bundle::nexus::Relationship rel;
  rel.name = "loan_account";
  rel.left_table = "loan";
  rel.right_table = "account";
  rel.keys = {{"id", "id"}};
  rel.cardinality = pluss::bundle::nexus::RelationshipCardinality::OneToOne;

  auto nx = pluss::bundle::Nexus({
                                    {"loan", loan},
                                    {"account", account},
                                })
                .links(rel);

  const std::vector<std::string> table_keys{"loan"};
  const auto nx_selected = nx.select(table_keys);
  expect(nx_selected.ntables() == 1 && nx_selected.has("loan") && !nx_selected.has("account"),
         "Nexus::select(vector<string>) should accept lvalue key ranges");

  const std::vector<std::string> drop_keys{"account"};
  const auto nx_dropped = nx.drop(drop_keys);
  expect(nx_dropped.ntables() == 1 && nx_dropped.has("loan") && !nx_dropped.has("account"),
         "Nexus::drop(vector<string>) should accept lvalue key ranges");

  const std::vector<std::string> key_cols{"id"};
  expect(nx.validate_keys("loan", key_cols),
         "Nexus::validate_keys(vector<string>) should accept lvalue key ranges");
  expect(nx.detect_duplicates("loan", key_cols).ncases() == 0,
         "Nexus::detect_duplicates(vector<string>) should accept lvalue key ranges");
  expect(nx.duplicates("loan", key_cols).ncases() == 0,
         "Nexus::duplicates(vector<string>) should accept lvalue key ranges");

  const std::vector<std::string> pulled_fields{"loan.amount", "account.balance"};
  const auto pulled = nx.pull("loan", pulled_fields);
  expect(pulled.get("amount").has_value() && pulled.get("account_balance").has_value(),
         "Nexus::pull(vector<string>) should accept lvalue field ranges");

  const auto atlas_from_nx = nx.to_atlas(table_keys);
  expect(atlas_from_nx.depth() == 1 && atlas_from_nx.frame(0).get("amount").has_value(),
         "Nexus::to_atlas(vector<string>) should accept lvalue key ranges");

  const std::vector<std::string> picked{"id", "amount", "balance"};
  const std::vector<std::string> dropped_features{"id"};
  const auto queried = nx.from("loan").join("account").pick(picked).drop(dropped_features).collect();
  expect(queried.get("amount").has_value() && queried.get("balance").has_value() && !queried.get("id").has_value(),
         "NexusQuery::pick/drop(vector<string>) should accept lvalue feature-name ranges");
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

  expect(t.ncases() == df.nfeatures(), "transpose should swap row/feature dimensions (rows)");
  expect(t.nfeatures() == df.ncases(), "transpose should swap row/feature dimensions (features)");
  expect(t.label().values().kind() == ThreadKind::Note, "transpose case axis should be Note-backed feature labels");

  const auto tcases = t.label().values().as_note();
  expect(std::string(tcases.at(0)) == "a", "transpose cases[0] should equal first source feature name");
  expect(std::string(tcases.at(2)) == "c", "transpose cases[last] should equal last source feature name");

  expect(t.get("0").has_value(), "transpose should expose source-case labels as output feature names");
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
  expect(atlas.nfeatures() == df.nfeatures(), "unsqueeze should preserve feature count");
  expect(atlas.squeeze().equals(df), "squeeze(unsqueeze(df)) should roundtrip");

  const auto label = pluss::table::Thread(
      pluss::make_note(std::vector<std::string>{"batch_0"}, std::array<std::int64_t, 1>{1}));
  const auto labeled = df.unsqueeze(label);
  expect(labeled.axis0_labels().has_value(), "unsqueeze(depth_labels) should attach labels");
  expect(labeled.squeeze().equals(df), "squeeze should preserve frame contents");

  const auto atlas2 = pluss::table::Atlas({df, df});
  expect_throw([&]() { (void)atlas2.squeeze(); }, "squeeze should throw when depth != 1");
}

void test_parallel_engine_matches_serial_engine() {
  ReadCsvOptions parallel;
  parallel.sep = ';';
  parallel.engine = pluss::loader::CsvEngine::Parallel;
  parallel.ncases = 220;
  parallel.usefeats = {"age", "job", "balance", "y"};
  parallel.parallel_threads = 4;
  parallel.parallel_min_bytes = 1;

  ReadCsvOptions serial = parallel;
  serial.engine = pluss::loader::CsvEngine::Serial;

  const auto path = fixture_path("bank-full.csv").string();
  const auto df_parallel = DataFrame::load_csv(path, parallel);
  const auto df_serial = DataFrame::load_csv(path, serial);

  expect(df_serial.ncases() == df_parallel.ncases(), "serial engine: nrows mismatch vs parallel");
  expect(df_serial.nfeatures() == df_parallel.nfeatures(), "serial engine: ncols mismatch vs parallel");
  expect(df_serial.features() == df_parallel.features(),
         "serial engine: column names mismatch vs parallel");

  const auto p_age = df_parallel.features("age").as_series().to_vec<std::int64_t>();
  const auto s_age = df_serial.features("age").as_series().to_vec<std::int64_t>();
  expect(s_age == p_age, "serial engine: age mismatch vs parallel");

  const auto p_balance = df_parallel.features("balance").as_series().to_vec<std::int64_t>();
  const auto s_balance = df_serial.features("balance").as_series().to_vec<std::int64_t>();
  expect(s_balance == p_balance, "serial engine: balance mismatch vs parallel");

  const auto p_job = df_parallel.features("job").as_note();
  const auto s_job = df_serial.features("job").as_note();
  expect(s_job.at(0) == p_job.at(0), "serial engine: first job mismatch vs parallel");
  const auto last = df_serial.ncases() - 1;
  expect(s_job.at(last) == p_job.at(last), "serial engine: last job mismatch vs parallel");
}

void test_auto_engine_policy_switches_between_parallel_and_serial() {
  const auto path = fixture_path("bank-full.csv").string();

  ReadCsvOptions parallel;
  parallel.sep = ';';
  parallel.engine = pluss::loader::CsvEngine::Parallel;
  parallel.ncases = 180;
  parallel.usefeats = {"age", "job", "balance"};
  parallel.parallel_min_bytes = 1;

  ReadCsvOptions serial = parallel;
  serial.engine = pluss::loader::CsvEngine::Serial;

  ReadCsvOptions auto_parallel = parallel;
  auto_parallel.engine = pluss::loader::CsvEngine::Auto;
  auto_parallel.auto_serial_min_bytes = std::numeric_limits<std::size_t>::max();

  ReadCsvOptions auto_serial = parallel;
  auto_serial.engine = pluss::loader::CsvEngine::Auto;
  auto_serial.auto_serial_min_bytes = 1;

  const auto df_parallel = DataFrame::load_csv(path, parallel);
  const auto df_serial = DataFrame::load_csv(path, serial);
  const auto df_auto_parallel = DataFrame::load_csv(path, auto_parallel);
  const auto df_auto_serial = DataFrame::load_csv(path, auto_serial);

  expect(df_auto_parallel.ncases() == df_parallel.ncases(), "auto(parallel policy): nrows mismatch");
  expect(df_auto_parallel.nfeatures() == df_parallel.nfeatures(), "auto(parallel policy): ncols mismatch");
  expect(df_auto_parallel.features() == df_parallel.features(),
         "auto(parallel policy): column names mismatch");

  expect(df_auto_serial.ncases() == df_serial.ncases(), "auto(serial policy): nrows mismatch");
  expect(df_auto_serial.nfeatures() == df_serial.nfeatures(), "auto(serial policy): ncols mismatch");
  expect(df_auto_serial.features() == df_serial.features(),
         "auto(serial policy): column names mismatch");

  const auto p_age = df_parallel.features("age").as_series().to_vec<std::int64_t>();
  const auto ap_age = df_auto_parallel.features("age").as_series().to_vec<std::int64_t>();
  expect(ap_age == p_age, "auto(parallel policy): age mismatch");

  const auto s_age = df_serial.features("age").as_series().to_vec<std::int64_t>();
  const auto as_age = df_auto_serial.features("age").as_series().to_vec<std::int64_t>();
  expect(as_age == s_age, "auto(serial policy): age mismatch");
}

void test_dataframe_sql_accessor() {
  ReadCsvOptions options;
  options.sep = ';';
  options.ncases = 80;
  options.usefeats = {"age", "job", "balance"};
  const auto df = DataFrame::load_csv(fixture_path("bank-full.csv").string(), options);

  const auto filtered = df.sql().execute(
      "SELECT age, job FROM df WHERE age >= 45 ORDER BY age DESC LIMIT 5");
  expect(filtered.nfeatures() == 2, "sql().execute: projected feature count mismatch");
  expect(filtered.get("age").has_value() && filtered.get("job").has_value(),
         "sql().execute: projection should keep requested features");
  expect(filtered.ncases() <= 5, "sql().execute: LIMIT should cap row count");
  const auto ages = filtered.features("age").as_series().to_vec<std::int64_t>();
  for (std::size_t i = 1; i < ages.size(); ++i) {
    expect(ages[i - 1] >= ages[i], "sql().execute: ORDER BY age DESC mismatch");
  }

  const auto aliased = df.sql().execute(
      "SELECT age AS age_years FROM df LIMIT 3");
  expect(aliased.ncases() == 3, "sql().execute: LIMIT should apply to aliased projections");
  expect(aliased.nfeatures() == 1 && aliased.get("age_years").has_value(),
         "sql().execute: SELECT alias should rename output feature");

  const auto with_case = df.sql().execute(
      "SELECT case, age FROM df ORDER BY age ASC LIMIT 4 OFFSET 2");
  expect(with_case.ncases() == 4, "sql().execute: LIMIT/OFFSET row count mismatch");
  expect(with_case.nfeatures() == 2 && with_case.get("case").has_value() && with_case.get("age").has_value(),
         "sql().execute: case projection should materialize as output feature");
  const auto sorted = df.arrange({DataFrame::ArrangeSpec{"age", true, true}});
  const auto expected = sorted.slice(2, 6).features("age").as_series().to_vec<std::int64_t>();
  const auto actual = with_case.features("age").as_series().to_vec<std::int64_t>();
  expect(expected == actual, "sql().execute: LIMIT/OFFSET should match native slice over sorted data");

  expect_throw([&]() { (void)df.sql().execute("DELETE FROM df"); },
               "sql().execute: non-SELECT statements should throw");

  const auto star_selected = df.sql().execute("SELECT * FROM df LIMIT 2");
  expect(star_selected.ncases() == 2,
         "sql().execute: SQL LIMIT should control result size directly");
  expect(star_selected.nfeatures() == df.nfeatures(),
         "sql().execute: SELECT * should project all dataframe features");

  const auto aliased_primary = df.sql().execute("SELECT prices.age FROM df AS prices LIMIT 1");
  expect(aliased_primary.ncases() == 1 && aliased_primary.get("prices.age").has_value(),
         "sql().execute: FROM alias should rename the calling dataframe source");

  expect_throw([&]() { (void)df.sql().execute("SELECT idx, age FROM df LIMIT 1"); },
               "sql().execute: unknown projection should throw when not a feature/case alias");

  const auto with_case_alias = df.sql(DataFrame::SqlOptions{
      .case_axis_aliases = {"case", "idx"},
  }).execute("SELECT idx, age FROM df ORDER BY age DESC LIMIT 3");
  expect(with_case_alias.nfeatures() == 2 && with_case_alias.get("idx").has_value() && with_case_alias.get("age").has_value(),
         "sql(options).execute: case_axis_aliases should expose case axis via configured alias");
  expect(with_case_alias.ncases() == 3,
         "sql(options).execute: SQL LIMIT should still apply with case alias projections");

  const auto with_named_case_alias = df.sql(DataFrame::SqlOptions{
      .case_axis_aliases = {"row_id"},
  }).execute("SELECT row_id FROM df LIMIT 1");
  expect(with_named_case_alias.nfeatures() == 1 && with_named_case_alias.get("row_id").has_value(),
         "sql(options).execute: case_axis_aliases should support custom case-axis name");

  DataFrame::SqlOptions param_options;
  param_options.params["min_age"] = std::int64_t{45};
  param_options.params["limit_n"] = std::int64_t{4};
  param_options.params["blocked_job"] = std::string{"unknown"};
  const auto parametrized = df.sql(std::move(param_options)).execute(
      "SELECT age, job FROM df WHERE age >= {min_age} and job != {blocked_job} ORDER BY age DESC LIMIT {limit_n}");
  expect(parametrized.ncases() <= 4 && parametrized.get("age").has_value() && parametrized.get("job").has_value(),
         "sql(options).execute: params should interpolate scalar placeholders");

  DataFrame::SqlOptions identifier_options;
  identifier_options.identifier_params["col"] = "age";
  identifier_options.params["limit_n"] = std::int64_t{2};
  const auto identifier_selected = df.sql(std::move(identifier_options))
                                       .execute("SELECT {@col} FROM df LIMIT {limit_n}");
  expect(identifier_selected.ncases() == 2 && identifier_selected.get("age").has_value(),
         "sql(options).execute: identifier params should bind projected feature names");

  const auto duplicate_projection = "SELECT age as v, balance as v FROM df LIMIT 3";
  expect_throw([&]() {
                (void)df.sql().execute(duplicate_projection);
              },
              "sql().execute: duplicate projection aliases should error by default");
  const auto suffix_dups = df.sql(DataFrame::SqlOptions{
      .collision_policy = DataFrame::SqlOptions::CollisionPolicy::Suffix,
  }).execute(duplicate_projection);
  expect(suffix_dups.get("v").has_value() && suffix_dups.get("v_2").has_value(),
         "sql(options).execute: CollisionPolicy::Suffix should disambiguate duplicate aliases");

  const auto overwrite_dups = df.sql(DataFrame::SqlOptions{
      .collision_policy = DataFrame::SqlOptions::CollisionPolicy::Overwrite,
  }).execute(duplicate_projection);
  expect(overwrite_dups.nfeatures() == 1 && overwrite_dups.get("v").has_value(),
         "sql(options).execute: CollisionPolicy::Overwrite should keep one alias");

  const auto metrics = DataFrame({
      {"age", df.features("age")},
      {"score", df.features("balance")},
  });
  const auto joined = df.sql(DataFrame::SqlOptions{
      .sources = {{"m", &metrics}},
  }).execute("SELECT df.age, m.score FROM df JOIN m ON df.age = m.age LIMIT 6");
  expect(joined.nfeatures() == 2 && joined.get("df.age").has_value() && joined.get("m.score").has_value(),
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

  const auto prepared = df.prep_sql(
      "SELECT age FROM df WHERE age >= {min_age} ORDER BY age DESC LIMIT {limit_n}");
  const auto prepared_out = prepared.execute(
      {{"min_age", std::int64_t{40}}, {"limit_n", std::int64_t{3}}});
  expect(prepared_out.ncases() <= 3 && prepared_out.get("age").has_value(),
         "prepare_sql().execute(params): prepared query should execute with late-bound params");

  const auto source_only = df.sql(DataFrame::SqlOptions{
      .sources = {{"m", &metrics}},
  }).execute("SELECT m.score FROM m LIMIT 1");
  expect(source_only.ncases() == 1 && source_only.get("m.score").has_value(),
         "sql(options).execute: real SQL backend should allow querying a bound source directly");

  const auto grouped = df.sql().execute(
      "SELECT job, COUNT(*) AS n, AVG(age) AS mean_age "
      "FROM df "
      "WHERE job != 'unknown' "
      "GROUP BY job "
      "HAVING COUNT(*) >= 1 "
      "ORDER BY n DESC, mean_age DESC "
      "LIMIT 5");
  expect(grouped.nfeatures() == 3 && grouped.get("job").has_value() && grouped.get("n").has_value()
             && grouped.get("mean_age").has_value() && grouped.ncases() > 0,
         "sql().execute: GROUP BY/HAVING aggregates should materialize through the sqlite backend");

  const auto distinct_cte = df.sql().execute(
      "WITH older AS ("
      "  SELECT job "
      "  FROM df "
      "  WHERE age >= 45 AND job != 'unknown'"
      ") "
      "SELECT DISTINCT job FROM older ORDER BY job LIMIT 5");
  expect(distinct_cte.nfeatures() == 1 && distinct_cte.get("job").has_value() && distinct_cte.ncases() > 0,
         "sql().execute: CTEs and DISTINCT should execute through the sqlite backend");

  const auto left_joined = df.sql(DataFrame::SqlOptions{
      .sources = {{"m", &metrics}},
  }).execute(
      "SELECT df.age, m.score "
      "FROM df LEFT JOIN m ON df.age = m.age "
      "ORDER BY df.age DESC "
      "LIMIT 6");
  expect(left_joined.nfeatures() == 2 && left_joined.get("df.age").has_value()
             && left_joined.get("m.score").has_value() && left_joined.ncases() == 6,
         "sql().execute: LEFT JOIN should work through the embedded sqlite backend");

  const auto table_name_shortcut = df.sql().execute("df");
  expect(table_name_shortcut.nfeatures() == df.nfeatures() && table_name_shortcut.ncases() == df.ncases(),
         "sql().execute: bare table-name shorthand should mirror pandas-style read_sql(table_name, ...)");
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
  std::int64_t expected_unknown = 0;
  const auto jobs = df.features("job").as_note();
  for (std::size_t i = 0; i < age.size(); ++i) {
    const auto job = std::string(jobs.at(static_cast<std::int64_t>(i)));
    if (job == "unknown") {
      ++expected_unknown;
    }
    if (age[i] >= 45 && job != "unknown") {
      ++expected_known;
    }
  }
  const auto q2 = df.filter(df.features("job") == "unknown");
  expect(q2.ncases() == expected_unknown,
         "filter(df.features(note_col) == \"literal\") should support direct string literal equality");
  const auto q3 = df.query("age >= 45 and job != 'unknown'");
  expect(q3.ncases() == expected_known, "query(string): combined numeric/string predicate mismatch");

  const auto q4 = df.query("case >= 5 and case < 10");
  expect(q4.ncases() == 5, "query(string): case-axis filtering should work");

  const auto q5 = df.query("contains(job, 'man')");
  expect(q5.ncases() > 0, "query(string): contains(feature, literal) should match rows");

  const DataFrame typed({
      {"name", pluss::table::Thread(pluss::make_note({"Laurel", "Ava", "Laurel"}))},
      {"age2", pluss::table::Thread(pluss::make_series({20, 21, 20}))},
      {"entered", pluss::table::Thread(pluss::make_timeline({
          "2024-01-01T00:00:00Z",
          "2024-01-02T00:00:00Z",
          "2024-01-01T00:00:00Z",
      }))},
      {"idx_col", pluss::table::Thread(pluss::make_timeindex({
          "2024-02-01T00:00:00Z",
          "2024-02-01T00:00:00Z",
          "2024-02-02T00:00:00Z",
      }))},
  });
  expect(typed.filter(typed.features("name") == pluss::make_note("Laurel")).ncases() == 2,
         "filter(Thread == scalar Note) should broadcast the scalar note value");
  expect(typed.filter(typed.features("age2") == pluss::make_series(20)).ncases() == 2,
         "filter(Thread == scalar Series) should broadcast the scalar series value");
  expect(typed.filter(typed.features("entered") == pluss::make_timeline("2024-01-01T00:00:00Z")).ncases() == 2,
         "filter(Thread == scalar Timeline) should broadcast the scalar timeline value");
  expect(typed.filter(typed.features("idx_col") == pluss::make_timeline("2024-02-01T00:00:00Z")).ncases() == 2,
         "filter(TimeIndex Thread == scalar Timeline) should broadcast the scalar timeline value");
  bool one_row_timeindex_payload_threw = false;
  try {
    (void)(typed.features("idx_col") == pluss::make_timeindex("2024-02-01T00:00:00Z"));
  } catch (const std::exception&) {
    one_row_timeindex_payload_threw = true;
  }
  expect(one_row_timeindex_payload_threw,
         "Thread == one-row TimeIndex payload should not broadcast or align by default");
  bool note_one_row_thread_threw = false;
  try {
    (void)(typed.features("name") ==
           pluss::table::Thread(pluss::make_note(std::vector<std::string>{"Laurel"})));
  } catch (const std::exception&) {
    note_one_row_thread_threw = true;
  }
  expect(note_one_row_thread_threw,
         "Thread == one-row Note Thread should not broadcast or align by default");
  bool age_one_row_thread_threw = false;
  try {
    (void)(typed.features("age2") ==
           pluss::table::Thread(pluss::make_series(std::vector<std::int64_t>{20})));
  } catch (const std::exception&) {
    age_one_row_thread_threw = true;
  }
  expect(age_one_row_thread_threw,
         "Thread == one-row Series Thread should not broadcast or align by default");
  bool timeline_one_row_thread_threw = false;
  try {
    (void)(typed.features("entered") ==
           pluss::table::Thread(pluss::make_timeline(std::vector<std::string>{"2024-01-01T00:00:00Z"})));
  } catch (const std::exception&) {
    timeline_one_row_thread_threw = true;
  }
  expect(timeline_one_row_thread_threw,
         "Thread == one-row Timeline Thread should not broadcast or align by default");
  bool timeindex_one_row_thread_threw = false;
  try {
    (void)(typed.features("idx_col") ==
           pluss::table::Thread(pluss::make_timeindex(std::vector<std::string>{"2024-02-01T00:00:00Z"})));
  } catch (const std::exception&) {
    timeindex_one_row_thread_threw = true;
  }
  expect(timeindex_one_row_thread_threw,
         "Thread == one-row TimeIndex Thread should not broadcast or align by default");
  const auto masked_scalar = pluss::table::Thread(
      pluss::make_series(std::vector<std::int64_t>{20}),
      pluss::make_series(std::vector<bool>{false}));
  bool masked_one_row_thread_threw = false;
  try {
    (void)(typed.features("age2") == masked_scalar);
  } catch (const std::exception&) {
    masked_one_row_thread_threw = true;
  }
  expect(masked_one_row_thread_threw,
         "masked one-row Thread comparison should not broadcast or align by default");
  const auto one_case = df.case_study(DataFrame::Case::at(0));
  expect(one_case.ncases() == 1, "case_study(pos(row)) should return exactly one case");
  const auto row_span = df.case_study(DataFrame::Case::slice(2, 7));
  expect(row_span.ncases() == 5, "case_study(pos(start, stop)) should return half-open row slice");
  const auto pos_list = df.case_study(DataFrame::Case::take({4, 1, 3}));
  expect(pos_list.ncases() == 3, "case_study(pos({..})) should keep one row per requested position");
  expect(std::string(pos_list.features("job").as_note().at(1)) == std::string(df.features("job").as_note().at(1)),
         "case_study(pos({..})) should preserve requested positional order");

  const auto one_column = df.case_study(DataFrame::Case::at(0), DataFrame::Feat::name("job"));
  expect(one_column.ncases() == 1 && one_column.nfeatures() == 1,
         "case_study(row, feature(name)) should return a 1x1 frame");

  const auto one_column_by_pos = df.case_study(DataFrame::Case::at(0), DataFrame::Feat::at(1));
  expect(one_column_by_pos.nfeatures() == 1, "case_study(row, feature(pos)) should project one feature");
  auto labeled_source = df.case_study(DataFrame::Case::slice(0, 8));
  auto labeled = labeled_source.label().set(
      pluss::table::Thread(pluss::make_note({"cs0", "cs1", "cs1", "cs3", "cs4", "cs5", "cs6", "cs7"})));
  const auto label_match = labeled.case_study(DataFrame::Case::label("cs1"));
  expect(label_match.ncases() == 2, "case_study(label(...)) should return every matching case label");

  const auto label_list = labeled.case_study(DataFrame::Case::labels({"cs4", "cs1"}),
                                             DataFrame::Feat::names({"job", "age"}));
  expect(label_list.ncases() == 3 && label_list.nfeatures() == 2,
         "case_study(labels({...}), features({...})) should preserve label order and selected features");

  const auto label_window = labeled.case_study(DataFrame::Case::span("cs1", "cs4"));
  expect(label_window.ncases() == 4, "case_study(label_range(...)) should include both label endpoints");
  const auto masked_rows = df.case_study(DataFrame::Case::where(df.features("age") > 30.0),
                                         DataFrame::Feat::names({"age", "job"}));
  expect(masked_rows.nfeatures() == 2, "case_study(mask(...), features({...})) should keep requested feature subset");
  expect(masked_rows.ncases() == df.filter(df.features("age") > 30.0).ncases(),
         "case_study(mask(...)) should match ordinary boolean row filtering");

  const auto feature_span = df.case_study(DataFrame::Case::slice(0, 2), DataFrame::Feat::slice(0, 2));
  expect(feature_span.ncases() == 2 && feature_span.nfeatures() == 2,
         "case_study(..., features(start, stop)) should use half-open positional feature slices");

  const auto feature_window = df.case_study(DataFrame::Case::slice(0, 2), DataFrame::Feat::span("age", "job"));
  expect(feature_window.nfeatures() == 2, "case_study(..., feature_range(...)) should use inclusive feature-name slices");

  expect(df.get("job").has_value() && !df.get("not_a_feature").has_value(),
         "get(feature) should return optional thread only when feature exists");
  const auto sampled = df.sample(5, 7);
  expect(sampled.ncases() == 5 && sampled.nfeatures() == df.nfeatures(),
         "sample(n) should return n sampled cases with same feature count");

  const auto sorted_age = df.arrange({"age"});
  const auto sorted_age_values = sorted_age.features("age").as_series().to_vec<std::int64_t>();
  for (std::size_t i = 1; i < sorted_age_values.size(); ++i) {
    expect(sorted_age_values[i - 1] <= sorted_age_values[i],
           "arrange(feature) should sort ascending by default");
  }
  const auto sorted_cases = df.label().sort(false, true);
  expect(sorted_cases.ncases() == df.ncases(), "label().sort should preserve shape");

  const auto top_age = df.top("age", 3);
  const auto bot_age = df.bottom("age", 3);
  expect(top_age.ncases() == 3 && bot_age.ncases() == 3, "top/bottom should return requested row count");
  const auto top_vals = top_age.features("age").as_series().to_vec<std::int64_t>();
  const auto bot_vals = bot_age.features("age").as_series().to_vec<std::int64_t>();
  expect(top_vals.front() >= top_vals.back(), "top should sort descending by feature");
  expect(bot_vals.front() <= bot_vals.back(), "bottom should sort ascending by feature");
  const auto dup_mask = df.features({"job"}).repeated();
  expect(dup_mask.ncases() == df.ncases(), "repeated(feature) should emit bool mask over cases");
  const auto unique_job = df.features({"job"}).drop_duplicates();
  expect(unique_job.ncases() > 0 && unique_job.ncases() <= df.ncases(),
         "unique_cases should retain only first case per key");

  const auto aggregated = df.aggregate({{"cases", "age", DataFrame::SummaryOp::Count}}, false);
  expect(aggregated.ncases() == 1 && aggregated.nfeatures() == 1,
         "aggregate should produce one-row summary output");
  const auto grouped_alias = df.group({.dropnull = true})
                                 .by({"job"})
                                 .agg({
                                     {"cases", "", DataFrame::GroupOp::Count},
                                 });
  expect(grouped_alias.nfeatures() == 2 && grouped_alias.get("cases").has_value(),
         "group should perform grouped aggregations");
  const auto pivot_path = write_temp_csv(
      "pivot_base",
      "state,severity,distance\n"
      "CA,1,10\n"
      "CA,2,20\n"
      "TX,1,30\n"
      "TX,2,40\n");
  const auto pivot_df = DataFrame::load_csv(pivot_path.string());
  const auto pivoted = pivot_df.pivot("state", "severity", "distance", true, false);
  expect(pivoted.ncases() == 2 && pivoted.nfeatures() == 2, "pivot should reshape rows x unique feature labels");
  const auto pivot_features = pivoted.features();
  expect(pivot_features.size() == 2, "pivot output should expose two pivoted features");
  expect(pivoted.label().values().kind() == ThreadKind::Note, "pivot should set cases to the pivot case feature values");
  const auto pivot_cases = pivoted.label().values().as_note();
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
  expect(pivoted_mean.ncases() == 2 && pivoted_mean.nfeatures() == 2,
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
  expect(pivot_dup_mean.ncases() == 1 && pivot_dup_mean.nfeatures() == 1, "pivot_table should aggregate duplicate pairs");
  const auto d = pivot_dup_mean.features(pivot_dup_mean.features().front()).as_series().to_vec<double>();
  expect(std::fabs(d[0] - 11.0) < 1e-9, "pivot_table(mean) should aggregate duplicates correctly");
  const auto pivot_dup_median =
      pivot_dup.pivot_table("state", "severity", "distance", DataFrame::GroupOp::Median, true, false);
  const auto dm = pivot_dup_median.features(pivot_dup_median.features().front()).as_series().to_vec<double>();
  expect(std::fabs(dm[0] - 11.0) < 1e-9, "pivot_table(median) should aggregate duplicates correctly");
  const auto case_df = df.features({"job", "age"}).label().set("job", true);
  expect(case_df.nfeatures() == 1 && case_df.features()[0] == "age",
         "label().set(drop_feature=true) should move feature to case axis");
  const auto case_reset = case_df.label().reset("job_case");
  expect(case_reset.get("job_case").has_value(), "label().reset should restore previous case axis as feature");
  const auto re_aligned = case_reset.label().align(case_reset.label().values());
  expect(re_aligned.ncases() == case_reset.ncases(), "label().align should conform to provided case labels");
  const auto jleft = df.features({"age", "job"}).head(6, false);
  const auto jright = df.features({"age", "balance"}).head(6, false);
  const auto joined = jleft.join(jright, "_r");
  expect(joined.get("age_r").has_value() && joined.get("balance").has_value(),
         "join should append right features and resolve collisions");
  const auto patched = jleft.update(jright, true);
  expect(patched.get("age").has_value() && patched.get("job").has_value(),
         "update should preserve left schema and update overlapping features");

  const auto renamed_bulk = jleft.rename({{"age", "age_years"}, {"job", "role"}});
  expect(renamed_bulk.get("age_years").has_value() && renamed_bulk.get("role").has_value(),
         "rename_features should apply batch feature renames");
  const auto relabeled = jleft.rename({"feat_age", "feat_job"});
  expect(relabeled.features()[0] == "feat_age" && relabeled.features()[1] == "feat_job",
         "relabel_features should set full feature axis names");
  const auto renamed_all = jleft.rename({"a0", "a1"});
  expect(renamed_all.features()[0] == "a0" && renamed_all.features()[1] == "a1",
         "rename({...}) should replace the full feature axis positionally");

  const auto reordered = jleft.reorder({"job", "age"});
  const auto conformed = jleft.conform(reordered);
  expect(conformed.features() == reordered.features(),
         "conform should reorder features to match target frame");
  expect(conformed.label().values().size() == reordered.label().values().size(),
         "conform should align case axis to target frame");
  const auto reindexed_like = jleft.conform(reordered);
  expect(reindexed_like.features() == reordered.features(),
         "conform should provide frame-shape matching");
  const auto reindexed_cases = jleft.label().align(jleft.label().values());
  expect(reindexed_cases.ncases() == jleft.ncases(),
         "label().align should preserve existing labels when target matches");
  const auto align_base = DataFrame({
      {"A", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
      {"B", pluss::table::Thread(pluss::make_series({100.0, 200.0}))},
  }, pluss::make_note({"r1", "r2"}));
  const auto aligned_labels = align_base.label().align(pluss::table::Thread(pluss::make_note({"r2", "missing", "r1"})));
  expect(aligned_labels.ncases() == 3 &&
             pluss::table::detail::valid_at(aligned_labels.features("A"), 0) &&
             !pluss::table::detail::valid_at(aligned_labels.features("A"), 1) &&
             pluss::table::detail::valid_at(aligned_labels.features("A"), 2),
         "label().align should insert nulls for missing case labels");
  const auto aligned_label_vals = aligned_labels.features("A").as_series().to_vec<double>();
  expect(std::fabs(aligned_label_vals[0] - 20.0) < 1e-9 && std::fabs(aligned_label_vals[2] - 10.0) < 1e-9,
         "label().align should reorder matched case labels");
  const auto align_filled =
      align_base.align(DataFrame::AlignSpec{.label = pluss::table::Thread(pluss::make_note({"r2", "missing"})),
                                            .features = {"B", "C", "A"},
                                            .fill = 0.0});
  expect(align_filled.features()[0] == "B" && align_filled.features()[1] == "C" &&
             align_filled.features()[2] == "A",
         "align should conform feature labels and preserve requested order");
  expect(pluss::table::detail::valid_at(align_filled.features("C"), 0) &&
             pluss::table::detail::valid_at(align_filled.features("C"), 1),
         "align(fill) should fill entirely missing numeric features");
  const auto filled_a = align_filled.features("A").as_series().to_vec<double>();
  expect(std::fabs(filled_a[0] - 20.0) < 1e-9 && std::fabs(filled_a[1]) < 1e-9,
         "align(fill) should fill missing case-label rows for numeric features");
  const auto sleft = df.features({"job", "age"}).head(12, false);
  const auto sright = df.features({"job", "duration"}).head(12, false);
  const auto stitched = sleft.unify("job", sright, "job", "common", "_rhs");
  expect(stitched.ncases() > 0 && stitched.get("duration").has_value(),
         "stitch_on should inner-match on key and append right payload features");
  const auto sleft_multi = df.features({"job", "age", "duration"}).head(20, false);
  const auto sright_multi = df.features({"job", "age", "balance"}).head(20, false);
  const auto stitched_multi = sleft_multi.unify({"job", "age"}, sright_multi, {"job", "age"}, "common", "_rhs");
  expect(stitched_multi.ncases() > 0 && stitched_multi.get("balance").has_value(),
         "stitch_on multi-key should inner-match on all key features");
  const DataFrame unify_left({
      {"OrgDefinedId", pluss::table::Thread(pluss::make_note({"10", "20", "30"}))},
      {"midterm", pluss::table::Thread(pluss::make_series({71, 82, 93}))},
  });
  const DataFrame unify_right({
      {"Username", pluss::table::Thread(pluss::make_note({"20", "30"}))},
      {"Final Points Grade", pluss::table::Thread(pluss::make_series({88, 99}))},
  });
  const auto unified_inner = unify_left.unify(
      "OrgDefinedId",
      unify_right,
      "Username",
      "inner");
  expect(unified_inner.ncases() == 2
             && unified_inner.get("Username").has_value()
             && unified_inner.get("Final Points Grade").has_value(),
         "unify(inner) should key-match rows and keep differently named right keys");

  const auto unified_left = unify_left.unify(
      "OrgDefinedId",
      unify_right,
      "Username",
      "left");
  expect(unified_left.ncases() == 3,
         "unify(left) should preserve unmatched left rows");
  expect(std::string(unified_left.features("OrgDefinedId").as_note().at(0)) == "10"
             && std::string(unified_left.features("OrgDefinedId").as_note().at(1)) == "20"
             && std::string(unified_left.features("OrgDefinedId").as_note().at(2)) == "30",
         "unify(left) should preserve left row order");
  expect(pluss::table::detail::has_validity_mask(unified_left.features("Final Points Grade")),
         "unify(left) should mark unmatched right payload rows as invalid");
  expect(pluss::table::detail::validity_mask(unified_left.features("Final Points Grade"))->to_vec<bool>()
             == std::vector<bool>({false, true, true}),
         "unify(left) should null-mark unmatched right payload rows");
  const DataFrame rel_left_unique({
      {"OrgDefinedId", pluss::table::Thread(pluss::make_note({"10", "20"}))},
      {"midterm", pluss::table::Thread(pluss::make_series({71, 82}))},
  });
  const DataFrame rel_left_dup({
      {"OrgDefinedId", pluss::table::Thread(pluss::make_note({"10", "10", "20"}))},
      {"midterm", pluss::table::Thread(pluss::make_series({71, 72, 82}))},
  });
  const DataFrame rel_right_unique({
      {"OrgDefinedId", pluss::table::Thread(pluss::make_note({"10", "20"}))},
      {"Final Points Grade", pluss::table::Thread(pluss::make_series({88, 99}))},
  });
  const DataFrame rel_right_dup({
      {"OrgDefinedId", pluss::table::Thread(pluss::make_note({"10", "10", "20"}))},
      {"Final Points Grade", pluss::table::Thread(pluss::make_series({88, 89, 99}))},
  });
  const auto default_many_match = rel_left_dup.unify(
      "OrgDefinedId",
      rel_right_dup,
      "",
      "inner");
  expect(default_many_match.ncases() == 5,
         "unify default validate should preserve many-to-many row expansion");
  const auto one_to_one = rel_left_unique.unify(
      "OrgDefinedId",
      rel_right_unique,
      "",
      "inner",
      "_right",
      "one_to_one");
  expect(one_to_one.ncases() == 2,
         "unify(validate='one_to_one') should allow unique keys on both sides");
  const auto one_to_many = rel_left_unique.unify(
      "OrgDefinedId",
      rel_right_dup,
      "",
      "inner",
      "_right",
      "one_to_many");
  expect(one_to_many.ncases() == 3,
         "unify(validate='one_to_many') should allow duplicate matches on the right only");
  const auto many_to_one = rel_left_dup.unify(
      "OrgDefinedId",
      rel_right_unique,
      "",
      "left",
      "_right",
      "many_to_one");
  expect(many_to_one.ncases() == 3,
         "unify(validate='many_to_one') should allow duplicate matches on the left only");
  expect_throw([&]() {
    (void)rel_left_dup.unify("OrgDefinedId", rel_right_unique, "", "inner", "_right", "one_to_one");
  }, "unify(validate='one_to_one') should reject duplicate left keys");
  expect_throw([&]() {
    (void)rel_left_dup.unify("OrgDefinedId", rel_right_unique, "", "inner", "_right", "one_to_many");
  }, "unify(validate='one_to_many') should reject duplicate left keys");
  expect_throw([&]() {
    (void)rel_left_unique.unify("OrgDefinedId", rel_right_dup, "", "inner", "_right", "many_to_one");
  }, "unify(validate='many_to_one') should reject duplicate right keys");
  const auto f0 = df.head(5, false);
  const auto f1 = df.tail(5, false);
  auto depth_series = pluss::Series::empty({2}, pluss::codem::ScalarType::Int64);
  auto* depth_ptr = reinterpret_cast<std::int64_t*>(depth_series.raw_data());
  depth_ptr[0] = 0;
  depth_ptr[1] = 1;
  const auto atlas = pluss::table::Atlas({f0, f1}, pluss::table::Thread(depth_series));
  expect(atlas.depth() == 2 && atlas.ncases() == 5, "Atlas should track depth and frame shape");
  expect(atlas.label().has(), "Atlas::label().has should report explicit depth labels");
  expect(atlas.label().values().as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1}),
         "Atlas::label().values should expose typed depth labels");
  expect(atlas.label().find(pluss::table::Thread(pluss::make_series(std::vector<std::int64_t>{1, 9})))
             .as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({1, -1}),
         "Atlas::label().find should delegate to Thread label indexing");
  expect(atlas.label().unite(pluss::table::Thread(pluss::make_series(std::vector<std::int64_t>{1, 2})))
             .as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1, 2}),
         "Atlas::label().unite should preserve typed numeric depth labels");
  expect(atlas.label().pos(static_cast<double>(1)).as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({1}),
         "Atlas::label().pos should return depth positions through Thread label indexing");
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
  const auto atlas_label_relabeled = atlas.label().set(pluss::table::Thread(relabel_series));
  expect(atlas_label_relabeled.loc(static_cast<std::int64_t>(11)).ncases() == f1.ncases(),
         "Atlas::label().set should replace depth labels");
  auto align_series = pluss::Series::empty({2}, pluss::codem::ScalarType::Int64);
  auto* align_ptr = reinterpret_cast<std::int64_t*>(align_series.raw_data());
  align_ptr[0] = 1;
  align_ptr[1] = 0;
  const auto atlas_aligned = atlas.align_depth(pluss::table::Thread(align_series));
  expect(atlas_aligned.dloc(0).ncases() == f1.ncases(), "Atlas align_depth should reorder by requested labels");
  const auto atlas_label_aligned = atlas.label().align(pluss::table::Thread(align_series));
  expect(atlas_label_aligned.dloc(0).ncases() == f1.ncases() &&
             atlas_label_aligned.label().values().as_series().to_vec<std::int64_t>() ==
                 std::vector<std::int64_t>({1, 0}),
         "Atlas::label().align should reorder frames and carry requested depth labels");
  expect(atlas.label().range().label().values().as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1}),
         "Atlas::label().range should materialize typed range depth labels");
  expect(!atlas.label().reset().label().has(),
         "Atlas::label().reset should clear explicit depth labels");
  expect(!atlas.empty() && atlas.frames().size() == static_cast<std::size_t>(atlas.depth()),
         "Atlas should expose empty()/frames() with consistent depth");
  const auto atlas_empty = pluss::table::Atlas();
  expect(atlas_empty.empty(), "default Atlas should be empty");
  const auto atlas_roundtrip = atlas.kindelize().tabelize();
  expect(atlas_roundtrip.depth() == atlas.depth() && atlas_roundtrip.ncases() == atlas.ncases(),
         "Atlas::kindelize().tabelize() should reconstruct atlas shape");
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
  expect(with_copy.nfeatures() == df.nfeatures() + 1, "assign(assignments) should append a new feature");
  expect(with_copy.get("age_copy").has_value(), "assign(assignments) missing assigned feature");

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
  expect(with_flag.get("is_senior").has_value(), "assign(specs) missing computed feature");
  expect(with_flag.features("is_senior").kind() == ThreadKind::Series, "assign(specs) should produce Series-backed flag");

  const auto with_alias = df.assign({{"age_alias", df.features("age")}});
  expect(with_alias.get("age_alias").has_value(), "assign should accept direct Thread assignments");
  const auto transformed_all = df.transform([](const pluss::table::Thread& feature) {
    if (feature.kind() == ThreadKind::Series) {
      return feature + 1.0;
    }
    return feature;
  });
  expect(transformed_all.ncases() == df.ncases() && transformed_all.nfeatures() == df.nfeatures(),
         "transform(all) should preserve frame shape");
  expect(transformed_all.features("age").as_series().to_vec<std::int64_t>()[0] == age[0] + 1,
         "transform(all) should transform Series-backed features");
  expect(std::string(transformed_all.features("job").as_note().at(0)) == std::string(df.features("job").as_note().at(0)),
         "transform(all) should allow passthrough for Note-backed features");

  auto transformed_age = df;
  transformed_age.set("age") = df.features("age").transform([](const pluss::table::Thread& feature) {
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
                auto bad = df;
                bad.set("age") = df.features("age").head(2);
              },
              "transform should reject transformed feature length mismatches");

  auto transformed_frame = df;
  transformed_frame.set("age_plus_duration") = df.features("age") + df.features("duration");
  expect(transformed_frame.get("age_plus_duration").has_value(),
         "transform(feature, dataframe-fn) should allow deriving a new feature");
  expect(transformed_frame.features("age_plus_duration").as_series().to_vec<std::int64_t>()[0] ==
             age[0] + duration[0],
         "transform(feature, dataframe-fn) should evaluate against the full DataFrame");

  const auto selected_vec = df.features({"age", "job"});
  expect(selected_vec.nfeatures() == 2 && selected_vec.get("age").has_value() && selected_vec.get("job").has_value(),
         "features({...}) should project requested features");
  const std::vector<std::string> selected_names = {"age", "job"};
  const auto selected_lvalue = df.features(selected_names);
  expect(selected_lvalue.nfeatures() == 2 && selected_lvalue.get("age").has_value() && selected_lvalue.get("job").has_value(),
         "features(vector<string>) should project requested features");
  const std::vector<std::string> selected_one_name = {"age"};
  const auto selected_one_lvalue = df.features(selected_one_name);
  expect(selected_one_lvalue.nfeatures() == 1 && selected_one_lvalue.get("age").has_value(),
         "features(single-item vector<string>) should return one-column DataFrame");
  const auto selected_public = df.features({"age", "job"});
  expect(selected_public.nfeatures() == 2 && selected_public.get("age").has_value() && selected_public.get("job").has_value(),
         "select({...}) should project requested features");
  const auto selected_like = df.feataxis().like("dur");
  expect(selected_like.get("duration").has_value() && selected_like.nfeatures() >= 1,
         "select_like(\"dur\") should match substring in feature names");
  const auto selected_like_ci = df.feataxis().like("JOB", false);
  expect(selected_like_ci.get("job").has_value(),
         "select_like(..., case_sensitive=false) should support case-insensitive matching");
  const auto selected_regex = df.feataxis().match("^ba");
  expect(selected_regex.get("balance").has_value(),
         "feataxis().match(\"^ba\") should match regex on feature names");
  const auto selected_none = df.feataxis().like("__definitely_not_a_feature__");
  expect(selected_none.nfeatures() == 0 && selected_none.ncases() == df.ncases(),
         "select_like(no match) should return empty feature projection with preserved cases");
  expect_throw([&]() { (void)df.feataxis().match("[unclosed"); },
               "feataxis().match(invalid pattern) should throw");
  const auto dropped_vec = df.drop({"balance"});
  expect(!dropped_vec.get("balance").has_value() && dropped_vec.nfeatures() == df.nfeatures() - 1,
         "drop({...}) should remove requested features");

  const auto renamed_map = selected_vec.rename({{"age", "age_new"}, {"job", "job_new"}});
  expect(renamed_map.get("age_new").has_value() && renamed_map.get("job_new").has_value(),
         "rename_features(...) should rename mapped features");

  const auto sorted_single = df.arrange({"age"}, false);
  const auto sorted_single_age = sorted_single.features("age").as_series().to_vec<std::int64_t>();
  expect(sorted_single_age.size() >= 2 && sorted_single_age[0] >= sorted_single_age[1],
         "arrange(feature, descending) should sort as requested");

  const auto sorted_multi = df.arrange({
      DataFrame::ArrangeSpec{"job", true, true},
      DataFrame::ArrangeSpec{"age", false, true},
  });
  expect(sorted_multi.ncases() == df.ncases() && sorted_multi.nfeatures() == df.nfeatures(),
         "arrange(ArrangeSpec[]) should preserve shape");

  const auto masked = df.filter(mask);
  expect(masked.ncases() == q1.ncases(), "filter(mask) should select matching rows");

  const auto grouped_frame = df.group({.dropnull = true})
                                 .by({"job"})
                                 .agg({DataFrame::GroupSpec{"cases", "", DataFrame::GroupOp::Count}});
  expect(grouped_frame.get("job").has_value() && grouped_frame.get("cases").has_value(),
         "group(...).by(...).agg(...) should return grouped aggregate DataFrame");

  auto applied = df;
  applied.set("age") = df.features("age") + 5.0;
  expect(applied.features("age").as_series().to_vec<std::int64_t>()[0] == age[0] + 5,
         "transform(feature, fn) should transform selected feature");
  auto applied_inplace = df;
  applied_inplace.set("age") = applied_inplace.features("age") + 4.0;
  expect(applied_inplace.features("age").as_series().to_vec<std::int64_t>()[0] == age[0] + 4,
         "transform(feature, fn) should support assignment-back update");

  const auto merged = jleft.unify("age", jright, "age", "common", "_m");
  expect(merged.ncases() > 0 && merged.get("balance").has_value(),
         "stitch_on should join on key and include right payload features");

  const auto text_path = write_temp_csv(
      "df_text_wrappers",
      "txt,txt2\n"
      "heavy RAIN on road,rain on road\n"
      "clear sky and sun,clear sky\n"
      "fog near junction,fog near school\n");
  const auto text_df = DataFrame::load_csv(text_path.string());
  const auto text_catalog = text_df.kindelize();
  const auto text_clean = text_catalog.clean("txt").tabelize();
  expect(text_clean.ncases() == text_df.ncases(), "clean should preserve case count");

  const DataFrame trim_df({
      {"raw", pluss::table::Thread(pluss::make_note({"  alpha  ", "\tbeta", "gamma \n"}))},
  });
  auto trimmed = trim_df;
  trimmed.set("raw") = trim_df.features("raw").strip();
  expect(std::string(trimmed.features("raw").as_note().at(0)) == "alpha"
             && std::string(trimmed.features("raw").as_note().at(1)) == "beta"
             && std::string(trimmed.features("raw").as_note().at(2)) == "gamma",
         "Thread::strip should trim Note payload values for dataframe feature workflows");

  const DataFrame id_df({
      {"student_id", pluss::table::Thread(pluss::make_note({"#203930", "##7788", "4455##"}))},
  });
  auto left_trimmed = id_df;
  left_trimmed.set("student_id") = id_df.features("student_id").lstrip("#");
  expect(std::string(left_trimmed.features("student_id").as_note().at(0)) == "203930"
             && std::string(left_trimmed.features("student_id").as_note().at(1)) == "7788"
             && std::string(left_trimmed.features("student_id").as_note().at(2)) == "4455##",
         "Thread::lstrip(chars) should remove only leading matching characters");
  auto right_trimmed = id_df;
  right_trimmed.set("student_id") = id_df.features("student_id").rstrip("#");
  expect(std::string(right_trimmed.features("student_id").as_note().at(0)) == "#203930"
             && std::string(right_trimmed.features("student_id").as_note().at(1)) == "##7788"
             && std::string(right_trimmed.features("student_id").as_note().at(2)) == "4455",
         "Thread::rstrip(chars) should remove only trailing matching characters");
  const auto keywords = text_catalog.keywords("txt", 3).tabelize();
  expect(keywords.get("keyword").has_value() && keywords.get("count").has_value(),
         "keywords should return keyword/count features");
  const auto sim_df = text_catalog.similarity("txt", "txt2").tabelize();
  expect(sim_df.ncases() == text_df.ncases(), "similarity should emit one score per case");
  const auto filtered_text = text_catalog.grep("txt", "rain").tabelize();
  expect(filtered_text.ncases() == 1, "grep should keep matching text rows");
  const auto enriched_text = text_catalog.featurize("txt").tabelize();
  expect(enriched_text.get("txt_char_len").has_value() && enriched_text.get("txt_word_count").has_value(),
         "featurize should append generated text features");
  const auto text_stats = text_catalog.stats("txt", "count").tabelize();
  expect(text_stats.ncases() == 1 && text_stats.get("value").has_value(),
         "stats should emit one-row stat table");
  const auto text_tfidf = text_catalog.tfidf("txt").tabelize();
  expect(text_tfidf.get("txt_tfidf_norm").has_value(), "tfidf should append tfidf norm feature");
  const auto from_catalog = df.head(5, false).kindelize().tabelize();
  expect(from_catalog.ncases() == 5, "DataFrame::kindelize().tabelize() should reconstruct frame rows");

  const auto summary = df.aggregate(
      {
          {"cases", "age", DataFrame::SummaryOp::Count},
          {"age_mean", "age", DataFrame::SummaryOp::Mean},
          {"jobs", "job", DataFrame::SummaryOp::Ndistinct},
      },
      false);
  expect(summary.ncases() == 1, "summarize should produce one summary row");
  expect(summary.nfeatures() == 3, "summarize should emit requested output features");
  expect(summary.features("cases").as_series().to_vec<std::int64_t>(0) == df.ncases(),
         "summarize count should match frame row count");
  expect(summary.features("jobs").as_series().to_vec<std::int64_t>(0) > 0,
         "summarize ndistinct should be positive for job feature");

  const auto text_only = df.feataxis().note();
  expect(text_only.nfeatures() == 1 && text_only.get("job").has_value(),
         "note_features() should retain only Note-backed features");

  const auto numeric_only = df.feataxis().series();
  expect(numeric_only.nfeatures() == 3 && numeric_only.get("age").has_value()
             && numeric_only.get("balance").has_value() && numeric_only.get("duration").has_value(),
         "series_features() should retain Series-backed features");

  const auto matrix_vals = df.matrix();
  expect(std::get<0>(matrix_vals).size() == 1 && std::get<1>(matrix_vals).size() == 1
             && std::get<2>(matrix_vals).empty() && std::get<3>(matrix_vals).empty(),
         "matrix() should group frame values into typed matrix blocks");

  const auto numeric_matrix = std::get<0>(matrix_vals).front();
  const auto numeric_shape = numeric_matrix.sizes();
  expect(numeric_shape.size() == 2 && numeric_shape[0] == df.ncases() && numeric_shape[1] == 3,
         "matrix() numeric block should have [ncases, n_series_features] shape");

  const auto note_matrix = std::get<1>(matrix_vals).front();
  const auto note_shape = note_matrix.sizes();
  expect(note_shape.size() == 2 && note_shape[0] == df.ncases() && note_shape[1] == 1,
         "matrix() note block should have [ncases, n_note_features] shape");

  const auto age_thread = df.features("age");
  const auto mean_scalar = age_thread.mean();
  expect(mean_scalar.defined(), "mean() should return a defined scalar ThreadValue");
  const auto mean_series = mean_scalar.as_series();
  expect(mean_series.numel() == 1, "mean() should return scalar-sized Series payload");
  const auto mean_vals = mean_series.to_vec<double>();
  expect(mean_vals.size() == 1, "mean() scalar payload should contain a single value");
  const auto logged = mean_series.log();
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

  const auto age_shifted = age_thread.shift(1);
  expect(pluss::table::detail::has_validity_mask(age_shifted), "Thread::shift should null-pad shifted edge");
  const auto age_shifted_valid = pluss::table::detail::validity_mask(age_shifted)->to_vec<bool>();
  expect(!age_shifted_valid.front(),
         "Thread::shift(periods=1) should set first row null");
  expect(age_shifted.as_series().to_vec<std::int64_t>(1) == age_thread.as_series().to_vec<std::int64_t>(0),
         "Thread::shift(periods=1) should move values by one case");

  const auto age_self_corr = age_thread.corr(age_thread);
  const auto age_bal_corr = age_thread.corr(df.features("balance"));
  const auto age_bal_spearman = age_thread.corr(df.features("balance"), pluss::table::ThreadCorrMethod::Spearman);
  const auto age_bal_kendall = age_thread.corr(df.features("balance"), pluss::table::ThreadCorrMethod::KendallTau);
  expect(std::fabs(age_self_corr - 1.0) < 1e-12, "Thread::corr(self) should be 1 for non-constant Series");
  expect(std::isfinite(age_bal_corr) && std::isfinite(age_bal_spearman) && std::isfinite(age_bal_kendall),
         "Thread::corr should return finite coefficients on numeric inputs");

  auto df_assign = df;
  const auto age_before = df_assign.features("age").as_series().to_vec<std::int64_t>();
  df_assign.set_("age", df_assign.features("age") + 7.0);
  const auto age_after = df_assign.features("age").as_series().to_vec<std::int64_t>();
  expect(age_after.size() == age_before.size(), "feature assignment should preserve case count");
  expect(age_after[0] == age_before[0] + 7, "set_features_(\"age\", feature(\"age\") + 7) should update in place");

  df_assign.set_("age_plus_1", df_assign.features("age") + 1.0);
  expect(df_assign.get("age_plus_1").has_value(),
         "set_features_(\"new\", feature(\"curr\") + 1) should create a new feature");
  const auto age_plus = df_assign.features("age_plus_1").as_series().to_vec<std::int64_t>();
  expect(age_plus[0] == age_after[0] + 1, "new feature assignment should store RHS values");

  auto df_bulk = df;
  const auto age_before_bulk = df_bulk.features("age").as_series().to_vec<std::int64_t>();
  const auto dur_before_bulk = df_bulk.features("duration").as_series().to_vec<std::int64_t>();
  df_bulk.set_("age", df_bulk.features("age") + 2.0);
  df_bulk.set_("duration", df_bulk.features("duration") + 3.0);
  const auto age_after_bulk = df_bulk.features("age").as_series().to_vec<std::int64_t>();
  const auto dur_after_bulk = df_bulk.features("duration").as_series().to_vec<std::int64_t>();
  expect(age_after_bulk[0] == age_before_bulk[0] + 2 && dur_after_bulk[0] == dur_before_bulk[0] + 3,
         "set_features_ should update multiple existing features in place");

  df_bulk.set_("age_copy2", df_bulk.features("age"));
  df_bulk.set_("dur_copy2", df_bulk.features("duration"));
  expect(df_bulk.get("age_copy2").has_value() && df_bulk.get("dur_copy2").has_value(),
         "set_features_ should create missing features");

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
  df_bulk.set_("age_copy3", copied.features("age"));
  df_bulk.set_("dur_copy3", copied.features("duration"));
  expect(df_bulk.get("age_copy3").has_value() && df_bulk.get("dur_copy3").has_value(),
         "feature({..}) should return DataFrame for multi-feature copy workflows");

  const auto temporal_only = df.feataxis().timeline();
  expect(temporal_only.nfeatures() == 0, "timeline_features() should be empty when no temporal features exist");

  expect(df.features("age").as_series().numel() == df.ncases(), "as_series should expose Series payload");
  expect(df.features("job").as_note().numel() == df.ncases(), "as_note should expose Note payload");

  const auto cast_age_to_f64 = df.features({"age"}).astype(pluss::codem::ScalarType::Float64);
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
         "assign should add a bool Series comparison feature");

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
         "assign should add a bool Series membership feature");

  const auto tmp_time = write_temp_csv(
      "time_feature_helpers",
      "start,end\n"
      "2020-01-01 00:00:00,2020-01-01 01:30:00\n"
      "2020-01-02 08:00:00,2020-01-02 09:00:00\n");
  auto tdf = DataFrame::load_csv(tmp_time.string());
  tdf.set({"start", "end"}) = tdf.features({"start", "end"}).cast().timeline();
  tdf = tdf.assign({
                           {"Start_Year", [](const DataFrame& frame) {
                              return pluss::table::Thread(frame.features("start").as_timeline().year());
                            }},
                           {"Start_Hour", [](const DataFrame& frame) {
                              return pluss::table::Thread(frame.features("start").as_timeline().hour());
                            }},
                           {"Duration_min", [](const DataFrame& frame) {
                              auto end_ns = frame.features("end")
                                                .cast({.series_dtype = pluss::codem::ScalarType::Int64})
                                                .series()
                                                .as_series()
                                                .to(pluss::codem::ScalarType::Float64);
                              auto start_ns = frame.features("start")
                                                  .cast({.series_dtype = pluss::codem::ScalarType::Int64})
                                                  .series()
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

  expect(tdf.feataxis().timeline().nfeatures() == 2,
         "timeline_features() should retain Timeline features after cast(Timeline)");
  expect(tdf.features("start").as_timeline().numel() == 2, "as_timeline should expose Timeline payload");
  auto tdf_timeindex = tdf;
  tdf_timeindex.set("start") = tdf.features("start").cast().timeindex();
  expect(tdf_timeindex.feataxis().timeline().nfeatures() == 2 && tdf_timeindex.get("start").has_value()
             && tdf_timeindex.get("end").has_value(),
         "timeline_features() should retain Timeline/TimeIndex-backed features");
  const auto shifted_tdf = tdf.features({"start"}).tl().shift(1, "D");
  expect(shifted_tdf.ncases() == tdf.ncases() && shifted_tdf.get("start").has_value(),
         "shift_time should preserve shape and update selected temporal feature");

  const auto shifted_age = df.features({"age"}).shift(1);
  expect(shifted_age.ncases() == df.ncases() && shifted_age.nfeatures() == 1,
         "features(...).shift(periods) should preserve case count and selected feature count");
  expect(pluss::table::detail::has_validity_mask(shifted_age.features("age")),
         "shift should introduce null validity at shifted edge");
  const auto shifted_age_valid = pluss::table::detail::validity_mask(shifted_age.features("age"))->to_vec<bool>();
  const auto shifted_age_values = shifted_age.features("age").as_series().to_vec<std::int64_t>();
  const auto base_age_values = df.features("age").as_series().to_vec<std::int64_t>();
  expect(!shifted_age_valid.front() && shifted_age_values[1] == base_age_values[0],
         "shift(periods=1) should move values down and mark first row null");

  const auto shifted_job_lead = df.features({"job"}).shift(-1);
  expect(pluss::table::detail::has_validity_mask(shifted_job_lead.features("job")),
         "shift with negative periods should null-pad tail rows");
  const auto shifted_job_valid = pluss::table::detail::validity_mask(shifted_job_lead.features("job"))->to_vec<bool>();
  expect(!shifted_job_valid.back(),
         "shift(periods=-1) should set last row null");
  expect(std::string(shifted_job_lead.features("job").as_note().at(0))
             == std::string(df.features("job").as_note().at(1)),
         "shift(periods=-1) should move values up by one case");

  const auto zero_shift = df.shift(0);
  expect(zero_shift.equals(df), "shift(periods=0) should be identity");

  const auto floored_tdf = tdf.features({"start"}).tl().floor("D");
  expect(floored_tdf.ncases() == tdf.ncases(), "tl().floor should preserve shape");
  const auto localized_tdf = tdf.features({"start"}).tl().localize({.tz = "UTC"});
  const auto utc_tdf = localized_tdf.features({"start"}).tl().utc();
  expect(utc_tdf.ncases() == tdf.ncases(), "tz_localize/to_utc should preserve shape");
  const auto daily_mean = tdf.features({"Duration_min"})
                              .ts({.axis = tdf.features("start")})
                              .resample("D", pluss::subsp::stats::ts::ResampleOp::Mean);
  expect(daily_mean.ncases() == 2 && daily_mean.get("Duration_min").has_value(),
         "resample should aggregate values onto time bins");

  const auto years = tdf.features("Start_Year").as_series().to_vec<std::int64_t>();
  expect(years.size() == 2 && years[0] == 2020 && years[1] == 2020,
         "assign should expose Timeline year extraction");

  const auto mins = tdf.features("Duration_min").as_series().to_vec<double>();
  expect(mins.size() == 2 && std::fabs(mins[0] - 90.0) < 1e-6 && std::fabs(mins[1] - 60.0) < 1e-6,
         "assign should compute End_Time - Start_Time in minutes");

  const auto rush = tdf.features("Is_RushHour").as_series().to_vec<bool>();
  expect(rush.size() == 2 && !rush[0] && rush[1],
         "assign should mark Start_Hour membership against provided values");

  const auto job_counts = df.group({.dropnull = true})
                              .by({"job"})
                              .agg({
                                  {"count", "", DataFrame::GroupOp::Count},
                              })
                              .top("count", 5);
  expect(job_counts.nfeatures() == 2, "group count should emit key + count features");
  expect(job_counts.features()[0] == "job" && job_counts.features()[1] == "count",
         "group count feature naming mismatch");
  expect(job_counts.ncases() <= 5, "group count top(count, n) should cap row count");

  const auto job_age_mean = df.group({.dropnull = true})
                                .by({"job"})
                                .agg({
                                    {"mean_age", "age", DataFrame::GroupOp::Mean},
                                    {"count", "age", DataFrame::GroupOp::Count},
                                })
                                .top("mean_age", 5);
  expect(job_age_mean.nfeatures() == 3, "group mean should emit group + mean + count features");
  expect(job_age_mean.features()[0] == "job"
             && job_age_mean.features()[1] == "mean_age"
             && job_age_mean.features()[2] == "count",
         "group mean feature naming mismatch");
  expect(job_age_mean.ncases() <= 5, "group mean top(mean_age, n) should cap row count");

  const auto name_counts_keep_na = df_nulls.group({.dropnull = false})
                                       .by({"name"})
                                       .agg({
                                           {"count", "", DataFrame::GroupOp::Count},
                                       });
  expect(name_counts_keep_na.ncases() == 3, "group(dropnull=false) should include NA bucket");

  const auto grouped_job = df.group({.dropnull = true})
                               .by({"job"})
                               .agg({
                                   {"cases", "", DataFrame::GroupOp::Count},
                                   {"age_mean", "age", DataFrame::GroupOp::Mean},
                                   {"job_ndistinct", "job", DataFrame::GroupOp::Ndistinct},
                               });
  expect(grouped_job.nfeatures() == 4, "group(single key) should emit key + requested aggregates");
  expect(grouped_job.get("job").has_value() && grouped_job.get("cases").has_value()
             && grouped_job.get("age_mean").has_value() && grouped_job.get("job_ndistinct").has_value(),
         "group(single key) missing expected output features");

  const auto grouped_multi = df.group({.dropnull = true})
                                 .by({"job", "age"})
                                 .agg({
                                     {"cases", "", DataFrame::GroupOp::Count},
                                 });
  expect(grouped_multi.nfeatures() == 3, "group(multi key) should emit all keys + aggregates");
  expect(grouped_multi.ncases() > 0, "group(multi key) should produce grouped rows");

  const auto left_small = df.features({"age", "job"}).head(6, false);
  const auto right_small = df.features({"age", "duration"}).head(6, false);
  const auto combined_features = left_small.combine(
      right_small,
      {DataFrame::CombineOp::Features, "_r", true});
  expect(combined_features.nfeatures() == 4,
         "combine(features) should append right-hand features and resolve collisions");
  expect(combined_features.get("age").has_value() && combined_features.get("job").has_value()
             && combined_features.get("age_r").has_value() && combined_features.get("duration").has_value(),
         "combine(features) expected feature names missing");

  const auto bump_age = left_small.features("age").as_series().add(100.0);
  auto right_update = left_small;
  right_update.set("age") = pluss::table::Thread(bump_age);
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
  expect(agg_out.ncases() == 1 && agg_out.nfeatures() == 1, "aggregate should summarize requested specs");

  auto let_df = df.features({"age", "duration", "balance"});
  let_df.set("age_copy") = let_df.features("age");
  expect(let_df.get("age_copy").has_value(), "set(single)=Thread should create/update feature");
  expect(let_df.features("age_copy").as_series().to_vec<std::int64_t>() == let_df.features("age").as_series().to_vec<std::int64_t>(),
         "set(single)=Thread should preserve assigned values");

  let_df.set("age_plus_duration") = let_df.features("age") + let_df.features("duration");
  expect(let_df.get("age_plus_duration").has_value(), "set(single)=expression should assign computed Thread");

  let_df.set("age") = let_df.features("duration");
  expect(let_df.features("age").as_series().to_vec<std::int64_t>() ==
             let_df.features("duration").as_series().to_vec<std::int64_t>(),
         "set(single) should allow assigning into an existing feature");

  let_df.set({"age_rhs", "dur_rhs"}) = let_df.features({"age", "duration"});
  expect(let_df.get("age_rhs").has_value() && let_df.get("dur_rhs").has_value(),
         "set(multi)=DataFrame should assign multiple features");

  let_df.set({"age_rhs2", "dur_rhs2"}) = {let_df.features("age"), let_df.features("duration")};
  expect(let_df.get("age_rhs2").has_value() && let_df.get("dur_rhs2").has_value(),
         "set(multi)=initializer_list<Thread> should assign multiple features");

  const auto short_age = let_df.head(3, false).features("age");
  bool partial_age_default_threw = false;
  try {
    let_df.set("partial_age") = short_age;
  } catch (const std::exception&) {
    partial_age_default_threw = true;
  }
  expect(partial_age_default_threw,
         "set(single) default should reject partial rhs length mismatch");
  let_df.set("partial_age", {.align = pluss::table::AlignBy::Caselbl}) = short_age;
  expect(let_df.get("partial_age").has_value(), "set(single) should accept labeled partial assignment");
  expect(pluss::table::detail::validity_mask(let_df.features("partial_age"))->to_vec<bool>()[0]
             && pluss::table::detail::validity_mask(let_df.features("partial_age"))->to_vec<bool>()[1]
             && pluss::table::detail::validity_mask(let_df.features("partial_age"))->to_vec<bool>()[2]
             && !pluss::table::detail::validity_mask(let_df.features("partial_age"))->to_vec<bool>()[3],
         "set(single) should align labeled partial rhs and null-fill unmatched cases");
  bool default_partial_threw = false;
  try {
    let_df.set("default_partial") =
        pluss::table::Thread(pluss::make_series(std::vector<std::int64_t>{1, 2, 3}));
  } catch (const std::exception&) {
    default_partial_threw = true;
  }
  expect(default_partial_threw,
         "set(single) default should reject default-label rhs length mismatch");
  let_df.set("default_partial", {.align = pluss::table::AlignBy::Caselbl}) =
      pluss::table::Thread(pluss::make_series(std::vector<std::int64_t>{1, 2, 3}));
  expect(pluss::table::detail::validity_mask(let_df.features("default_partial"))->to_vec<bool>()[0]
             && pluss::table::detail::validity_mask(let_df.features("default_partial"))->to_vec<bool>()[1]
             && pluss::table::detail::validity_mask(let_df.features("default_partial"))->to_vec<bool>()[2]
             && !pluss::table::detail::validity_mask(let_df.features("default_partial"))->to_vec<bool>()[3],
         "set(single) should align default-case rhs and null-fill unmatched cases");
  bool multi_default_partial_threw = false;
  try {
    let_df.set({"default_a", "default_b"}) = {
        let_df.features("age"),
        pluss::table::Thread(pluss::make_series(std::vector<std::int64_t>{1, 2, 3})),
    };
  } catch (const std::exception&) {
    multi_default_partial_threw = true;
  }
  expect(multi_default_partial_threw,
         "set(multi) default should reject partial rhs length mismatch");
  let_df.set({"default_a", "default_b"}, {.align = pluss::table::AlignBy::Caselbl}) = {
      let_df.features("age"),
      pluss::table::Thread(pluss::make_series(std::vector<std::int64_t>{1, 2, 3})),
  };
  expect(let_df.get("default_a").has_value() && let_df.get("default_b").has_value() &&
             !pluss::table::detail::validity_mask(let_df.features("default_b"))->to_vec<bool>()[3],
         "set(multi) should align each rhs by cases and null-fill unmatched cases");

  const auto first_duration =
      let_df.features("duration").as_series().to_vec<std::int64_t>(0);
  const auto selected_duration =
      let_df.features("duration") == static_cast<double>(first_duration);
  const auto age_put = let_df.features({"age"}).put(selected_duration, 60.0);
  const auto selected = selected_duration.as_series().to_vec<bool>();
  const auto before_put = let_df.features("age").as_series().to_vec<std::int64_t>();
  const auto after_put = age_put.features("age").as_series().to_vec<std::int64_t>();
  for (std::size_t row = 0; row < selected.size(); ++row) {
    expect(after_put[row] == (selected[row] ? 60 : before_put[row]),
           selected[row]
               ? "put(feature, mask, double) should assign into masked numeric rows"
               : "put(feature, mask, double) should preserve unmasked numeric rows");
  }

  auto put_inplace = df.features({"age", "job"});
  const auto put_ages =
      put_inplace.features("age").as_series().to_vec<std::int64_t>();
  put_inplace.put_(
      "job",
      put_inplace.features("age") == static_cast<double>(put_ages[0]),
      "mentor");
  expect(std::string(put_inplace.features("job").as_note().at(0)) == "mentor",
         "put_(feature, mask, string) should update masked Note rows in place");
  expect(put_ages[1] == put_ages[0] ||
             std::string(put_inplace.features("job").as_note().at(1)) != "mentor",
         "put_(feature, mask, string) should preserve unmasked Note rows");

  const auto nulled_job = put_inplace.features({"job"}).put(
      put_inplace.features("age") == static_cast<double>(put_ages[1]),
      pluss::null);
  expect(nulled_job.features("job").isnull().as_series().to_vec<bool>()[1],
         "put(feature, mask, null) should nullify masked rows");

  const auto numeric = df.features({"age", "balance", "duration"});
  const auto age0 = numeric.features("age").as_series().to_vec<std::int64_t>(0);

  const auto add_scalar = numeric.add(1.0);
  expect(add_scalar.features("age").as_series().to_vec<std::int64_t>(0) == age0 + 1,
         "add(scalar) should shift numeric Series features");
  const auto add_frame = numeric.add(numeric);
  expect(add_frame.features("age").as_series().to_vec<std::int64_t>(0) == age0 * 2,
         "add(frame) should add feature-wise values");

  const auto sub_alias = numeric.sub(1.0);
  expect(sub_alias.features("age").as_series().to_vec<std::int64_t>(0) == age0 - 1,
         "subtract alias should delegate to sub");
  const auto mul_alias = numeric.mul(2.0);
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
  expect(reverse_add.ncases() == numeric.ncases() && reverse_add.nfeatures() == numeric.nfeatures(),
         "radd should preserve frame shape");
  expect(reverse_mul.ncases() == numeric.ncases() && reverse_mul.nfeatures() == numeric.nfeatures(),
         "rmul should preserve frame shape");
  expect(reverse_div.ncases() == numeric.ncases() && reverse_div.nfeatures() == numeric.nfeatures(),
         "rdiv should preserve frame shape");
  expect(reverse_true_div.ncases() == numeric.ncases() && reverse_true_div.nfeatures() == numeric.nfeatures(),
         "rtruediv should preserve frame shape");
  expect(reverse_floor_div.ncases() == numeric.ncases() && reverse_floor_div.nfeatures() == numeric.nfeatures(),
         "rfloordiv should preserve frame shape");
  expect(reverse_mod.ncases() == numeric.ncases() && reverse_mod.nfeatures() == numeric.nfeatures(),
         "rmod should preserve frame shape");
  expect(reverse_pow.ncases() == numeric.ncases() && reverse_pow.nfeatures() == numeric.nfeatures(),
         "rpow should preserve frame shape");

  const auto div_alias = numeric.div(2.0);
  const auto true_div = numeric.truediv(2.0);
  const auto floor_div = numeric.floordiv(2.0);
  const auto mod_alias = numeric.mod(2.0);
  const auto pow_alias = numeric.pow(2.0);
  expect(div_alias.ncases() == numeric.ncases() && div_alias.nfeatures() == numeric.nfeatures(),
         "divide alias should preserve frame shape");
  expect(true_div.ncases() == numeric.ncases() && true_div.nfeatures() == numeric.nfeatures(),
         "truediv should preserve frame shape");
  expect(floor_div.ncases() == numeric.ncases() && floor_div.nfeatures() == numeric.nfeatures(),
         "floordiv should preserve frame shape");
  expect(mod_alias.ncases() == numeric.ncases() && mod_alias.nfeatures() == numeric.nfeatures(),
         "mod should preserve frame shape");
  expect(pow_alias.ncases() == numeric.ncases() && pow_alias.nfeatures() == numeric.nfeatures(),
         "pow should preserve frame shape");
  const auto eq_self = numeric.eq(numeric);
  const auto ne_self = numeric.ne(numeric);
  const auto ge_scalar = numeric.gt(45.0, false);
  const auto lt_scalar = numeric.lt(45.0);
  const auto le_scalar = numeric.lt(45.0, false);
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

  const auto miss_isnull = miss.isnull();
  expect(miss_isnull.nfeatures() == miss.nfeatures(), "missing(isnull) should retain all features by default");
  expect(miss_isnull.features("score").as_series().scalar_type() == pluss::codem::ScalarType::Bool,
         "missing(isnull) should return bool mask features");
  expect(miss_isnull.features("score").as_series().to_vec<bool>(1),
         "missing(isnull) should mark null score rows");
  const auto miss_any = miss_isnull.any(Axis::Feature);
  expect(miss_any.kind() == ThreadKind::Series, "DataFrame::any should return a Series-backed Thread");
  const auto miss_any_values = miss_any.as_series().to_vec<bool>();
  expect(miss_any_values == std::vector<bool>({false, true, false, true}),
         "DataFrame::any should reduce bool features row-wise with OR semantics");
  expect(miss.filter(miss_any).ncases() == 2,
         "filter(df.isnull().any()) should keep rows where any selected feature is null");
  const auto miss_all = miss_isnull.features({"score", "label"}).all(Axis::Feature);
  const auto miss_all_values = miss_all.as_series().to_vec<bool>();
  expect(miss_all_values == std::vector<bool>({false, true, false, false}),
         "DataFrame::all should reduce bool features row-wise with AND semantics");
  expect(miss.filter(miss_all).ncases() == 1,
         "filter(df.isnull().all({score,label})) should keep rows where both selected features are null");
  const auto miss_any_score = miss_isnull.features({"score"}).any(Axis::Feature);
  expect(miss_any_score.as_series().to_vec<bool>() == miss_isnull.features("score").as_series().to_vec<bool>(),
         "DataFrame::any(single feature) should match the source bool feature");
  expect_throw([&]() { (void)miss.features({"score"}).any(); },
               "DataFrame::any should reject non-bool features");
  const auto miss_any_axis1 = miss_isnull.any(Axis::Feature);
  expect(miss_any_axis1.as_series().to_vec<bool>() == miss_any_values,
         "DataFrame::any(axis=1) should reduce row-wise");
  const auto miss_any_axis0 = miss_isnull.any(Axis::Case);
  expect(miss_any_axis0.as_series().to_vec<bool>() == std::vector<bool>({false, true, true}),
         "DataFrame::any(axis=0) should reduce over cases and return one value per feature");
  expect(miss_any_axis0.label().has(), "DataFrame::any(axis=0) should carry feature labels");
  const auto miss_any_axis0_labels = miss_any_axis0.label().values().as_note();
  expect(std::string(miss_any_axis0_labels.at(0)) == "id"
             && std::string(miss_any_axis0_labels.at(1)) == "score"
             && std::string(miss_any_axis0_labels.at(2)) == "label",
         "DataFrame::any(axis=0) labels should match reduced feature order");
  const auto miss_all_axis0 = miss_isnull.all(Axis::Case);
  expect(miss_all_axis0.as_series().to_vec<bool>() == std::vector<bool>({false, false, false}),
         "DataFrame::all(axis=0) should reduce over cases and return one value per feature");
  expect_throw([&]() { (void)miss_isnull.any(static_cast<Axis>(2)); },
               "DataFrame::any(axis) should reject unsupported axis values");
  expect(miss_isnull.features("score").any().as_series().item<bool>(),
         "Thread::any should reduce Bool Series thread to a scalar true/false");
  expect(!miss_isnull.features("score").all().as_series().item<bool>(),
         "Thread::all should reduce Bool Series thread to a scalar true/false");

  const auto miss_notnull = miss.notnull();
  expect(!miss_notnull.features("score").as_series().to_vec<bool>(1),
         "missing(notnull) should invert isnull mask");

  const auto miss_isna = miss.isnull({"score"});
  const auto miss_notna = miss.notnull({"score"});
  expect(miss_isna.nfeatures() == 1 && miss_notna.nfeatures() == 1,
         "isnull/notnull should support selected feature subsets");
  expect(miss_isna.features("score").as_series().to_vec<bool>(1),
         "isnull should mark null rows");
  expect(!miss_notna.features("score").as_series().to_vec<bool>(1),
         "notnull should invert isnull mask");

  const auto miss_filled = miss.fillnull(0.0, {"score"});
  expect(miss_filled.features("score").as_series().to_vec<std::int64_t>(1) == 0,
         "fillnull(value, features) should replace selected numeric nulls");
  expect(miss_filled.features("score").as_series().to_vec<std::int64_t>()
             == std::vector<std::int64_t>({10, 0, 30, 0}),
         "fillnull(value, features) should preserve selected numeric non-null values");
  expect(!pluss::table::detail::has_validity_mask(miss_filled.features("score")),
         "fillnull(value, features) should clear numeric validity mask when all nulls are filled");
  expect(std::string(miss_filled.features("label").as_note().at(1)) == std::string(miss.features("label").as_note().at(1)),
         "missing(fill) on selected features should not mutate unselected features");

  const auto miss_note_filled = miss.fillnull("missing", {"label"});
  expect(std::string(miss_note_filled.features("label").as_note().at(1)) == "missing",
         "fillnull(string, features) should replace null Note values");
  expect(!pluss::table::detail::has_validity_mask(miss_note_filled.features("label")),
         "fillnull(string, features) should clear Note validity mask when all nulls are filled");

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
  const auto mixed_ts_filled = mixed_missing.fillnull(ts_fill, {"ts"});
  const auto ts_ticks = mixed_ts_filled.features("ts").as_timeline().to_epoch_nanos_vector();
  const auto ts_fill_tick = ts_fill.to_epoch_nanos_vector()[0];
  expect(ts_ticks[1] == ts_fill_tick && ts_ticks[3] == ts_fill_tick,
         "fillnull(Timeline, features) should replace null Timeline values");
  expect(!pluss::table::detail::has_validity_mask(mixed_ts_filled.features("ts")),
         "fillnull(Timeline, features) should clear Timeline validity mask when all nulls are filled");
  expect(pluss::table::detail::has_validity_mask(mixed_ts_filled.features("ti")),
         "fillnull(Timeline, features) should not mutate unselected TimeIndex features");

  const auto ti_fill = pluss::make_timeindex("2024-02-01T00:00:00Z", false);
  const auto mixed_ti_filled = mixed_missing.fillnull(ti_fill, {"ti"});
  const auto ti_ticks = mixed_ti_filled.features("ti").as_timeindex().timeline().to_epoch_nanos_vector();
  const auto ti_fill_tick = ti_fill.timeline().to_epoch_nanos_vector()[0];
  expect(ti_ticks[1] == ti_fill_tick && ti_ticks[3] == ti_fill_tick,
         "fillnull(TimeIndex, features) should replace null TimeIndex values");
  expect(!pluss::table::detail::has_validity_mask(mixed_ti_filled.features("ti")),
         "fillnull(TimeIndex, features) should clear TimeIndex validity mask when all nulls are filled");

  const auto miss_ffill = miss.ffill();
  expect(miss_ffill.features("score").as_series().to_vec<std::int64_t>(1) == 10,
         "ffill should propagate previous numeric value");
  expect(std::string(miss_ffill.features("label").as_note().at(1)) == "a",
         "ffill should propagate previous Note value");

  const auto miss_bfill = miss.bfill();
  expect(miss_bfill.features("score").as_series().to_vec<std::int64_t>(1) == 30,
         "bfill should propagate next numeric value");
  expect(pluss::table::detail::has_validity_mask(miss_bfill.features("score")),
         "bfill should preserve trailing null when no backward source exists");
  expect(!pluss::table::detail::validity_mask(miss_bfill.features("score"))->to_vec<bool>(3),
         "bfill should keep trailing nulls unresolved");

  auto limited_validity = pluss::Series::zeros({4}, pluss::codem::ScalarType::Bool);
  auto* limited_valid_ptr = reinterpret_cast<bool*>(limited_validity.raw_data());
  limited_valid_ptr[0] = true;
  limited_valid_ptr[1] = false;
  limited_valid_ptr[2] = false;
  limited_valid_ptr[3] = true;
  const auto limited = DataFrame({
      {"score", pluss::table::Thread(pluss::make_series(std::vector<double>{1.0, 0.0, 0.0, 4.0}), limited_validity)},
      {"label", pluss::table::Thread(pluss::make_note(std::vector<std::string>{"a", "", "", "d"}), limited_validity)},
  });

  const auto limited_ffill = limited.ffill(DataFrame::FillOptions{{"score"}, 1});
  expect(pluss::table::detail::validity_mask(limited_ffill.features("score"))->to_vec<bool>()
             == std::vector<bool>({true, true, false, true}),
         "ffill(limit) should fill only the configured number of consecutive nulls");
  expect(limited_ffill.features("score").as_series().to_vec<double>(1) == 1.0,
         "ffill(limit) should use the previous valid numeric value");

  const auto limited_bfill = limited.bfill(DataFrame::FillOptions{{"score"}, 1});
  expect(pluss::table::detail::validity_mask(limited_bfill.features("score"))->to_vec<bool>()
             == std::vector<bool>({true, false, true, true}),
         "bfill(limit) should fill only the configured number of consecutive nulls");
  expect(limited_bfill.features("score").as_series().to_vec<double>(2) == 4.0,
         "bfill(limit) should use the next valid numeric value");

  const auto limited_interp = limited.interp(DataFrame::FillOptions{{"score"}, 1});
  expect(pluss::table::detail::validity_mask(limited_interp.features("score"))->to_vec<bool>()
             == std::vector<bool>({true, true, false, true}),
         "interp(limit) should preserve unresolved nulls beyond the limit");
  expect(std::abs(limited_interp.features("score").as_series().to_vec<double>(1) - 2.0) < 1e-12,
         "interp(limit) should linearly interpolate interior numeric gaps");

  const auto mapped_fill = limited.fillnull({
      DataFrame::FillSpec{"score", pluss::table::Thread(pluss::make_series(std::vector<double>{99.0}))},
      DataFrame::FillSpec{"label", pluss::table::Thread(pluss::make_note(std::vector<std::string>{"missing"}))},
  });
  expect(mapped_fill.features("score").as_series().to_vec<double>(1) == 99.0
             && mapped_fill.features("score").as_series().to_vec<double>(2) == 99.0,
         "fillnull(FillSpec...) should fill numeric features with per-feature scalar Threads");
  expect(std::string(mapped_fill.features("label").as_note().at(1)) == "missing"
             && std::string(mapped_fill.features("label").as_note().at(2)) == "missing",
         "fillnull(FillSpec...) should fill Note features with per-feature scalar Threads");

  auto null_mask = pluss::Series::zeros({4}, pluss::codem::ScalarType::Bool);
  auto* null_mask_ptr = reinterpret_cast<bool*>(null_mask.raw_data());
  null_mask_ptr[0] = false;
  null_mask_ptr[1] = true;
  null_mask_ptr[2] = false;
  null_mask_ptr[3] = true;
  const auto masked_score = miss.mask(pluss::table::Thread(null_mask), {"score"});
  expect(pluss::table::detail::validity_mask(masked_score.features("score"))->to_vec<bool>()
             == std::vector<bool>({true, false, true, false}),
         "mask(mask, features) should set selected cells to null while preserving unmasked values");
  expect(!pluss::table::detail::has_validity_mask(masked_score.features("id")),
         "mask(mask, features) should not mutate unselected features");

  const auto miss_drop_any = miss.dropnull();
  expect(miss_drop_any.ncases() == 2, "dropnull should drop rows with any null by default");

  const auto miss_drop_all = miss.dropnull({"score", "label"}, true);
  expect(miss_drop_all.ncases() == 3, "missing(drop all) should drop rows only when all selected features are null");

  const auto miss_drop_threshold = miss.dropnull(DataFrame::DropOptions{
      .features = {"score", "label"},
      .axis = Axis::Case,
      .threshold = 1,
  });
  expect(miss_drop_threshold.ncases() == 3,
         "dropnull(DropOptions threshold, Axis::Case) should keep rows meeting the non-null threshold");

  const auto miss_drop_features = miss.dropnull(DataFrame::DropOptions{
      .features = {"score", "label"},
      .axis = Axis::Feature,
      .threshold = 3,
  });
  expect(miss_drop_features.nfeatures() == 2
             && miss_drop_features.get("id").has_value()
             && miss_drop_features.get("label").has_value()
             && !miss_drop_features.get("score").has_value(),
         "dropnull(DropOptions threshold, Axis::Feature) should drop selected features below threshold");

  const auto arranged_age = df.arrange({"age"});
  const auto arranged_age_values = arranged_age.features("age").as_series().to_vec<std::int64_t>();
  for (std::size_t i = 1; i < arranged_age_values.size(); ++i) {
    expect(arranged_age_values[i - 1] <= arranged_age_values[i],
           "arrange(feature) should sort ascending by default");
  }

  const auto arranged_score = miss.arrange({"score"});
  const auto arranged_score_ids = arranged_score.features("id").as_series().to_vec<std::int64_t>();
  expect(arranged_score_ids[0] == 1 && arranged_score_ids[1] == 3 && arranged_score_ids[2] == 2 && arranged_score_ids[3] == 4,
         "arrange(score): expected ascending values with nulls last and stable tie order");

  const auto arranged_score_desc_nulls_first = miss.arrange({{
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

  const auto cast_to_note = df.features({"age"}).cast().note();
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
  const auto note_feature = note_df.features().front();
  const auto note_cast =
      note_df.features({note_feature})
          .cast(pluss::table::ThreadCastOptions{.series_dtype = pluss::codem::ScalarType::Float64})
          .series();
  expect(note_cast.features(note_feature).kind() == ThreadKind::Series,
         "cast(spec): Note -> Series should convert payload kind");
  expect(pluss::table::detail::has_validity_mask(note_cast.features(note_feature)),
         "cast(spec): Note -> Series should set validity mask when parse fails");
  expect(!pluss::table::detail::validity_mask(note_cast.features(note_feature))->to_vec<bool>(2),
         "cast(spec): unparsable Note value should become null");

  ReadCsvOptions timeline_opts;
  timeline_opts.ncases = 8;
  timeline_opts.usefeats = {"Updated Date"};
  const auto timeline_df = DataFrame::load_csv(fixture_path("messy_data.csv").string(), timeline_opts);
  const auto timeline_cast = timeline_df.features({"Updated Date"}).cast().note();
  expect(timeline_cast.features("Updated Date").kind() == ThreadKind::Note,
         "cast(spec): Timeline -> Note should delegate to underlying timeline string formatting");

  const auto rolling_mean = numeric.ts().rolling({{
      "age_roll_mean",
      "age",
      pluss::table::ThreadWindowOp::RollingMean,
      3,
      2,
  }},
                                          false);
  expect(rolling_mean.ncases() == numeric.ncases(), "window(keep_input=false) should preserve case count");
  expect(rolling_mean.nfeatures() == 1, "window(keep_input=false) should emit only computed features");
  expect(rolling_mean.get("age_roll_mean").has_value(), "window should create requested output feature name");
  expect(rolling_mean.features("age_roll_mean").kind() == ThreadKind::Series,
         "window output should be Series-backed");
  expect(rolling_mean.features("age_roll_mean").as_series().scalar_type() == pluss::codem::ScalarType::Float64,
         "window numeric outputs should be float64");
  expect(pluss::table::detail::has_validity_mask(rolling_mean.features("age_roll_mean")),
         "window with min_periods > 1 should produce a validity mask");
  expect(!pluss::table::detail::validity_mask(rolling_mean.features("age_roll_mean"))->to_vec<bool>(0),
         "window first row should be null when min_periods is not met");

  const auto age_i64 = numeric.features("age").as_series().to_vec<std::int64_t>();
  const auto mean_v = rolling_mean.features("age_roll_mean").as_series().to_vec<double>();
  expect(std::fabs(mean_v[1] - (static_cast<double>(age_i64[0] + age_i64[1]) / 2.0)) < 1e-9,
         "window rolling mean row1 mismatch");
  expect(std::fabs(mean_v[2] - (static_cast<double>(age_i64[0] + age_i64[1] + age_i64[2]) / 3.0)) < 1e-9,
         "window rolling mean row2 mismatch");

  const auto rolling_sum_keep_input = numeric.ts().rolling({{
      "",
      "age",
      pluss::table::ThreadWindowOp::RollingSum,
      3,
      1,
  }},
                                                       true);
  expect(rolling_sum_keep_input.nfeatures() == numeric.nfeatures() + 1,
         "rolling(keep_input=true) should append computed feature");
  expect(rolling_sum_keep_input.get("age_rollsum_3").has_value(),
         "rolling default output name should use feature/op/window policy");

  const auto score_roll_max = miss.features({"score"}).ts().rolling({{
      "score_rollmax",
      "score",
      pluss::table::ThreadWindowOp::RollingMax,
      2,
      1,
  }},
                                       false);
  const auto score_roll_max_vals = score_roll_max.features("score_rollmax").as_series().to_vec<double>();
  expect(!pluss::table::detail::has_validity_mask(score_roll_max.features("score_rollmax")),
         "rolling max(min_periods=1) should clear mask when every output row is valid");
  expect(std::fabs(score_roll_max_vals[1] - 10.0) < 1e-9,
         "rolling max should ignore null in two-row window");

  const auto score_roll_min_minp2 = miss.features({"score"}).ts().rolling({{
      "score_rollmin",
      "score",
      pluss::table::ThreadWindowOp::RollingMin,
      2,
      2,
  }},
                                              false);
  const auto score_roll_min_valid = pluss::table::detail::validity_mask(score_roll_min_minp2.features("score_rollmin"))->to_vec<bool>();
  expect(!score_roll_min_valid[1] && !score_roll_min_valid[3],
         "rolling min should be null when min_periods is not met due nulls");
}

void test_thread_and_dataframe_freq_surfaces() {
  using pluss::table::Thread;
  using pluss::table::ThreadFreqKind;
  using pluss::table::ThreadFreqOptions;

  const auto city_thread = Thread(pluss::make_note({"Accra", "Tema", "Accra", "Tema", "Accra"}));
  const auto city_freq = city_thread.freq();
  expect(city_freq.name() == "count",
         "Thread::freq() should name the default output `count`");
  expect(city_freq.size() == 2,
         "Thread::freq should return one count per distinct value");
  expect(city_freq.label().values().as_note().vector() == std::vector<std::string_view>({"Accra", "Tema"}),
         "Thread::freq should sort values by descending frequency");
  expect(city_freq.as_series().to_vec<std::int64_t>() == std::vector<std::int64_t>({3, 2}),
         "Thread::freq should count distinct values correctly");

  const auto city_prop = city_thread.freq(ThreadFreqOptions{.kind = ThreadFreqKind::Prop});
  const auto city_prop_values = city_prop.as_series().to_vec<double>();
  expect(city_prop.name() == "prop",
         "Thread::freq(prop) should rename the output `prop`");
  expect(city_prop_values.size() == 2
             && std::fabs(city_prop_values[0] - 0.6) < 1e-12
             && std::fabs(city_prop_values[1] - 0.4) < 1e-12,
         "Thread::freq(prop) should normalize counts by total mass");

  const auto first_seen_thread = Thread(pluss::make_note({"Tema", "Accra", "Tema", "Kumasi", "Accra", "Kumasi"}));
  const auto first_seen_freq = first_seen_thread.freq(ThreadFreqOptions{.sort = false});
  expect(first_seen_freq.label().values().as_note().vector()
             == std::vector<std::string_view>({"Tema", "Accra", "Kumasi"}),
         "Thread::freq(sort=false) should preserve first-seen order");

  const auto df = DataFrame({
      {"default", Thread(pluss::make_note({"yes", "yes", "yes", "no", "yes", "no"}))},
      {"is_adult", Thread(pluss::make_series(std::vector<bool>{true, true, true, true, false, false}))},
  });

  const auto counts = df.features({"default", "is_adult"}).freq();
  expect(counts.name() == "count",
         "DataFrame::freq() should name the default output `count`");
  expect(counts.as_series().to_vec<std::int64_t>() == std::vector<std::int64_t>({3, 1, 1, 1}),
         "DataFrame::freq() should count row combinations across selected features");

  std::ostringstream rendered;
  rendered << counts;
  const auto rendered_text = rendered.str();
  expect(rendered_text.find("default  is_adult") != std::string::npos,
         "DataFrame::freq() should render structured multi-feature labels as separate index headers");
  expect(rendered_text.find("yes      true") != std::string::npos,
         "DataFrame::freq() should render each structured label combination on its own row");
  expect(rendered_text.find("Name: count, ptype: Series") != std::string::npos,
         "DataFrame::freq() should preserve the Series footer");

  const auto prop = df.features({"default", "is_adult"}).freq(
      DataFrame::FreqOptions{.kind = DataFrame::FreqKind::Prop});
  const auto prop_values = prop.as_series().to_vec<double>();
  expect(prop.name() == "prop",
         "DataFrame::freq(prop) should rename the output to `prop`");
  expect(prop_values.size() == 4
             && std::fabs(prop_values[0] - 0.5) < 1e-12
             && std::fabs(prop_values[1] - (1.0 / 6.0)) < 1e-12
             && std::fabs(prop_values[2] - (1.0 / 6.0)) < 1e-12
             && std::fabs(prop_values[3] - (1.0 / 6.0)) < 1e-12,
         "DataFrame::freq(prop) should normalize counts by the total mass");

  const auto first_seen = DataFrame({
      {"city", Thread(pluss::make_note({"Tema", "Accra", "Tema", "Kumasi", "Accra", "Kumasi"}))},
      {"product", Thread(pluss::make_note({"Phone", "Phone", "Phone", "Phone", "Phone", "Phone"}))},
  }).freq(DataFrame::FreqOptions{.sort = false});
  std::ostringstream first_seen_rendered;
  first_seen_rendered << first_seen;
  const auto first_seen_text = first_seen_rendered.str();
  const auto tema_pos = first_seen_text.find("Tema");
  const auto accra_pos = first_seen_text.find("Accra");
  const auto kumasi_pos = first_seen_text.find("Kumasi");
  expect(tema_pos != std::string::npos && accra_pos != std::string::npos && kumasi_pos != std::string::npos
             && tema_pos < accra_pos && accra_pos < kumasi_pos,
         "DataFrame::freq(sort=false) should preserve first-seen group order");
}

void test_thread_bucketize_correctness() {
  const auto income = pluss::make_series(std::vector<double>{
      0.0, 0.5, 1.5, 2.0, 3.0, 4.0, 4.5, 5.9, 6.0, 6.1, 10.0, -1.0
  });
  const auto bucketed = pluss::table::Thread(income).bucketize(
      {0.0, 1.5, 3.0, 4.5, 6.0, std::numeric_limits<double>::infinity()},
      {1, 2, 3, 4, 5});

  expect(bucketed.kind() == ThreadKind::Series,
         "Thread::bucketize should return Series-backed Thread");
  expect(bucketed.as_series().scalar_type() == pluss::codem::ScalarType::Int64,
         "Thread::bucketize should return Int64 labels");
  expect(pluss::table::detail::has_validity_mask(bucketed),
         "Thread::bucketize should null out values outside configured bins");

  const auto null_count = bucketed.isnull().count().as_series().item<std::int64_t>();
  expect(null_count == 1, "Thread::bucketize should mark only out-of-range value as null");

  const auto c1 = (bucketed == 1.0).count().as_series().item<std::int64_t>();
  const auto c2 = (bucketed == 2.0).count().as_series().item<std::int64_t>();
  const auto c3 = (bucketed == 3.0).count().as_series().item<std::int64_t>();
  const auto c4 = (bucketed == 4.0).count().as_series().item<std::int64_t>();
  const auto c5 = (bucketed == 5.0).count().as_series().item<std::int64_t>();
  expect(c1 == 3 && c2 == 2 && c3 == 2 && c4 == 2 && c5 == 2,
         fmt::format("Thread::bucketize counts mismatch (c1={}, c2={}, c3={}, c4={}, c5={})",
                     c1,
                     c2,
                     c3,
                     c4,
                     c5));

  const auto total_assigned = bucketed.count().as_series().item<std::int64_t>();
  expect(total_assigned == 11, "Thread::bucketize assigned count mismatch");

  const auto lower_open = pluss::table::Thread(income).bucketize(
      {0.0, 1.5, 3.0, 4.5, 6.0, std::numeric_limits<double>::infinity()},
      {1, 2, 3, 4, 5},
      true,
      false);
  const auto lower_open_c1 = (lower_open == 1.0).count().as_series().item<std::int64_t>();
  expect(lower_open_c1 == 2,
         "Thread::bucketize(include_lowest=false) should exclude value exactly on first left edge");

  using pluss::table::DataFrame;
  const auto scores = DataFrame({
      {"income", pluss::table::Thread(income)},
      {"bonus", pluss::table::Thread(pluss::make_series(std::vector<double>{
                    1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0
                }))},
      {"city", pluss::table::Thread(pluss::make_note({
                   "A", "A", "B", "B", "C", "C", "D", "D", "E", "E", "F", "F"
               }))},
  });

  const auto selected_features = std::array<std::string_view, 2>{"income", "bonus"};
  const auto df_bucketed = scores.features(
      pluss::msharp::RangeView::Of<std::string_view>(
          std::span<const std::string_view>(selected_features.data(), selected_features.size())))
                               .bucketize(
      {0.0, 1.5, 3.0, 4.5, 6.0, std::numeric_limits<double>::infinity()},
      {1, 2, 3, 4, 5});
  expect(df_bucketed.features("income").as_series().scalar_type() == pluss::codem::ScalarType::Int64,
         "DataFrame::bucketize should replace selected numeric features with Int64 bucket labels");
  expect(df_bucketed.features("income").isnull().count().as_series().item<std::int64_t>() == 1,
         "DataFrame::bucketize should preserve out-of-range values as null");
  expect((df_bucketed.features("bonus") == 1.0).count().as_series().item<std::int64_t>() == 1,
         "DataFrame::bucketize should bucketize every selected numeric feature independently");
  expect(df_bucketed.features("city").as_note().vector() == scores.features("city").as_note().vector(),
         "DataFrame::bucketize should leave unselected features unchanged");

  const auto auto_bucketed = scores.bucketize(
      {0.0, 1.5, 3.0, 4.5, 6.0, std::numeric_limits<double>::infinity()},
      {1, 2, 3, 4, 5});
  expect(auto_bucketed.features("income").as_series().scalar_type() == pluss::codem::ScalarType::Int64,
         "DataFrame::bucketize(all) should auto-select numeric Series features");
  expect(auto_bucketed.features("bonus").as_series().scalar_type() == pluss::codem::ScalarType::Int64,
         "DataFrame::bucketize(all) should bucketize every numeric Series feature");
}

void test_replace_exact_and_null_overloads() {
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const auto numeric = Thread(pluss::make_series(std::vector<double>{2.0, 0.0, 2.0, 4.0}));
  const auto numeric_replaced = numeric.replace(2.0, 5.0);
  expect(numeric_replaced.as_series().to_vec<double>() == std::vector<double>({5.0, 0.0, 5.0, 4.0}),
         "Thread::replace(double, double) should replace exact numeric matches");

  const auto numeric_put = numeric.put(numeric == 2.0, 9.0);
  expect(numeric_put.as_series().to_vec<double>() == std::vector<double>({9.0, 0.0, 9.0, 4.0}),
         "Thread::put(mask, double) should assign into masked numeric rows");

  const auto numeric_nulled = numeric.replace(0.0, pluss::null);
  expect(pluss::table::detail::has_validity_mask(numeric_nulled), "Thread::replace(double, null) should create a validity mask");
  expect(pluss::table::detail::validity_mask(numeric_nulled)->to_vec<bool>() == std::vector<bool>({true, false, true, true}),
         "Thread::replace(double, null) should null-mark exact numeric matches");

  const auto numeric_refilled = numeric_nulled.replace(pluss::null, 9.0);
  expect(!pluss::table::detail::has_validity_mask(numeric_refilled), "Thread::replace(null, double) should resolve null mask when fully filled");
  expect(numeric_refilled.as_series().to_vec<double>() == std::vector<double>({2.0, 9.0, 2.0, 4.0}),
         "Thread::replace(null, double) should fill numeric null rows");

  const auto name_thread = Thread(pluss::make_note({"Laurel", "Laurel Kirk", "Kojo"}));
  const auto exact_names = name_thread.replace(pluss::make_note("Laurel"), pluss::make_note("L"));
  expect(std::string(exact_names.as_note().at(0)) == "L",
         "Thread::replace(Note, Note) should replace exact whole-cell Note matches");
  expect(std::string(exact_names.as_note().at(1)) == "Laurel Kirk",
         "Thread::replace(Note, Note) should not perform substring replacement");

  const auto named_put = name_thread.put(name_thread == "Laurel", "GPT");
  expect(std::string(named_put.as_note().at(0)) == "GPT",
         "Thread::put(mask, string) should assign into masked Note rows");
  expect(std::string(named_put.as_note().at(1)) == "Laurel Kirk",
         "Thread::put(mask, string) should preserve unmasked Note rows");

  const auto substring_names = name_thread.replace("Laurel", "L");
  expect(std::string(substring_names.as_note().at(1)) == "L Kirk",
         "Thread::replace(string, string) should preserve existing substring semantics");

  auto ts_valid = pluss::Series::zeros({3}, pluss::codem::ScalarType::Bool);
  auto* ts_valid_ptr = reinterpret_cast<bool*>(ts_valid.raw_data());
  ts_valid_ptr[0] = true;
  ts_valid_ptr[1] = false;
  ts_valid_ptr[2] = true;
  const auto timeline_thread = Thread(
      pluss::make_timeline({
          "2024-01-01T00:00:00Z",
          "2024-01-02T00:00:00Z",
          "2024-01-03T00:00:00Z",
      }),
      ts_valid);
  const auto ts_fill = pluss::make_timeline("2024-01-31T00:00:00Z");
  const auto timeline_refilled = timeline_thread.replace(pluss::null, ts_fill);
  expect(!pluss::table::detail::has_validity_mask(timeline_refilled),
         "Thread::replace(null, Timeline) should resolve timeline null rows");
  expect(timeline_refilled.as_timeline().to_epoch_nanos_vector()[1] == ts_fill.to_epoch_nanos_vector()[0],
         "Thread::replace(null, Timeline) should fill timeline null rows");

  const auto gradebook = DataFrame({
      {"Midterm 1", Thread(pluss::make_series(std::vector<double>{2.0, 0.0, 3.0}))},
      {"Midterm 2", Thread(pluss::make_series(std::vector<double>{0.0, 2.0, 0.0}))},
      {"Name", Thread(pluss::make_note({"Laurel", "Laurel Kirk", "Kojo"}))},
  }).title("gradebook");

  const auto all_numeric_replaced = gradebook.replace(2.0, 5.0);
  expect(all_numeric_replaced.features("Midterm 1").as_series().to_vec<double>()
             == std::vector<double>({5.0, 0.0, 3.0})
             && all_numeric_replaced.features("Midterm 2").as_series().to_vec<double>()
                    == std::vector<double>({0.0, 5.0, 0.0}),
         "DataFrame::replace without features should dispatch to every compatible column");
  expect(std::string(all_numeric_replaced.features("Name").as_note().at(0)) == "Laurel",
         "DataFrame::replace without features should preserve incompatible columns");
  expect(all_numeric_replaced.title() == "gradebook",
         "DataFrame::replace should preserve the DataFrame title");

  const auto mid_replaced = gradebook.replace(2.0, 5.0, {"Midterm 1", "Midterm 2"});
  expect(mid_replaced.features("Midterm 1").as_series().to_vec<double>() == std::vector<double>({5.0, 0.0, 3.0}),
         "DataFrame::replace(double, double, features) should replace exact matches in selected numeric features");
  expect(mid_replaced.features("Midterm 2").as_series().to_vec<double>() == std::vector<double>({0.0, 5.0, 0.0}),
         "DataFrame::replace(double, double, features) should apply to every selected numeric feature");
  expect(std::string(mid_replaced.features("Name").as_note().at(0)) == "Laurel",
         "DataFrame::replace(double, double, features) should not mutate unselected Note features");

  const auto mid_nulled = gradebook.replace(0.0, pluss::null, {"Midterm 1", "Midterm 2"});
  expect(pluss::table::detail::validity_mask(mid_nulled.features("Midterm 1"))->to_vec<bool>() == std::vector<bool>({true, false, true}),
         "DataFrame::replace(double, null, features) should null-mark exact matches in selected numeric features");
  expect(pluss::table::detail::validity_mask(mid_nulled.features("Midterm 2"))->to_vec<bool>() == std::vector<bool>({false, true, false}),
         "DataFrame::replace(double, null, features) should handle multiple selected numeric features");

  const auto name_replaced = gradebook.replace("Laurel", "L", {"Name"});
  expect(std::string(name_replaced.features("Name").as_note().at(0)) == "L",
         "DataFrame::replace(string, string, features) should replace exact Note values");
  expect(std::string(name_replaced.features("Name").as_note().at(1)) == "Laurel Kirk",
         "DataFrame::replace(string, string, features) should not perform substring replacement");

  const auto typed_numeric = gradebook.replace(
      pluss::make_series(std::vector<double>{3.0}),
      pluss::make_series(std::vector<double>{7.0}),
      {"Midterm 1"});
  expect(typed_numeric.features("Midterm 1").as_series().to_vec<double>()
             == std::vector<double>({2.0, 0.0, 7.0}),
         "DataFrame::replace(Series, Series) should delegate typed scalar replacement");

  const auto typed_name = gradebook.replace(
      pluss::make_note("Kojo"),
      pluss::make_note("Kofi"),
      {"Name"});
  expect(std::string(typed_name.features("Name").as_note().at(2)) == "Kofi",
         "DataFrame::replace(Note, Note) should delegate exact Note replacement");

  const auto name_nulled = gradebook.replace(pluss::make_note("Kojo"), pluss::null, {"Name"});
  expect(pluss::table::detail::has_validity_mask(name_nulled.features("Name"))
             && !pluss::table::detail::validity_mask(name_nulled.features("Name"))->to_vec<bool>(2),
         "DataFrame::replace(Note, null) should delegate Note nullification");
  const auto name_refilled = name_nulled.replace(pluss::null, pluss::make_note("Ama"), {"Name"});
  expect(!pluss::table::detail::has_validity_mask(name_refilled.features("Name"))
             && std::string(name_refilled.features("Name").as_note().at(2)) == "Ama",
         "DataFrame::replace(null, Note) should delegate Note filling");
  expect(name_refilled.title() == "gradebook",
         "DataFrame::replace(null, Note) should preserve the DataFrame title");

  const auto event_times = pluss::make_timeline({
      "2024-01-01T00:00:00Z",
      "2024-01-02T00:00:00Z",
      "2024-01-03T00:00:00Z",
  });
  const auto indexed_times = pluss::make_timeindex({
      "2024-01-01T00:00:00Z",
      "2024-01-02T00:00:00Z",
      "2024-01-03T00:00:00Z",
  });
  const auto temporal = DataFrame({
      {"event", event_times},
      {"indexed", indexed_times},
      {"label", pluss::make_note({"a", "b", "c"})},
  }).title("temporal");
  const auto old_time = pluss::make_timeline("2024-01-02T00:00:00Z");
  const auto new_time = pluss::make_timeline("2024-01-09T00:00:00Z");
  const auto temporal_replaced = temporal.replace(old_time, new_time);
  const auto expected_tick = new_time.to_epoch_nanos_vector()[0];
  expect(temporal_replaced.features("event").as_timeline().to_epoch_nanos_vector()[1] == expected_tick,
         "DataFrame::replace(Timeline, Timeline) should update Timeline columns");
  expect(temporal_replaced.features("indexed").as_timeindex().timeline().to_epoch_nanos_vector()[1]
             == expected_tick,
         "DataFrame::replace(Timeline, Timeline) should update TimeIndex columns");
  expect(temporal_replaced.features("event").kind() == ThreadKind::Timeline
             && temporal_replaced.features("indexed").kind() == ThreadKind::TimeIndex,
         "DataFrame::replace should preserve temporal payload kinds");
  expect(temporal_replaced.title() == "temporal",
         "DataFrame temporal replacement should preserve the DataFrame title");

  const auto temporal_nulled = temporal.replace(old_time, pluss::null);
  expect(!pluss::table::detail::validity_mask(temporal_nulled.features("event"))->to_vec<bool>(1)
             && !pluss::table::detail::validity_mask(temporal_nulled.features("indexed"))->to_vec<bool>(1),
         "DataFrame::replace(Timeline, null) should nullify matching temporal values");

  const auto old_index = pluss::make_timeindex("2024-01-01T00:00:00Z");
  const auto new_index = pluss::make_timeindex("2024-01-08T00:00:00Z");
  const auto index_replaced = temporal.replace(old_index, new_index);
  expect(index_replaced.features("indexed").as_timeindex().timeline().to_epoch_nanos_vector()[0]
             == new_index.timeline().to_epoch_nanos_vector()[0],
         "DataFrame::replace(TimeIndex, TimeIndex) should update TimeIndex columns");
  expect(index_replaced.features("event").as_timeline().to_epoch_nanos_vector()
             == temporal.features("event").as_timeline().to_epoch_nanos_vector(),
         "DataFrame::replace(TimeIndex, TimeIndex) should preserve Timeline columns");

  expect_throw([&]() { (void)gradebook.replace(1.0, 2.0, {"missing"}); },
               "DataFrame::replace should reject unknown selected features");
}

void test_select_and_place_helpers() {
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const auto cond0 = pluss::make_series(std::vector<bool>{true, false, false, false});
  const auto cond1 = pluss::make_series(std::vector<bool>{false, true, false, false});
  const auto cond2 = pluss::make_series(std::vector<bool>{false, false, true, false});
  const auto pick0 = pluss::make_series(std::vector<double>{10.0, 10.0, 10.0, 10.0});
  const auto pick1 = pluss::make_series(std::vector<double>{20.0, 20.0, 20.0, 20.0});
  const auto pick2 = pluss::make_series(std::vector<double>{30.0, 30.0, 30.0, 30.0});

  const auto selected = pluss::select({cond0, cond1, cond2}, {pick0, pick1, pick2}, -1.0);
  expect(selected.to_vec<double>() == std::vector<double>({10.0, 20.0, 30.0, -1.0}),
         "pluss::select should choose branch values by the first matching boolean condition");

  const auto thread_selected = Thread::select(
      {Thread(cond0), Thread(cond1), Thread(cond2)},
      {Thread(pick0), Thread(pick1), Thread(pick2)},
      -1.0);
  expect(thread_selected.as_series().to_vec<double>() == std::vector<double>({10.0, 20.0, 30.0, -1.0}),
         "Thread::select should delegate multi-branch numeric selection to pluss::select");

  expect_throw([&]() { (void)Thread(pluss::make_note("A+")); },
               "Thread should reject scalar Note payloads and preserve its strict 1D contract");

  const auto letter_selected = Thread::select(
      {Thread(cond0), Thread(cond1), Thread(cond2)},
      {pluss::make_note({"A+"}), pluss::make_note({"B"}), pluss::make_note({"C"})},
      pluss::null);
  expect(letter_selected.kind() == ThreadKind::Note,
         "Thread::select should accept explicit length-1 Note choices");
  expect(letter_selected.as_note().vector() == std::vector<std::string_view>({"A+", "B", "C", ""}),
         "Thread::select should broadcast explicit length-1 Note choices across matching rows");
  expect(pluss::table::detail::validity_mask(letter_selected)->to_vec<bool>() == std::vector<bool>({true, true, true, false}),
         "Thread::select should preserve null default when selecting explicit length-1 Note choices");

  const auto thread_selected_null = Thread::select(
      {Thread(cond0), Thread(cond1), Thread(cond2)},
      {Thread(pick0), Thread(pick1), Thread(pick2)},
      pluss::null);
  expect(pluss::table::detail::has_validity_mask(thread_selected_null),
         "Thread::select(null) should mark unmatched rows as null");
  const auto thread_selected_null_valid = pluss::table::detail::validity_mask(thread_selected_null)->to_vec<bool>();
  expect(thread_selected_null_valid == std::vector<bool>({true, true, true, false}),
         fmt::format(
             "Thread::select(null) should preserve validity only on matched rows; got [{}, {}, {}, {}]",
             static_cast<int>(thread_selected_null_valid[0]),
             static_cast<int>(thread_selected_null_valid[1]),
             static_cast<int>(thread_selected_null_valid[2]),
             static_cast<int>(thread_selected_null_valid[3])));

  const auto row_labels = Thread(pluss::make_note({"r0", "r1", "r2", "r3"}));
  const auto branch0 = DataFrame({
      {"score", Thread(pick0)},
      {"grade", pluss::make_note({"A", "A", "A", "A"})},
  }, row_labels);
  const auto branch1 = DataFrame({
      {"score", Thread(pick1)},
      {"grade", pluss::make_note({"B", "B", "B", "B"})},
  }, row_labels);
  const auto branch2 = DataFrame({
      {"score", Thread(pick2)},
      {"grade", pluss::make_note({"C", "C", "C", "C"})},
  }, row_labels);
  const auto default_branch = DataFrame({
      {"score", Thread(pluss::make_series(std::vector<double>{-1.0, -1.0, -1.0, -1.0}))},
      {"grade", pluss::make_note({"NA", "NA", "NA", "NA"})},
  }, row_labels);

  const auto frame_selected = DataFrame::select(
      {Thread(cond0), Thread(cond1), Thread(cond2)},
      {branch0, branch1, branch2},
      default_branch);
  expect(frame_selected.features("score").as_series().to_vec<double>() ==
             std::vector<double>({10.0, 20.0, 30.0, -1.0}) &&
             frame_selected.features("grade").as_note().vector() ==
                 std::vector<std::string_view>({"A", "B", "C", "NA"}),
         "DataFrame::select should delegate multi-branch feature selection through Thread::select");
  expect(frame_selected.label().values().as_note().vector() ==
             std::vector<std::string_view>({"r0", "r1", "r2", "r3"}),
         "DataFrame::select should preserve default case labels");

  const auto inventory = Thread(pluss::make_series(std::vector<double>{5.0, 6.0}));
  const auto price = Thread(pluss::make_series(std::vector<double>{3.0, 4.0}));
  const auto tips = Thread(pluss::make_series(std::vector<double>{1.0, 2.0}));
  const auto sales = DataFrame({
      {"price", price},
      {"inventory", inventory},
      {"id", Thread(pluss::make_note({"A", "B"}))},
  });

  const auto placed = sales.place("tip", tips, "inventory");
  expect(placed.features() == std::vector<std::string>({"price", "tip", "inventory", "id"}),
         "DataFrame::place(name, feature, anchor) should insert new features before the anchor by default");

  const auto moved = placed.place("id", "price");
  expect(moved.features() == std::vector<std::string>({"id", "price", "tip", "inventory"}),
         "DataFrame::place(name, anchor) should move an existing feature before the anchor");

  const auto reordered = moved.reorder({"id", "price", "tip", "inventory"});
  expect(reordered.features() == std::vector<std::string>({"id", "price", "tip", "inventory"}),
         "DataFrame::reorder should honor an explicit full feature order");

  const auto partial_front = sales.reorder({"id"});
  expect(partial_front.features() == std::vector<std::string>({"id", "price", "inventory"}),
         "DataFrame::reorder(partial) should move selected features to the front and preserve the rest");

  const auto partial_back = sales.reorder(
      {.features = {"id"}, .placement = DataFrame::ReorderPlacement::Back});
  expect(partial_back.features() == std::vector<std::string>({"price", "inventory", "id"}),
         "DataFrame::reorder(back) should move selected features to the back");

  const auto partial_before = sales.reorder(
      {.features = {"id"}, .placement = DataFrame::ReorderPlacement::Before, .anchor = "inventory"});
  expect(partial_before.features() == std::vector<std::string>({"price", "id", "inventory"}),
         "DataFrame::reorder(before) should insert the moved block before the anchor and preserve other order");

  const auto partial_after = sales.reorder(
      {.features = {"price", "id"}, .placement = DataFrame::ReorderPlacement::After, .anchor = "inventory"});
  expect(partial_after.features() == std::vector<std::string>({"inventory", "price", "id"}),
         "DataFrame::reorder(after) should insert the moved block after the anchor with stable moved order");
}

void test_series_nan_participates_in_table_missing_semantics() {
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const auto nan = std::numeric_limits<double>::quiet_NaN();
  const auto raw = pluss::make_series(std::vector<double>{1.0, nan, 3.0});
  const auto thread = Thread(raw);

  expect(thread.isnull().as_series().to_vec<bool>() == std::vector<bool>({false, true, false}),
         "Thread::isnull should treat floating NaN as missing for Series payloads");
  expect(thread.notnull().as_series().to_vec<bool>() == std::vector<bool>({true, false, true}),
         "Thread::notnull should mark floating NaN as missing for Series payloads");

  const auto filled = thread.fillnull(9.0);
  expect(filled.as_series().to_vec<double>() == std::vector<double>({1.0, 9.0, 3.0}),
         "Thread::fillnull should fill floating NaN values even without an explicit validity mask");
  expect(!pluss::table::detail::has_validity_mask(filled),
         "Thread::fillnull should clear missingness after filling floating NaN values");

  const auto df = DataFrame({{"score", thread}});
  const auto null_mask = df.isnull({"score"}).features("score").as_series().to_vec<bool>();
  expect(null_mask == std::vector<bool>({false, true, false}),
         "DataFrame::isnull should treat floating NaN as missing through Thread delegation");

  const auto dropped = df.dropnull({"score"}, true);
  expect(dropped.ncases() == 2,
         "DataFrame::dropnull(..., all=true) should drop rows whose selected numeric features are NaN");

  const auto summed = df.sum(Axis::Case).as_series().to_vec<double>();
  expect(summed == std::vector<double>({4.0}),
         "DataFrame::sum(skipnull=true) should skip floating NaN values in numeric reductions");

  const auto csv_path = write_temp_csv("nan_missing_export", "");
  df.save_csv(csv_path.string());
  std::ifstream in(csv_path, std::ios::in | std::ios::binary);
  expect(in.is_open(), "failed to reopen saved csv for NaN missing export test");
  std::string saved((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  expect(saved == "score\n1\n\n3\n",
         fmt::format("save_csv should serialize floating NaN as the default blank missing marker; got [{}]", saved));
}

void test_dataframe_shownull_and_zoom() {
  using pluss::table::Axis;
  using pluss::table::DataFrame;

  const auto nan = std::numeric_limits<double>::quiet_NaN();

  const auto row_source = DataFrame({
      {"a", pluss::make_series(std::vector<double>{1.0, nan, 3.0, nan})},
      {"b", pluss::make_series(std::vector<double>{10.0, 11.0, nan, nan})},
      {"c", pluss::make_series(std::vector<double>{100.0, 101.0, 102.0, nan})},
  });

  const auto null_rows_any = row_source.shownull();
  expect(null_rows_any.ncases() == 3 && null_rows_any.nfeatures() == row_source.nfeatures(),
         "DataFrame::shownull() should keep rows with at least one null by default");
  expect(null_rows_any.isnull({"a"}).features("a").as_series().to_vec<bool>() ==
             std::vector<bool>({true, false, true}),
         "DataFrame::shownull(Axis::Case) should preserve row order for null-containing rows");

  const auto null_rows_all = row_source.shownull(Axis::Case, true);
  expect(null_rows_all.ncases() == 1,
         "DataFrame::shownull(Axis::Case, all=true) should keep only rows whose selected cells are all null");
  expect(null_rows_all.isnull().all(Axis::Case).as_series().to_vec<bool>() == std::vector<bool>({true}),
         "DataFrame::shownull(Axis::Case, all=true) should keep the fully-null row");

  const auto feature_source = DataFrame({
      {"clean", pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0})},
      {"some_null", pluss::make_series(std::vector<double>{5.0, nan, 7.0, 8.0})},
      {"all_null", pluss::make_series(std::vector<double>{nan, nan, nan, nan})},
  });

  const auto null_features_any = feature_source.shownull(Axis::Feature);
  expect(null_features_any.features() == std::vector<std::string>({"some_null", "all_null"}),
         "DataFrame::shownull(Axis::Feature) should keep features with at least one null");

  const auto null_features_all = feature_source.shownull(Axis::Feature, true);
  expect(null_features_all.features() == std::vector<std::string>({"all_null"}),
         "DataFrame::shownull(Axis::Feature, all=true) should keep only fully-null features");

  const auto grid = DataFrame({
      {"w", pluss::make_series(std::vector<std::int64_t>{1, 2, 3, 4})},
      {"x", pluss::make_series(std::vector<std::int64_t>{5, 6, 7, 8})},
      {"y", pluss::make_series(std::vector<std::int64_t>{9, 10, 11, 12})},
      {"z", pluss::make_series(std::vector<std::int64_t>{13, 14, 15, 16})},
  }).label().set(pluss::make_note({"r0", "r1", "r2", "r3"}));

  const auto zoomed = grid.zoom(1, 3, 1, 4);
  expect(zoomed.ncases() == 2 && zoomed.nfeatures() == 3,
         "DataFrame::zoom should return the requested rectangular half-open block");
  expect(zoomed.features() == std::vector<std::string>({"x", "y", "z"}),
         "DataFrame::zoom should project the requested feature block");
  expect(zoomed.features("y").as_series().to_vec<std::int64_t>() == std::vector<std::int64_t>({10, 11}),
         "DataFrame::zoom should preserve the values inside the carved block");
  expect(zoomed.label().values().as_note().vector() == std::vector<std::string>({"r1", "r2"}),
         "DataFrame::zoom should preserve the matching case labels for the carved block");
}

void test_thread_arrange_and_corr_stability() {
  const auto values = pluss::make_series(std::vector<std::int64_t>{3, 99, 1, 2});
  const auto valid = pluss::make_series(std::vector<bool>{true, false, true, true});
  const auto thread = pluss::table::Thread(values, valid);

  const auto asc = thread.arrange(true, true);
  const auto asc_vals = asc.as_series().to_vec<std::int64_t>();
  const auto asc_valid = pluss::table::detail::validity_mask(asc)->to_vec<bool>();
  expect(asc_vals == std::vector<std::int64_t>({1, 2, 3, 99}),
         "Thread::arrange(ascending) values mismatch");
  expect(asc_valid == std::vector<bool>({true, true, true, false}),
         "Thread::arrange(ascending) validity ordering mismatch");

  const auto desc_nulls_first = thread.arrange(false, false);
  const auto desc_vals = desc_nulls_first.as_series().to_vec<std::int64_t>();
  const auto desc_valid = pluss::table::detail::validity_mask(desc_nulls_first)->to_vec<bool>();
  expect(desc_vals == std::vector<std::int64_t>({99, 3, 2, 1}),
         "Thread::arrange(descending, nulls_first) values mismatch");
  expect(desc_valid == std::vector<bool>({false, true, true, true}),
         "Thread::arrange(descending, nulls_first) validity ordering mismatch");

  constexpr std::int64_t n = 20000;
  std::vector<double> x;
  std::vector<double> y;
  x.reserve(static_cast<std::size_t>(n));
  y.reserve(static_cast<std::size_t>(n));
  for (std::int64_t i = 0; i < n; ++i) {
    const double xv = 1.0e16 + static_cast<double>(i);
    const double yv = 2.0e16 + 3.0 * static_cast<double>(i)
        + (static_cast<double>(i % 7) - 3.0) * 0.01;
    x.push_back(xv);
    y.push_back(yv);
  }

  const auto lhs = pluss::table::Thread(pluss::make_series(x));
  const auto rhs = pluss::table::Thread(pluss::make_series(y));
  const auto actual = lhs.corr(rhs);
  expect(std::isfinite(actual), "Thread::corr should remain finite for large-offset data");
  expect(actual > 0.99,
         fmt::format("Thread::corr should remain strongly positive on monotone data (actual={})", actual));
}

void test_corr_feature_projection_and_sort_chain() {
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const auto df = DataFrame({
      {"x", Thread(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}))},
      {"y", Thread(pluss::make_series(std::vector<double>{2.0, 4.0, 6.0, 8.0}))},
      {"z", Thread(pluss::make_series(std::vector<double>{4.0, 3.0, 2.0, 1.0}))},
  });

  const auto corr = df.corr();
  expect(corr.nfeatures() == 3 && corr.ncases() == 3, "corr() should produce a square 3x3 matrix");

  const auto sorted_target = corr.features("x").arrange(/*ascending=*/false);
  const auto values = sorted_target.as_series().to_vec<double>();
  expect(values.size() == 3, "corr().features(target) should project one 1D thread");
  expect(values[0] >= values[1] && values[1] >= values[2],
         "arrange(descending) should sort projected correlation values");
  expect(std::fabs(values[0] - 1.0) < 1e-12 && std::fabs(values[2] + 1.0) < 1e-12,
         "projected sorted correlation values should match expected extrema");

  const auto rendered = sorted_target.to_string();
  const auto x_pos = rendered.find("x");
  const auto y_pos = rendered.find("y");
  const auto z_pos = rendered.find("z");
  expect(x_pos != std::string::npos && y_pos != std::string::npos && z_pos != std::string::npos,
         "rendered sorted projected corr thread should include feature-name labels");
  expect(x_pos < y_pos && y_pos < z_pos,
         "rendered sorted projected corr labels should follow descending correlation order");
}

void test_timeline_sort_nat_ordering() {
  using pluss::codeh::Timeline;

  const auto ticks = pluss::make_series(
      std::vector<std::int64_t>{
          30'000'000'000LL,
          Timeline::kNaT,
          10'000'000'000LL,
          Timeline::kNaT,
          20'000'000'000LL,
      },
      {5});
  const auto t = Timeline::fromSeries(ticks);

  expect(t.argsort().to_vec<std::int64_t>() == std::vector<std::int64_t>({2, 4, 0, 1, 3}),
         "Timeline::argsort should keep NaT values at the end while preserving NaT order");
  expect(t.sort().to_epoch_nanos_flat() == std::vector<std::int64_t>({
             10'000'000'000LL,
             20'000'000'000LL,
             30'000'000'000LL,
             Timeline::kNaT,
             Timeline::kNaT,
         }),
         "Timeline::sort should keep NaT values at the end");
}

void test_atlas_label_accessor() {
  const auto f0 = pluss::table::DataFrame({
      {"sales", pluss::table::Thread(pluss::make_series(std::vector<double>{10.0, 20.0}))},
  });
  const auto f1 = pluss::table::DataFrame({
      {"sales", pluss::table::Thread(pluss::make_series(std::vector<double>{30.0, 40.0}))},
  });
  const auto labels = pluss::table::Thread(pluss::make_series(std::vector<std::int64_t>{0, 1}));
  const auto atlas = pluss::table::Atlas({f0, f1}, labels);

  expect(atlas.label().has(), "Atlas::label().has should report explicit depth labels");
  expect(atlas.label().values().as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1}),
         "Atlas::label().values should expose typed depth labels");
  expect(atlas.label().find(pluss::table::Thread(pluss::make_series(std::vector<std::int64_t>{1, 9})))
             .as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({1, -1}),
         "Atlas::label().find should delegate to Thread label indexing");
  expect(atlas.label().unite(pluss::table::Thread(pluss::make_series(std::vector<std::int64_t>{1, 2})))
             .as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1, 2}),
         "Atlas::label().unite should preserve typed numeric depth labels");
  expect(atlas.label().pos(static_cast<double>(1)).as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({1}),
         "Atlas::label().pos should return depth positions through Thread label indexing");

  const auto relabeled = atlas.label().set(
      pluss::table::Thread(pluss::make_series(std::vector<std::int64_t>{10, 11})));
  expect(relabeled.loc(static_cast<std::int64_t>(11)).features("sales").as_series().to_vec<double>() ==
             std::vector<double>({30.0, 40.0}),
         "Atlas::label().set should replace depth labels");

  const auto aligned = atlas.label().align(
      pluss::table::Thread(pluss::make_series(std::vector<std::int64_t>{1, 0})));
  expect(aligned.dloc(0).features("sales").as_series().to_vec<double>() ==
             std::vector<double>({30.0, 40.0}) &&
             aligned.label().values().as_series().to_vec<std::int64_t>() ==
                 std::vector<std::int64_t>({1, 0}),
         "Atlas::label().align should reorder frames and carry requested depth labels");
  expect(atlas.label().range().label().values().as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1}),
         "Atlas::label().range should materialize typed range depth labels");
  expect(!atlas.label().reset().label().has(),
         "Atlas::label().reset should clear explicit depth labels");
}

void test_nexus_basic_workspace_ops() {
  const auto loan = DataFrame({
      {"loan_id", pluss::make_series(std::vector<std::int64_t>{1, 2})},
      {"account_id", pluss::make_series(std::vector<std::int64_t>{10, 11})},
  });
  const auto account = DataFrame({
      {"account_id", pluss::make_series(std::vector<std::int64_t>{10, 11, 12})},
      {"district_id", pluss::make_series(std::vector<std::int64_t>{1, 1, 2})},
  });
  const auto disp = DataFrame({
      {"disp_id", pluss::make_series(std::vector<std::int64_t>{100, 101, 102})},
      {"account_id", pluss::make_series(std::vector<std::int64_t>{10, 11, 12})},
      {"client_id", pluss::make_series(std::vector<std::int64_t>{501, 502, 503})},
  });

  const auto nx = pluss::bundle::Nexus({
      {"loan", loan},
      {"account", account},
      {"disp", disp},
  });

  expect(nx.defined(), "Nexus should be defined after named construction");
  expect(nx.ntables() == 3, "Nexus should track table count");
  expect(nx.keys() == std::vector<std::string>({"loan", "account", "disp"}),
         "Nexus should preserve named insertion order");
  expect(nx.names() == nx.keys(), "Nexus::names should alias keys");
  expect(nx.frame("loan").nfeatures() == 2, "Nexus::frame(key) should fetch named tables");
  expect(nx.frame(1).ncases() == 3, "Nexus::frame(index) should fetch ordered tables");
  expect(!nx.get("client").has_value(), "Nexus::get should return nullopt for missing names");

  const auto with_named = nx.inject("client", DataFrame({
      {"client_id", pluss::make_series(std::vector<std::int64_t>{501, 502})},
      {"district_id", pluss::make_series(std::vector<std::int64_t>{1, 2})},
  }));
  expect(with_named.keys().back() == "client", "Nexus::inject(name, frame) should append named tables");
  expect(!nx.has("client") && nx.ntables() == 3,
         "Nexus non-mutating inject should not mutate the source workspace");

  const auto with_auto = with_named.inject(DataFrame({
      {"district_id", pluss::make_series(std::vector<std::int64_t>{1, 2, 3})},
      {"name", pluss::make_note({"A", "B", "C"})},
  }));
  expect(with_auto.keys().back() == "table_0", "Nexus unnamed inject should use deterministic auto names");

  const auto with_many = with_auto.inject({
      DataFrame({{"x", pluss::make_series(std::vector<std::int64_t>{1})}}),
      DataFrame({{"y", pluss::make_series(std::vector<std::int64_t>{2})}}),
  });
  expect(with_many.keys()[5] == "table_1" && with_many.keys()[6] == "table_2",
         "Nexus multi-inject should continue the auto-name sequence");

  const auto renamed = with_many.rename("disp", "assignment");
  expect(renamed.has("assignment") && !renamed.has("disp"), "Nexus::rename should replace the key");
  expect(with_many.has("disp") && !with_many.has("assignment"),
         "Nexus non-mutating rename should leave the source workspace unchanged");

  const auto selected = renamed.select({"loan", "assignment"});
  expect(selected.keys() == std::vector<std::string>({"loan", "assignment"}),
         "Nexus::select should preserve requested order");

  const auto dropped = renamed.drop({"table_1", "table_2"});
  expect(dropped.ntables() == renamed.ntables() - 2 && !dropped.has("table_1"),
         "Nexus::drop should remove named tables");

  const auto summary = nx.summary();
  expect(summary.ncases() == 3 && summary.nfeatures() == 3,
         "Nexus::summary should expose one row per table with core size columns");
  expect(summary.features("table").as_note().at(0) == "loan",
         "Nexus::summary should include table names in order");
}

void test_nexus_delegation_and_atlas_interop() {
  const auto weekday = DataFrame({
      {"city", pluss::make_note({"Edmonton", "Calgary"})},
      {"temp", pluss::make_series(std::vector<double>{12.0, 15.0})},
  });
  const auto weekend = DataFrame({
      {"city", pluss::make_note({"Banff", "Jasper"})},
      {"temp", pluss::make_series(std::vector<double>{8.0, 6.0})},
  });

  const auto nx = pluss::bundle::Nexus({
      {"weekday", weekday},
      {"weekend", weekend},
  });

  const auto narrowed = nx.apply("weekday", [](const DataFrame& frame) {
    return frame.features({"city"});
  });
  expect(narrowed.frame("weekday").nfeatures() == 1 && narrowed.frame("weekend").nfeatures() == 2,
         "Nexus::apply should delegate to a single DataFrame");

  const auto mapped = nx.map_frames([](const DataFrame& frame) {
    return frame.features({"city"});
  });
  expect(mapped.frame("weekday").nfeatures() == 1 && mapped.frame("weekend").nfeatures() == 1,
         "Nexus::map_frames should delegate across all tables");

  const auto atlas = nx.to_atlas({"weekday", "weekend"});
  expect(atlas.depth() == 2, "Nexus::to_atlas should materialize a same-schema workspace subset");
  expect(atlas.label().values().as_note().at(0) == "weekday",
         "Nexus::to_atlas should default depth labels from table names");

  const auto roundtrip = pluss::bundle::Nexus::from_atlas(atlas);
  expect(roundtrip.keys() == std::vector<std::string>({"weekday", "weekend"}),
         "Nexus::from_atlas should recover named tables from depth labels");
  expect(roundtrip.frame("weekday").equals(weekday) && roundtrip.frame("weekend").equals(weekend),
         "Nexus Atlas roundtrip should preserve frame contents");

  const auto flowed = nx.flow([](const pluss::bundle::Nexus& value) {
    return value.head(1);
  });
  expect(flowed.ntables() == 1 && flowed.keys().front() == "weekday",
         "Nexus::flow should support chainable workspace callbacks");
}

void test_nexus_metadata_and_relationship_graph() {
  const auto loan = DataFrame({
      {"loan_id", pluss::make_series(std::vector<std::int64_t>{1, 2})},
      {"account_id", pluss::make_series(std::vector<std::int64_t>{10, 11})},
  });
  const auto account = DataFrame({
      {"account_id", pluss::make_series(std::vector<std::int64_t>{10, 11})},
      {"district_id", pluss::make_series(std::vector<std::int64_t>{1, 2})},
  });
  const auto disp = DataFrame({
      {"disp_id", pluss::make_series(std::vector<std::int64_t>{100, 101})},
      {"account_id", pluss::make_series(std::vector<std::int64_t>{10, 11})},
      {"client_id", pluss::make_series(std::vector<std::int64_t>{501, 502})},
  });

  const auto base = pluss::bundle::Nexus({
      {"loan", loan},
      {"account", account},
      {"disp", disp},
  });

  pluss::bundle::nexus::TableMeta loan_meta;
  loan_meta.grain = "one row per loan";
  loan_meta.primary_key = {"loan_id"};
  loan_meta.candidate_keys = {{"loan_id"}, {"account_id", "loan_id"}};
  loan_meta.time_field = "issued_at";
  loan_meta.case_labels = pluss::bundle::nexus::CaseLabelPolicy::ImplicitRange;
  loan_meta.source = "bank.csv";
  loan_meta.provenance = "loader";
  loan_meta.note = "core lending table";

  const auto with_meta = base.set_table_meta("loan", loan_meta);
  expect(!base.has_table_meta("loan"), "Nexus non-mutating metadata set should not mutate source");
  expect(with_meta.has_table_meta("loan"), "Nexus should retain stored metadata by table key");
  expect(with_meta.table_meta("loan")->primary_key == std::vector<std::string>({"loan_id"}),
         "Nexus::table_meta should expose stored primary key metadata");

  pluss::bundle::nexus::Relationship loan_account;
  loan_account.name = "loan_account";
  loan_account.left_table = "loan";
  loan_account.right_table = "account";
  loan_account.keys = {{"account_id", "account_id"}};
  loan_account.cardinality = pluss::bundle::nexus::RelationshipCardinality::ManyToOne;
  loan_account.confidence = 0.95;
  loan_account.status = pluss::bundle::nexus::RelationshipValidationStatus::Declared;

  pluss::bundle::nexus::Relationship account_disp;
  account_disp.name = "account_disp";
  account_disp.left_table = "account";
  account_disp.right_table = "disp";
  account_disp.keys = {{"account_id", "account_id"}};
  account_disp.cardinality = pluss::bundle::nexus::RelationshipCardinality::OneToMany;
  account_disp.left_optional = false;
  account_disp.right_optional = true;
  account_disp.confidence = 1.0;
  account_disp.status = pluss::bundle::nexus::RelationshipValidationStatus::Validated;

  const auto graph = with_meta.add_relationships({loan_account, account_disp});
  expect(graph.nrelationships() == 2, "Nexus should track relationship count");
  expect(graph.has_relationship("loan_account"), "Nexus should index relationships by name");
  expect(graph.relationship("account_disp")->right_table == "disp",
         "Nexus::relationship should expose stored relationship payloads");

  const auto renamed = graph.rename("disp", "assignment");
  expect(renamed.relationship("account_disp")->right_table == "assignment",
         "Nexus::rename should rewrite relationship table references");
  expect(renamed.table_meta("loan")->note == std::optional<std::string>{"core lending table"},
         "Nexus::rename should preserve unrelated table metadata");

  const auto relabeled_meta = renamed.rename("loan", "credit");
  expect(relabeled_meta.has_table_meta("credit") && !relabeled_meta.has_table_meta("loan"),
         "Nexus::rename should move metadata to the renamed table key");
  expect(relabeled_meta.relationship("loan_account")->left_table == "credit",
         "Nexus::rename should rewrite left_table relationship references");

  const auto subset = graph.select({"loan", "account"});
  expect(subset.nrelationships() == 1 && subset.has_relationship("loan_account") &&
             !subset.has_relationship("account_disp"),
         "Nexus::select should retain only relationships whose endpoints survive");
  expect(subset.has_table_meta("loan") && !subset.has_table_meta("disp"),
         "Nexus::select should carry metadata only for retained tables");

  const auto dropped = graph.drop({"disp"});
  expect(dropped.nrelationships() == 1 && dropped.has_relationship("loan_account"),
         "Nexus::drop should remove relationships touching dropped tables");
}

void test_nexus_validation_and_audit_layer() {
  const auto loan_account_validity = pluss::make_series(std::vector<bool>{true, true, true, false});
  const auto loan = DataFrame({
      {"loan_id", pluss::make_series(std::vector<std::int64_t>{1, 2, 3, 4})},
      {"account_id",
       pluss::table::Thread(
           pluss::make_series(std::vector<std::int64_t>{10, 11, 999, 0}),
           loan_account_validity)},
  });
  const auto account = DataFrame({
      {"account_id", pluss::make_series(std::vector<std::int64_t>{10, 11, 11, 12})},
      {"district_id", pluss::make_series(std::vector<std::int64_t>{1, 1, 2, 3})},
  });
  const auto disp = DataFrame({
      {"disp_id", pluss::make_series(std::vector<std::int64_t>{100, 101, 102})},
      {"account_id", pluss::make_series(std::vector<std::int64_t>{10, 11, 12})},
      {"client_id", pluss::make_series(std::vector<std::int64_t>{501, 502, 503})},
  });

  pluss::bundle::nexus::TableMeta loan_meta;
  loan_meta.primary_key = {"loan_id"};
  loan_meta.candidate_keys = {{"account_id"}};

  pluss::bundle::nexus::TableMeta account_meta;
  account_meta.primary_key = {"account_id"};
  account_meta.candidate_keys = {{"account_id", "district_id"}};

  pluss::bundle::nexus::TableMeta disp_meta;
  disp_meta.primary_key = {"disp_id"};

  pluss::bundle::nexus::Relationship loan_account;
  loan_account.name = "loan_account";
  loan_account.left_table = "loan";
  loan_account.right_table = "account";
  loan_account.keys = {{"account_id", "account_id"}};
  loan_account.cardinality = pluss::bundle::nexus::RelationshipCardinality::ManyToOne;
  loan_account.left_optional = true;
  loan_account.right_optional = false;
  loan_account.status = pluss::bundle::nexus::RelationshipValidationStatus::Declared;

  const auto nx = pluss::bundle::Nexus({
                        {"loan", loan},
                        {"account", account},
                        {"disp", disp},
                    })
                      .set_table_meta("loan", loan_meta)
                      .set_table_meta("account", account_meta)
                      .set_table_meta("disp", disp_meta)
                      .add_relationship(loan_account);

  expect(nx.validate_keys("loan"), "Nexus::validate_keys should accept a clean declared primary key");
  expect(!nx.validate_keys("account"),
         "Nexus::validate_keys should reject duplicate declared primary keys");
  expect(nx.validate_keys("account", {"account_id", "district_id"}),
         "Nexus::validate_keys(keys) should allow explicit alternate key audits");

  const auto account_dups = nx.detect_duplicates("account");
  expect(account_dups.ncases() == 2,
         "Nexus::detect_duplicates should materialize repeated declared-key rows");
  expect(account_dups.features("account_id").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({11, 11}),
         "Nexus::detect_duplicates should retain duplicate key values");

  const auto grain = nx.grain_audit("account");
  expect(grain.ncases() == 2 && grain.nfeatures() == 9,
         "Nexus::grain_audit should emit one audit row per declared key set");
  expect(grain.features("duplicate_rows").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({2, 0}),
         "Nexus::grain_audit should quantify duplicate pressure per declared key set");
  expect(grain.features("valid").as_series().to_vec<bool>() ==
             std::vector<bool>({false, true}),
         "Nexus::grain_audit should mark invalid and valid key declarations explicitly");

  const auto left_orphans =
      nx.detect_orphans("loan_account", pluss::bundle::nexus::RelationshipSide::Left);
  expect(left_orphans.ncases() == 1 &&
             left_orphans.features("loan_id").as_series().to_vec<std::int64_t>() ==
                 std::vector<std::int64_t>({3}),
         "Nexus::detect_orphans(left) should expose unmatched complete-key left rows and exclude missing keys");

  const auto right_orphans =
      nx.detect_orphans("loan_account", pluss::bundle::nexus::RelationshipSide::Right);
  expect(right_orphans.ncases() == 1 &&
             right_orphans.features("account_id").as_series().to_vec<std::int64_t>() ==
                 std::vector<std::int64_t>({12}),
         "Nexus::detect_orphans(right) should expose unmatched right-side rows");

  expect(!nx.validate_relationship("loan_account"),
         "Nexus::validate_relationship should fail when coverage and uniqueness guarantees are broken");

  const auto audit = nx.relationship_audit("loan_account");
  expect(audit.ncases() == 1, "Nexus::relationship_audit should return a one-row audit summary");
  expect(audit.features("left_missing_keys").as_series().to_vec<std::int64_t>() ==
                 std::vector<std::int64_t>({1}) &&
             audit.features("left_orphans").as_series().to_vec<std::int64_t>() ==
                 std::vector<std::int64_t>({1}) &&
             audit.features("right_duplicate_rows").as_series().to_vec<std::int64_t>() ==
                 std::vector<std::int64_t>({2}),
         "Nexus::relationship_audit should expose missing-key, orphan, and duplicate pressure together");
  expect(audit.features("computed_status").as_note().at(0) == "invalid" &&
             !audit.features("valid").as_series().to_vec<bool>().front(),
         "Nexus::relationship_audit should surface the computed invalid state");
}

void test_nexus_relational_execution_helpers() {
  const auto loan = DataFrame({
      {"loan_id", pluss::make_series(std::vector<std::int64_t>{1, 2})},
      {"account_id", pluss::make_series(std::vector<std::int64_t>{10, 11})},
      {"status", pluss::make_note({"open", "closed"})},
  });
  const auto account = DataFrame({
      {"account_id", pluss::make_series(std::vector<std::int64_t>{10, 11})},
      {"district_id", pluss::make_series(std::vector<std::int64_t>{1, 2})},
      {"status", pluss::make_note({"gold", "silver"})},
  });
  const auto disp = DataFrame({
      {"disp_id", pluss::make_series(std::vector<std::int64_t>{100, 101, 102})},
      {"account_id", pluss::make_series(std::vector<std::int64_t>{10, 10, 11})},
      {"client_id", pluss::make_series(std::vector<std::int64_t>{501, 502, 503})},
      {"status", pluss::make_note({"owner", "disponent", "owner"})},
  });
  const auto client = DataFrame({
      {"client_id", pluss::make_series(std::vector<std::int64_t>{501, 502, 503})},
      {"name", pluss::make_note({"Ama", "Kojo", "Esi"})},
  });

  pluss::bundle::nexus::Relationship loan_account;
  loan_account.name = "loan_account";
  loan_account.left_table = "loan";
  loan_account.right_table = "account";
  loan_account.keys = {{"account_id", "account_id"}};
  loan_account.cardinality = pluss::bundle::nexus::RelationshipCardinality::ManyToOne;

  pluss::bundle::nexus::Relationship account_disp;
  account_disp.name = "account_disp";
  account_disp.left_table = "account";
  account_disp.right_table = "disp";
  account_disp.keys = {{"account_id", "account_id"}};
  account_disp.cardinality = pluss::bundle::nexus::RelationshipCardinality::OneToMany;

  pluss::bundle::nexus::Relationship disp_client;
  disp_client.name = "disp_client";
  disp_client.left_table = "disp";
  disp_client.right_table = "client";
  disp_client.keys = {{"client_id", "client_id"}};
  disp_client.cardinality = pluss::bundle::nexus::RelationshipCardinality::ManyToOne;

  const auto nx = pluss::bundle::Nexus({
                        {"loan", loan},
                        {"account", account},
                        {"disp", disp},
                        {"client", client},
                    })
                      .add_relationships({loan_account, account_disp, disp_client});

  pluss::bundle::nexus::JoinSpec direct;
  direct.relationship_name = "loan_account";
  direct.how = pluss::bundle::nexus::JoinHow::Left;

  const auto joined = nx.join("loan", "account", direct);
  expect(joined.ncases() == 2, "Nexus::join should delegate a direct relationship-driven left join");
  expect(joined.title() == "loan_account",
         "Nexus::join should assign a stable default result title");
  expect(joined.get("status_account").has_value(),
         "Nexus::join should use deterministic right-table suffixing for feature collisions");
  expect(joined.features("district_id").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({1, 2}),
         "Nexus::join should pull right-table payload columns through DataFrame::unify");

  const auto path = nx.path("loan", "client");
  expect(path.steps.size() == 3,
         "Nexus::path should discover a multi-hop relationship path through the workspace graph");
  expect(path.steps[0].relationship_name == "loan_account" &&
             path.steps[1].relationship_name == "account_disp" &&
             path.steps[2].relationship_name == "disp_client",
         "Nexus::path should preserve traversal order across relationships");
  expect(!path.steps[0].reversed && !path.steps[1].reversed && !path.steps[2].reversed,
         "Nexus::path should mark forward traversals as non-reversed");

  const auto reverse_path = nx.path("client", "loan");
  expect(reverse_path.steps.size() == 3 && reverse_path.steps[0].reversed &&
             reverse_path.steps[1].reversed && reverse_path.steps[2].reversed,
         "Nexus::path should mark reverse traversal against relationship direction");

  const auto plan = nx.plan(path, pluss::bundle::nexus::JoinHow::Left);
  expect(plan.root_table == "loan" && plan.steps.size() == 3,
         "Nexus::plan should convert a graph path into an executable join plan");
  expect(plan.result_name == "loan_client",
         "Nexus::plan should generate a stable multi-table result name");
  expect(plan.steps[1].spec.right_suffix == "_disp" &&
             plan.steps[2].spec.relationship_name == std::optional<std::string>{"disp_client"},
         "Nexus::plan should carry deterministic per-step suffixing and relationship provenance");

  const auto materialized = nx.materialize(plan);
  expect(materialized.title() == "loan_client",
         "Nexus::materialize should apply the plan result name as the final frame title");
  expect(materialized.ncases() == 3,
         "Nexus::materialize should execute left-preserving multi-hop joins across the plan");
  expect(materialized.get("status_account").has_value() &&
             materialized.get("status_disp").has_value(),
         "Nexus::materialize should preserve deterministic right-table suffixing across multiple joins");
  expect(materialized.features("name").as_note().at(0) == "Ama" &&
             materialized.features("name").as_note().at(1) == "Kojo" &&
             materialized.features("name").as_note().at(2) == "Esi",
         "Nexus::materialize should carry payload columns from the terminal table through the join chain");
}

void test_nexus_query_surface() {
  const auto loan = DataFrame({
      {"loan_id", pluss::make_series(std::vector<std::int64_t>{1, 2})},
      {"account_id", pluss::make_series(std::vector<std::int64_t>{10, 11})},
      {"amount", pluss::make_series(std::vector<double>{5000.0, 7000.0})},
  });
  const auto account = DataFrame({
      {"account_id", pluss::make_series(std::vector<std::int64_t>{10, 11})},
      {"district_id", pluss::make_series(std::vector<std::int64_t>{1, 2})},
      {"status", pluss::make_note({"gold", "silver"})},
  });
  const auto disp = DataFrame({
      {"disp_id", pluss::make_series(std::vector<std::int64_t>{100, 101, 102})},
      {"account_id", pluss::make_series(std::vector<std::int64_t>{10, 10, 11})},
      {"client_id", pluss::make_series(std::vector<std::int64_t>{501, 502, 503})},
  });
  const auto client = DataFrame({
      {"client_id", pluss::make_series(std::vector<std::int64_t>{501, 502, 503})},
      {"name", pluss::make_note({"Ama", "Kojo", "Esi"})},
      {"tier", pluss::make_note({"A", "B", "A"})},
  });

  pluss::bundle::nexus::Relationship loan_account;
  loan_account.name = "loan_account";
  loan_account.left_table = "loan";
  loan_account.right_table = "account";
  loan_account.keys = {{"account_id", "account_id"}};
  loan_account.cardinality = pluss::bundle::nexus::RelationshipCardinality::ManyToOne;

  pluss::bundle::nexus::Relationship account_disp;
  account_disp.name = "account_disp";
  account_disp.left_table = "account";
  account_disp.right_table = "disp";
  account_disp.keys = {{"account_id", "account_id"}};
  account_disp.cardinality = pluss::bundle::nexus::RelationshipCardinality::OneToMany;

  pluss::bundle::nexus::Relationship disp_client;
  disp_client.name = "disp_client";
  disp_client.left_table = "disp";
  disp_client.right_table = "client";
  disp_client.keys = {{"client_id", "client_id"}};
  disp_client.cardinality = pluss::bundle::nexus::RelationshipCardinality::ManyToOne;

  const auto nx = pluss::bundle::Nexus({
                        {"loan", loan},
                        {"account", account},
                        {"disp", disp},
                        {"client", client},
                    })
                      .add_relationships({loan_account, account_disp, disp_client});

  const auto chained = nx.from("loan")
                           .join("account")
                           .join("disp")
                           .join("client")
                           .pick({"loan_id", "amount", "status", "name"})
                           .name("sheet")
                           .collect();
  expect(chained.title() == "sheet",
         "Nexus query chains should allow short final naming through name().collect()");
  expect(chained.ncases() == 3 && chained.nfeatures() == 4,
         "Nexus query chains should materialize multi-hop joins then project selected features");
  expect(chained.features("name").as_note().at(0) == "Ama" &&
             chained.features("name").as_note().at(1) == "Kojo" &&
             chained.features("name").as_note().at(2) == "Esi",
         "Nexus query join chaining should preserve terminal-table payload columns");

  const auto hopped = nx.from("loan")
                          .via("client")
                          .pick({"loan_id", "name", "tier"})
                          .collect();
  expect(hopped.title() == "loan_client",
         "NexusQuery::via should inherit the compact root-to-terminal default name");
  expect(hopped.ncases() == 3 &&
             hopped.features("tier").as_note().at(0) == "A" &&
             hopped.features("tier").as_note().at(1) == "B" &&
             hopped.features("tier").as_note().at(2) == "A",
         "NexusQuery::via should expand a graph path into a materialized multi-hop result");

  const auto trimmed = nx.from("loan")
                           .join("account")
                           .drop({"district_id"})
                           .collect();
  expect(!trimmed.get("district_id").has_value() && trimmed.get("status").has_value(),
         "Nexus query chains should support post-join column dropping with a one-word verb");
}

void test_nexus_pull_surface() {
  const auto customers = DataFrame({
      {"customer_id", pluss::make_series(std::vector<std::int64_t>{1, 2, 3})},
      {"region_id", pluss::make_series(std::vector<std::int64_t>{10, 11, 10})},
      {"observed_at", pluss::make_timeline({
          "2026-02-01T00:00:00Z",
          "2026-01-31T00:00:00Z",
          "2026-01-10T00:00:00Z",
      })},
      {"tier", pluss::make_note({"gold", "silver", "bronze"})},
  });
  const auto regions = DataFrame({
      {"region_id", pluss::make_series(std::vector<std::int64_t>{10, 11})},
      {"zone", pluss::make_note({"west", "east"})},
  });
  const auto events = DataFrame({
      {"event_id", pluss::make_series(std::vector<std::int64_t>{100, 101, 102, 103, 104})},
      {"customer_id", pluss::make_series(std::vector<std::int64_t>{1, 1, 2, 2, 2})},
      {"occurred_at", pluss::make_timeline({
          "2026-01-01T00:00:00Z",
          "2026-01-20T00:00:00Z",
          "2026-01-05T00:00:00Z",
          "2026-01-25T00:00:00Z",
          "2026-02-10T00:00:00Z",
      })},
      {"spend", pluss::make_series(std::vector<double>{10.0, 5.0, 2.0, 8.0, 4.0})},
      {"metric.value", pluss::make_series(std::vector<double>{100.0, 120.0, 80.0, 90.0, 110.0})},
  });

  pluss::bundle::nexus::TableMeta customers_meta;
  customers_meta.primary_key = {"customer_id"};
  customers_meta.time_field = "observed_at";

  pluss::bundle::nexus::TableMeta events_meta;
  events_meta.time_field = "occurred_at";

  pluss::bundle::nexus::Relationship customers_regions;
  customers_regions.name = "customers_regions";
  customers_regions.left_table = "customers";
  customers_regions.right_table = "regions";
  customers_regions.keys = {{"region_id", "region_id"}};
  customers_regions.cardinality = pluss::bundle::nexus::RelationshipCardinality::ManyToOne;

  pluss::bundle::nexus::Relationship customers_events;
  customers_events.name = "customers_events";
  customers_events.left_table = "customers";
  customers_events.right_table = "events";
  customers_events.keys = {{"customer_id", "customer_id"}};
  customers_events.cardinality = pluss::bundle::nexus::RelationshipCardinality::OneToMany;

  std::unordered_map<std::string, pluss::bundle::nexus::TableMeta> meta_batch;
  meta_batch.emplace("customers", customers_meta);
  meta_batch.emplace("events", events_meta);

  const auto nx = pluss::bundle::Nexus({
                        {"customers", customers},
                        {"regions", regions},
                        {"events", events},
                    })
                      .meta(meta_batch)
                      .links({customers_regions, customers_events});

  const auto pulled = nx.pull("customers", {
      "customer_id",
      "tier",
      "regions.zone",
      "events.spend.count",
      "events.spend.sum",
      "events.spend.sum_30d",
      "events.spend.sum_30d_root",
      "events.`metric.value`.max",
  });
  expect(pulled.title() == "customers_pull" && pulled.ncases() == customers.ncases(),
         "Nexus::pull should materialize a root-grain analytical table with a stable title");
  expect(pulled.features().size() == 8 &&
             pulled.get("customer_id").has_value() &&
             pulled.get("tier").has_value() &&
             pulled.get("regions_zone").has_value() &&
             pulled.get("events_spend_count").has_value() &&
             pulled.get("events_spend_sum").has_value() &&
             pulled.get("events_spend_sum_30d").has_value() &&
             pulled.get("events_spend_sum_30d_root").has_value() &&
             pulled.get("events_metric_value_max").has_value(),
         "Nexus::pull should expose plain workspace projections and aggregate child projections with stable aliases");
  expect(pulled.features("regions_zone").as_note().at(0) == "west" &&
             pulled.features("regions_zone").as_note().at(1) == "east" &&
             pulled.features("events_spend_count").as_series().to_vec<std::int64_t>()
                 == std::vector<std::int64_t>({2, 3, 0}) &&
             pulled.features("events_spend_sum").as_series().to_vec<double>(0) == 15.0 &&
             pulled.features("events_spend_sum_30d").as_series().to_vec<double>(1) == 12.0 &&
             pulled.features("events_spend_sum_30d_root").as_series().to_vec<double>(0) == 5.0 &&
             pulled.features("events_spend_sum_30d_root").as_series().to_vec<double>(1) == 10.0 &&
             pulled.features("events_metric_value_max").as_series().to_vec<double>(0) == 120.0 &&
             !pluss::table::detail::valid_at(pulled.features("events_spend_sum"), 2) &&
             !pluss::table::detail::valid_at(pulled.features("events_metric_value_max"), 2) &&
             pulled.features("events_spend_sum_30d").as_series().to_vec<double>(0) !=
                 pulled.features("events_spend_sum_30d_root").as_series().to_vec<double>(0),
         "Nexus::pull should aggregate one-to-many related rows back to root grain, including dotted feature names and explicit root-relative windows");

  auto chained_pull = nx.pull("customers");
  const auto built = chained_pull
                         .take("customer_id")
                         .take("regions", {{"zone", "zone"}})
                         .agg("events")
                             .count("event_id", "event_count")
                             .sum("spend", "total_spend")
                             .mean("spend", "mean_spend")
                             .median("spend", "median_spend")
                             .min("spend", "min_spend")
                             .max("spend", "max_spend")
                             .min("occurred_at", "first_event_at")
                             .max("occurred_at", "last_event_at")
                             .std("spend", "std_spend")
                             .var("spend", "var_spend")
                         .end()
                         .add({pluss::bundle::nexus::sum("events.spend", "recent_spend").windowed(30, "d")})
                         .collect();
  expect(built.get("customer_id").has_value() &&
             built.get("zone").has_value() &&
             built.get("event_count").has_value() &&
             built.get("median_spend").has_value() &&
             built.get("first_event_at").has_value() &&
             built.get("last_event_at").has_value() &&
             built.get("std_spend").has_value() &&
             built.get("var_spend").has_value() &&
             built.get("recent_spend").has_value(),
         "Nexus::pull(root).take(...).sum(...).collect() should assemble typed root-grain features without per-field namespace repetition");
  expect(chained_pull.root() == "customers" &&
             chained_pull.specs().empty(),
         "Nexus::pull(root) should start as an empty fluent builder anchored on the requested root table");
  expect(built.features("zone").as_note().at(0) == "west" &&
             built.features("event_count").as_series().to_vec<std::int64_t>()
                 == std::vector<std::int64_t>({2, 3, 0}) &&
             built.features("median_spend").as_series().to_vec<double>(0) == 7.5 &&
             built.features("median_spend").as_series().to_vec<double>(1) == 4.0 &&
             built.features("min_spend").as_series().to_vec<double>(0) == 5.0 &&
             built.features("max_spend").as_series().to_vec<double>(1) == 8.0 &&
             built.features("first_event_at").as_timeline().to_epoch_nanos_vector()[0] ==
                 events.features("occurred_at").as_timeline().to_epoch_nanos_vector()[0] &&
             built.features("last_event_at").as_timeline().to_epoch_nanos_vector()[1] ==
                 events.features("occurred_at").as_timeline().to_epoch_nanos_vector()[4] &&
             built.features("std_spend").as_series().to_vec<double>(0) > 0.0 &&
             built.features("var_spend").as_series().to_vec<double>(1) > 0.0 &&
             built.features("recent_spend").as_series().to_vec<double>(1) == 12.0 &&
             !pluss::table::detail::valid_at(built.features("total_spend"), 2) &&
             !pluss::table::detail::valid_at(built.features("first_event_at"), 2) &&
             !pluss::table::detail::valid_at(built.features("last_event_at"), 2),
         "Fluent Nexus::pull builders should preserve root grain while exposing scalar lifts, grouped reducers, and advanced typed specs when needed");

  const auto batched = nx.pull("customers")
                           .take(std::map<std::string, std::string>{
                               {"customer_id", "customer_id"},
                               {"tier", "tier"},
                           })
                           .take("regions", std::map<std::string, std::string>{
                               {"zone", "zone"},
                           })
                           .agg("events")
                               .count(std::map<std::string, std::string>{
                                   {"event_id", "event_count"},
                                   {"occurred_at", "dated_event_count"},
                               })
                               .sum(std::unordered_map<std::string, std::string>{
                                   {"spend", "total_spend"},
                               })
                               .max(std::unordered_map<std::string, std::string>{
                                   {"spend", "max_spend"},
                               })
                           .end()
                           .collect();
  expect(batched.get("customer_id").has_value() &&
             batched.get("tier").has_value() &&
             batched.get("zone").has_value() &&
             batched.get("event_count").has_value() &&
             batched.get("dated_event_count").has_value() &&
             batched.get("total_spend").has_value() &&
             batched.get("max_spend").has_value() &&
             batched.features("dated_event_count").as_series().to_vec<std::int64_t>()
                 == std::vector<std::int64_t>({2, 3, 0}) &&
             batched.features("total_spend").as_series().to_vec<double>(0) == 15.0 &&
             batched.features("max_spend").as_series().to_vec<double>(1) == 8.0,
         "Nexus fluent builders should accept batched std::map/std::unordered_map specs for metadata, take(), and grouped reducers");

  const auto typed = nx.pull("customers", {
      pluss::bundle::nexus::take("customer_id"),
      pluss::bundle::nexus::count("events.event_id", "event_count"),
      pluss::bundle::nexus::sum("events.spend", "total_spend"),
  });
  expect(typed.get("customer_id").has_value() &&
             typed.get("event_count").has_value() &&
             typed.get("total_spend").has_value(),
         "Nexus::pull(initializer_list<PullSpec>) should remain available as the lower-level typed execution surface");

  expect_throw([&]() { (void)nx.pull("customers", {"events.event_id"}); },
               "Nexus::pull should still reject raw one-to-many projections that would break root grain");
}

void test_nexus_query_sql_accessor() {
  const auto customers = DataFrame({
      {"customer_id", pluss::make_series(std::vector<std::int64_t>{1, 2, 3})},
      {"name", pluss::make_note({"Ama", "Kojo", "Esi"})},
      {"tier", pluss::make_note({"gold", "silver", "gold"})},
  });
  const auto orders = DataFrame({
      {"order_id", pluss::make_series(std::vector<std::int64_t>{10, 11, 12, 13})},
      {"customer_id", pluss::make_series(std::vector<std::int64_t>{1, 1, 2, 3})},
      {"amount", pluss::make_series(std::vector<double>{8.0, 12.0, 4.0, 20.0})},
  });

  const auto nx = pluss::bundle::Nexus({
      {"customers", customers},
      {"orders", orders},
  });

  const auto grouped = nx.query().execute(
      "SELECT customer_id, SUM(amount) AS total_amount "
      "FROM orders "
      "GROUP BY customer_id "
      "HAVING SUM(amount) >= 8 "
      "ORDER BY total_amount DESC");
  expect(grouped.nfeatures() == 2 && grouped.get("customer_id").has_value()
             && grouped.get("total_amount").has_value() && grouped.ncases() == 2,
         "Nexus::query().execute: workspace SQL should support grouped aggregates from any Nexus table");

  const auto joined = nx.query().execute(
      "WITH filtered AS ("
      "  SELECT customer_id, amount "
      "  FROM orders "
      "  WHERE amount >= 8"
      ") "
      "SELECT customers.name, SUM(filtered.amount) AS total_amount "
      "FROM filtered "
      "LEFT JOIN customers ON customers.customer_id = filtered.customer_id "
      "GROUP BY customers.name "
      "ORDER BY total_amount DESC");
  expect(joined.nfeatures() == 2 && joined.get("customers.name").has_value()
             && joined.get("total_amount").has_value() && joined.ncases() == 2,
         "Nexus::query().execute: workspace SQL should support CTEs and joins without a special root-table restriction");
}

void test_nexus_workspace_insights_and_views() {
  const auto loan = DataFrame({
      {"loan_id", pluss::make_series(std::vector<std::int64_t>{1, 2})},
      {"account_id", pluss::make_series(std::vector<std::int64_t>{10, 11})},
      {"amount", pluss::make_series(std::vector<double>{5000.0, 7000.0})},
  });
  const auto account = DataFrame({
      {"account_id", pluss::make_series(std::vector<std::int64_t>{10, 11})},
      {"district_id", pluss::make_series(std::vector<std::int64_t>{1, 2})},
      {"segment", pluss::make_note({"retail", "vip"})},
  });
  const auto trans = DataFrame({
      {"trans_id", pluss::make_series(std::vector<std::int64_t>{100, 101, 102})},
      {"account_id", pluss::make_series(std::vector<std::int64_t>{10, 10, 11})},
      {"amount", pluss::make_series(std::vector<double>{25.0, 40.0, 10.0})},
  });

  pluss::bundle::nexus::TableMeta loan_meta;
  loan_meta.grain = "one row per loan";
  loan_meta.primary_key = {"loan_id"};
  loan_meta.candidate_keys = {{"account_id"}};
  loan_meta.time_field = "booked_at";
  loan_meta.source = "loan.csv";

  pluss::bundle::nexus::TableMeta account_meta;
  account_meta.grain = "one row per account";
  account_meta.primary_key = {"account_id"};
  account_meta.source = "account.csv";

  pluss::bundle::nexus::Relationship loan_account;
  loan_account.name = "loan_account";
  loan_account.left_table = "loan";
  loan_account.right_table = "account";
  loan_account.keys = {{"account_id", "account_id"}};
  loan_account.cardinality = pluss::bundle::nexus::RelationshipCardinality::ManyToOne;

  const auto nx = pluss::bundle::Nexus({
                        {"loan", loan},
                        {"account", account},
                        {"trans", trans},
                    })
                      .meta("loan", loan_meta)
                      .meta("account", account_meta)
                      .links({loan_account});

  const auto profile = nx.profile();
  expect(profile.ncases() == 3 && profile.get("null_cells").has_value(),
         "Nexus::profile should return one profiling row per workspace table");
  expect(profile.features("table").as_note().at(0) == "loan" &&
             profile.features("key_duplicate_rows").as_series().to_vec<std::int64_t>()[0] == 0,
         "Nexus::profile should expose declared-key health beside size metrics");

  const auto scan = nx.scan();
  expect(scan.ncases() == 4 && scan.get("kind").has_value(),
         "Nexus::scan should combine table and relationship diagnostics into one command-center frame");
  expect(scan.features("kind").as_note().at(3) == "relationship" &&
             scan.features("computed_status").as_note().at(3) == "validated",
         "Nexus::scan should surface relationship validation state");
  const auto audit = nx.audit();
  expect(audit.ncases() == scan.ncases() &&
             audit.features("computed_status").as_note().at(3) == "validated",
         "Nexus::audit() should expose the workspace-level integrity audit without forcing a relationship-only query");

  const auto deduced = nx.deduce();
  expect(deduced.ncases() >= 1,
         "Nexus::deduce should suggest plausible joins from shared names and uniqueness pressure");
  expect(deduced.features("left").as_note().at(0) == "loan" &&
             deduced.features("right").as_note().at(0) == "account" &&
             deduced.features("confidence").as_series().to_vec<double>()[0] >= 0.5,
         "Nexus::deduce should report confident advisory matches without auto-committing them");
  {
    const auto names = deduced.features("name").as_note();
    bool unique_names = true;
    for (std::int64_t i = 0; i < deduced.ncases() && unique_names; ++i) {
      for (std::int64_t j = i + 1; j < deduced.ncases(); ++j) {
        if (names.at(i) == names.at(j)) {
          unique_names = false;
          break;
        }
      }
    }
    expect(unique_names,
           "Nexus::deduce should assign a distinct suggestion name to each candidate row");
  }

  const auto neigh = nx.neighbors("loan");
  expect(neigh.ncases() == 1 && neigh.features("other").as_note().at(0) == "account",
         "Nexus::neighbors should expose graph-local adjacent tables");

  const auto plan = nx.plan("loan", "account");
  const auto staged = nx.store("sheet", plan);
  expect(staged.has_store("sheet") && staged.store("sheet")->result_name == "sheet",
         "Nexus::store(name, plan) should persist a named run-plan inside the workspace");
  expect(staged.run("sheet").title() == "sheet" &&
             staged.run("sheet").features("segment").as_note().at(0) == "retail",
         "Nexus::run(name) should materialize stored plans under the saved store name");
  expect(staged.run("quick", plan).title() == "quick" &&
             staged.run("quick", plan).features("segment").as_note().at(1) == "vip",
         "Nexus::run(name, plan) should support ad hoc named materialization without pre-storing");
}

void test_nexus_persistence_and_alias_surface() {
  const auto loan = DataFrame({
      {"loan_id", pluss::make_series(std::vector<std::int64_t>{1, 2})},
      {"account_id", pluss::make_series(std::vector<std::int64_t>{10, 11})},
      {"amount", pluss::make_series(std::vector<double>{5000.0, 7000.0})},
  });
  const auto account = DataFrame({
      {"account_id", pluss::make_series(std::vector<std::int64_t>{10, 11})},
      {"district_id", pluss::make_series(std::vector<std::int64_t>{1, 2})},
      {"segment", pluss::make_note({"retail", "vip"})},
  });

  pluss::bundle::nexus::TableMeta loan_meta;
  loan_meta.grain = "one row per loan";
  loan_meta.primary_key = {"loan_id"};
  loan_meta.candidate_keys = {{"loan_id"}, {"account_id", "loan_id"}};
  loan_meta.source = "loan.csv";
  loan_meta.provenance = "fixture";
  loan_meta.note = "lending";

  pluss::bundle::nexus::Relationship loan_account;
  loan_account.name = "loan_account";
  loan_account.left_table = "loan";
  loan_account.right_table = "account";
  loan_account.keys = {{"account_id", "account_id"}};
  loan_account.cardinality = pluss::bundle::nexus::RelationshipCardinality::ManyToOne;
  loan_account.status = pluss::bundle::nexus::RelationshipValidationStatus::Validated;
  loan_account.note = "primary join";

  const auto nx = pluss::bundle::Nexus({
                        {"loan", loan},
                        {"account", account},
                    })
                      .meta("loan", loan_meta)
                      .links({loan_account})
                      .store("loan_sheet", pluss::bundle::nexus::JoinPlan{
                                              .root_table = "loan",
                                              .result_name = "loan_sheet",
                                              .steps = {{
                                                  .left_table = "loan",
                                                  .right_table = "account",
                                                  .spec =
                                                      pluss::bundle::nexus::JoinSpec{
                                                          .how = pluss::bundle::nexus::JoinHow::Left,
                                                          .relationship_name = "loan_account",
                                                      },
                                              }},
                                          });

  expect(nx.meta("loan")->primary_key == std::vector<std::string>({"loan_id"}),
         "Nexus::meta should alias table metadata access cleanly");
  expect(nx.links().size() == 1 && nx.links().front().name == "loan_account",
         "Nexus::links should expose stored relationship payloads");
  expect(nx.duplicates("loan").ncases() == 0,
         "Nexus::duplicates should alias duplicate detection");
  expect(nx.check("loan_account"),
         "Nexus::check should alias relationship validation");
  expect(nx.orphans("loan_account").ncases() == 0,
         "Nexus::orphans should alias orphan detection");
  expect(nx.audit("loan_account").features("computed_status").as_note().at(0) == "validated",
         "Nexus::audit should alias the relationship audit summary");

  const auto schema = nx.schema();
  expect(schema.ncases() == 2 && schema.get("pkey").has_value(),
         "Nexus::schema should expose a compact schema snapshot");
  expect(schema.features("table").as_note().at(0) == "loan" &&
             schema.features("source").as_note().at(0) == "loan.csv",
         "Nexus::schema should carry table identity and provenance fields");

  const auto graph = nx.graph();
  expect(graph.ncases() == 1 && graph.features("name").as_note().at(0) == "loan_account",
         "Nexus::graph should expose the relationship snapshot");

  const auto before_fp = nx.fingerprint();
  expect(!before_fp.empty(), "Nexus::fingerprint should produce a stable workspace hash");

  const auto dir = std::filesystem::temp_directory_path() / "plussmic_nexus_store_debug";
  std::filesystem::create_directories(dir);

  nx.save(dir.string());
  expect(std::filesystem::exists(dir / "manifest.csv") &&
             std::filesystem::exists(dir / "schema.csv") &&
             std::filesystem::exists(dir / "graph.csv") &&
             std::filesystem::exists(dir / "tables" / "0_loan.parquet") &&
             std::filesystem::exists(dir / "tables" / "1_account.parquet"),
         "Nexus::save should persist a manifest, snapshots, and table payload files");

  const auto loaded = pluss::bundle::Nexus::read(dir.string());
  expect(loaded.keys() == std::vector<std::string>({"loan", "account"}),
         "Nexus::read should restore workspace ordering and table keys");
  expect(loaded.frame("loan").equals(loan) && loaded.frame("account").equals(account),
         "Nexus::read should roundtrip saved DataFrame payloads");
  expect(loaded.meta("loan")->primary_key == std::vector<std::string>({"loan_id"}) &&
             loaded.meta("loan")->candidate_keys == std::vector<std::vector<std::string>>({{"loan_id"}, {"account_id", "loan_id"}}) &&
             loaded.meta("loan")->source == std::optional<std::string>{"loan.csv"} &&
             loaded.meta("loan")->provenance == std::optional<std::string>{"fixture"} &&
             loaded.meta("loan")->note == std::optional<std::string>{"lending"},
         "Nexus::read should restore per-table metadata");
  expect(loaded.links().size() == 1 &&
             loaded.links().front().keys == std::vector<std::pair<std::string, std::string>>({{"account_id", "account_id"}}) &&
             loaded.links().front().cardinality == pluss::bundle::nexus::RelationshipCardinality::ManyToOne &&
             loaded.links().front().status == pluss::bundle::nexus::RelationshipValidationStatus::Validated &&
             loaded.links().front().confidence == 1.0 &&
             loaded.links().front().note == std::optional<std::string>{"primary join"},
         "Nexus::read should restore relationship graph metadata");
  expect(loaded.check("loan_account"),
         "Nexus::read should preserve relationship join keys for validation");
  expect(loaded.schema().drop({"fingerprint"}).equals(nx.schema().drop({"fingerprint"})),
         "Nexus::read should preserve the canonical schema snapshot");
  expect(loaded.graph().equals(nx.graph()),
         "Nexus::read should preserve the canonical relationship graph snapshot");
  expect(loaded.has_store("loan_sheet") &&
             loaded.run("loan_sheet").title() == "loan_sheet" &&
             loaded.run("loan_sheet").features("segment").as_note().at(0) == "retail",
         "Nexus::read should restore persisted named workspace stores");
  expect(loaded.fingerprint() == before_fp,
         "Nexus save/load roundtrips should preserve the workspace fingerprint");
}

void test_nexus_raw_directory_loader() {
  const auto customers = DataFrame({
      {"customer_id", pluss::make_series(std::vector<std::int64_t>{1, 2})},
      {"name", pluss::make_note(std::vector<std::string>{"Ada", "Ben"})},
  });
  const auto orders = DataFrame({
      {"order_id", pluss::make_series(std::vector<std::int64_t>{100, 101})},
      {"customer_id", pluss::make_series(std::vector<std::int64_t>{1, 2})},
      {"amount", pluss::make_series(std::vector<double>{12.5, 19.0})},
  });
  const auto ignore_me = DataFrame({
      {"note_id", pluss::make_series(std::vector<std::int64_t>{1})},
      {"text", pluss::make_note(std::vector<std::string>{"ignore"})},
  });

  const auto dir = std::filesystem::temp_directory_path() /
      fmt::format("plussmic_nexus_raw_loader_{}", std::chrono::steady_clock::now().time_since_epoch().count());
  std::filesystem::create_directories(dir);

  customers.save_csv((dir / "customers.csv").string());
  orders.save_as("json", (dir / "orders.json").string());
  ignore_me.save_csv((dir / "ignore_me.csv").string());

  pluss::bundle::nexus::LoadOptions load_only;
  load_only.load = {"customers", "orders.json"};
  load_only.formats = {"csv", "json"};
  load_only.parallel = false;

  const auto loaded_subset = pluss::bundle::Nexus::load(dir.string(), load_only);
  expect(loaded_subset.ntables() == 2 &&
             loaded_subset.has("customers") &&
             loaded_subset.has("orders"),
         "Nexus::load(directory, LoadOptions) should honor an explicit load list");
  expect(loaded_subset.frame("customers").equals(customers) &&
             loaded_subset.frame("orders").equals(orders),
         "Nexus raw-directory loading should roundtrip mixed csv/json inputs");

  pluss::bundle::nexus::LoadOptions ignore_only;
  ignore_only.ignore = {"ignore_me"};
  ignore_only.parallel = false;

  const auto ignored = pluss::bundle::Nexus::load(dir.string(), ignore_only);
  expect(ignored.ntables() == 2 &&
             ignored.has("customers") &&
             ignored.has("orders") &&
             !ignored.has("ignore_me"),
         "Nexus::load(directory, LoadOptions) should honor an ignore list");

  pluss::bundle::nexus::LoadOptions invalid;
  invalid.load = {"customers"};
  invalid.ignore = {"orders"};
  expect_throw([&]() { (void)pluss::bundle::Nexus::load(dir.string(), invalid); },
               "Nexus::load should reject simultaneous load and ignore lists");

  const auto auto_loaded = pluss::bundle::Nexus::load(dir.string());
  expect(auto_loaded.keys() == std::vector<std::string>({"customers", "ignore_me", "orders"}),
         "Nexus::load(directory) should load supported raw tables directly from a folder");
  expect(auto_loaded.frame("ignore_me").equals(ignore_me),
         "Nexus::load(directory) should load all supported top-level files by default");
}

void test_nexus_cache_invalidation_on_mutation() {
  const auto loan = DataFrame({
      {"loan_id", pluss::make_series(std::vector<std::int64_t>{1, 2})},
      {"account_id", pluss::make_series(std::vector<std::int64_t>{10, 11})},
  });
  const auto account = DataFrame({
      {"account_id", pluss::make_series(std::vector<std::int64_t>{10, 11})},
      {"district_id", pluss::make_series(std::vector<std::int64_t>{1, 2})},
  });

  pluss::bundle::nexus::Relationship loan_account;
  loan_account.name = "loan_account";
  loan_account.left_table = "loan";
  loan_account.right_table = "account";
  loan_account.keys = {{"account_id", "account_id"}};
  loan_account.cardinality = pluss::bundle::nexus::RelationshipCardinality::ManyToOne;

  auto nx = pluss::bundle::Nexus({
                                     {"loan", loan},
                                     {"account", account},
                                 })
                .links({loan_account});

  expect(nx.check("loan_account"),
         "Baseline relationship validation should succeed before the phase-7 caches are warmed");
  expect(nx.path("loan", "account").steps.size() == 1,
         "Baseline direct path discovery should succeed before mutation");
  expect(nx.scan().features("computed_status").as_note().at(2) == "validated",
         "Baseline workspace scan should succeed before the runtime caches are warmed");

  nx.apply_("account", [](const DataFrame&) {
    return DataFrame({
        {"account_id", pluss::make_series(std::vector<std::int64_t>{10, 10})},
        {"district_id", pluss::make_series(std::vector<std::int64_t>{1, 2})},
    });
  });
  expect(!nx.check("loan_account"),
         "Relationship audit caches should invalidate after in-place frame mutation");
  expect(nx.scan().features("computed_status").as_note().at(2) == "invalid",
         "Workspace scan caches should invalidate after in-place frame mutation");

  auto renamed = pluss::bundle::Nexus({
                                           {"loan", loan},
                                           {"account", account},
                                       })
                     .links({loan_account});
  (void)renamed.path("loan", "account");
  renamed = renamed.store("sheet", renamed.plan("loan", "account"));
  renamed.rename_("account", "acct");
  expect(renamed.path("loan", "acct").steps.size() == 1,
         "Path caches should invalidate after in-place table renaming");
  expect_throw([&]() { (void)renamed.path("loan", "account"); },
               "Old path cache entries should not survive an in-place rename");
  expect(renamed.has_store("sheet") &&
             renamed.run("sheet").features("district_id").as_series().to_vec<std::int64_t>()[1] == 2,
         "Stored plans should rewrite their table references after in-place rename");
}

} // namespace

int main(int argc, char** argv) {
  using Test = std::pair<std::string, std::function<void()>>;
  const std::vector<Test> tests{
      {"comma_fixture_load_and_cases", test_comma_fixture_load_and_cases},
      {"utf8_bom_is_removed_from_first_header", test_utf8_bom_is_removed_from_first_header},
      {"auto_initializer_list_assignment_and_row_sum", test_auto_initializer_list_assignment_and_row_sum},
      {"dataframe_axis_reductions", test_dataframe_axis_reductions},
      {"axis_feature_assignment_alignment_and_shape", test_axis_feature_assignment_alignment_and_shape},
      {"semicolon_fixture_with_case_col", test_semicolon_fixture_with_case_col},
      {"bad_line_policy_and_locale_numeric_parsing", test_bad_line_policy_and_locale_numeric_parsing},
      {"usefeats_ncases_skipfooter_and_invalid_source", test_usefeats_ncases_skipfooter_and_invalid_source},
      {"display_engine_width_truncation_and_info", test_display_engine_width_truncation_and_info},
      {"html_display_renderers_and_backend_config", test_html_display_renderers_and_backend_config},
      {"atlas_html_display_renderer", test_atlas_html_display_renderer},
      {"describe_numeric_summary", test_describe_numeric_summary},
      {"view_windowing", test_view_windowing},
      {"dataframe_cast_accessor_surface", test_dataframe_cast_accessor_surface},
      {"dataframe_equals_and_reflected_logicals", test_dataframe_equals_and_reflected_logicals},
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
      {"temporal_save_csv_preserves_human_readable_dates", test_temporal_save_csv_preserves_human_readable_dates},
      {"temporal_string_compare_uses_timeline_semantics", test_temporal_string_compare_uses_timeline_semantics},
      {"temporal_save_reload_filter_matches_scalar_timeline", test_temporal_save_reload_filter_matches_scalar_timeline},
      {"dataframe_lvalue_name_range_surfaces", test_dataframe_lvalue_name_range_surfaces},
      {"atlas_and_nexus_lvalue_name_range_surfaces", test_atlas_and_nexus_lvalue_name_range_surfaces},
      {"transpose_shape_and_axes", test_transpose_shape_and_axes},
      {"squeeze_unsqueeze_bridge", test_squeeze_unsqueeze_bridge},
      {"parallel_engine_matches_serial_engine", test_parallel_engine_matches_serial_engine},
      {"auto_engine_policy_switches_between_parallel_and_serial", test_auto_engine_policy_switches_between_parallel_and_serial},
      {"dataframe_sql_accessor", test_dataframe_sql_accessor},
      {"nexus_query_sql_accessor", test_nexus_query_sql_accessor},
      {"versatile_dataframe_families", test_versatile_dataframe_families},
      {"replace_exact_and_null_overloads", test_replace_exact_and_null_overloads},
      {"select_and_place_helpers", test_select_and_place_helpers},
      {"series_nan_participates_in_table_missing_semantics", test_series_nan_participates_in_table_missing_semantics},
      {"dataframe_shownull_and_zoom", test_dataframe_shownull_and_zoom},
      {"thread_and_dataframe_freq_surfaces", test_thread_and_dataframe_freq_surfaces},
      {"thread_bucketize_correctness", test_thread_bucketize_correctness},
      {"thread_arrange_and_corr_stability", test_thread_arrange_and_corr_stability},
      {"corr_feature_projection_and_sort_chain", test_corr_feature_projection_and_sort_chain},
      {"timeline_sort_nat_ordering", test_timeline_sort_nat_ordering},
      {"atlas_label_accessor", test_atlas_label_accessor},
      {"nexus_basic_workspace_ops", test_nexus_basic_workspace_ops},
      {"nexus_delegation_and_atlas_interop", test_nexus_delegation_and_atlas_interop},
      {"nexus_metadata_and_relationship_graph", test_nexus_metadata_and_relationship_graph},
      {"nexus_validation_and_audit_layer", test_nexus_validation_and_audit_layer},
      {"nexus_relational_execution_helpers", test_nexus_relational_execution_helpers},
      {"nexus_query_surface", test_nexus_query_surface},
      {"nexus_pull_surface", test_nexus_pull_surface},
      {"nexus_workspace_insights_and_views", test_nexus_workspace_insights_and_views},
      {"nexus_persistence_and_alias_surface", test_nexus_persistence_and_alias_surface},
      {"nexus_raw_directory_loader", test_nexus_raw_directory_loader},
      {"nexus_cache_invalidation_on_mutation", test_nexus_cache_invalidation_on_mutation},
  };

  std::size_t passed = 0;
  for (const auto& [name, fn] : tests) {
    if (argc > 1) {
      bool selected = false;
      for (int arg = 1; arg < argc; ++arg) {
        selected = selected || name == argv[arg];
      }
      if (!selected) {
        continue;
      }
    }
    try {
      fn();
      ++passed;
      fmt::print("[PASS] {}\n", name);
    } catch (const std::exception& e) {
      fmt::print("[FAIL] {}\n{}\n", name, e.what());
      return 1;
    }
  }

  if (argc > 1 && passed == 0) {
    fmt::print("[FAIL] no requested test name matched\n");
    return 1;
  }
  fmt::print("{} tests passed\n", passed);
  return 0;
}
