#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "pluss/pluss.h"

namespace {

using pluss::table::DataFrame;
using pluss::table::Atlas;
using pluss::table::Axis;
using pluss::table::ScanStatistic;
using pluss::table::StatisticOptions;
using pluss::table::Thread;
using pluss::table::ThreadValue;
using pluss::table::TimeWindowStatistic;
using pluss::table::WindowClosure;
using pluss::table::WindowOptions;
using pluss::table::WindowStatistic;

void expect(bool value, std::string_view message) {
  if (!value) {
    throw std::runtime_error(std::string(message));
  }
}

void near(double actual, double expected, double tolerance, std::string_view message) {
  if (!std::isfinite(actual) || std::fabs(actual - expected) > tolerance) {
    throw std::runtime_error(
        std::string(message) + ": actual=" + std::to_string(actual) +
        ", expected=" + std::to_string(expected));
  }
}

DataFrame numeric_frame(pluss::codem::Platform platform = pluss::codem::Platform::CPU) {
  auto x = pluss::make_series(std::vector<float>{1, 2, 3, 4});
  auto y = pluss::make_series(std::vector<float>{2, 4, 6, 8});
  if (platform != pluss::codem::Platform::CPU) {
    x = x.to(platform);
    y = y.to(platform);
  }
  return DataFrame({
      {"x", Thread(x)},
      {"y", Thread(y)},
  });
}

void test_statistics() {
  const auto frame = numeric_frame();
  const auto cov = frame.covar();
  near(cov.features("x").as_series().to_vec<double>(0), 5.0 / 3.0, 1e-9,
       "sample covariance x/x");
  near(cov.features("y").as_series().to_vec<double>(0), 10.0 / 3.0, 1e-9,
       "sample covariance x/y");
  near(frame.covar({.ddof = 0, .min_count = 1})
           .features("x").as_series().to_vec<double>(0),
       1.25, 1e-9, "population covariance");
  const auto offset_cov = DataFrame({
      {"x", Thread(pluss::make_series(std::vector<double>{
                1.0e12 + 1.0, 1.0e12 + 2.0, 1.0e12 + 3.0, 1.0e12 + 4.0}))},
  }).covar();
  near(offset_cov.features("x").as_series().to_vec<double>(0),
       5.0 / 3.0, 1e-9, "large-offset covariance stability");

  const auto quantiles = frame.percentiles({0.0, 0.5, 1.0});
  expect(quantiles.ncases() == 3, "percentiles should emit one case per point");
  const auto qx = quantiles.features("x").as_series().to_vec<double>();
  near(qx[0], 1.0, 1e-9, "minimum percentile");
  near(qx[1], 2.5, 1e-9, "median percentile");
  near(qx[2], 4.0, 1e-9, "maximum percentile");

  const auto modes = DataFrame({
      {"x", Thread(pluss::make_series(std::vector<double>{3, 1, 3, 2}))},
  }).mode();
  near(modes.features("x").as_series().to_vec<double>(0), 3.0, 1e-9,
       "most frequent numeric value");
  const auto tied_modes = DataFrame({
      {"x", Thread(pluss::make_series(std::vector<double>{1, 1, 2, 2}))},
  }).mode();
  expect(tied_modes.ncases() == 2, "all tied modes should be returned");
  expect(tied_modes.features("x").as_series().to_vec<double>() ==
             std::vector<double>({1, 2}),
         "tied modes should be sorted");
  const auto time_mode = Thread(pluss::make_timeline({
      "2024-01-02T00:00:00Z",
      "2024-01-01T00:00:00Z",
      "2024-01-02T00:00:00Z",
  })).mode();
  expect(time_mode.kind() == pluss::table::ThreadKind::Timeline &&
             time_mode.size() == 1 &&
             time_mode.as_timeline().day().to_vec<std::int32_t>(0) == 2,
         "timeline mode preserves its Thread kind");
  const auto mixed_modes = DataFrame({
      {"x", Thread(pluss::make_series(
                std::vector<double>{1, 1, 2, 2}))},
      {"when", Thread(pluss::make_timeline({
                   "2024-01-02T00:00:00Z",
                   "2024-01-01T00:00:00Z",
                   "2024-01-02T00:00:00Z",
                   "2024-01-02T00:00:00Z",
               }))},
  }).mode();
  expect(mixed_modes.features("when").kind() ==
             pluss::table::ThreadKind::Timeline &&
             !mixed_modes.features("when").is_valid(1),
         "DataFrame mode pads time-like Threads without changing kind");

  const auto ranks = DataFrame({
      {"x", Thread(pluss::make_series(std::vector<double>{20, 10, 20}))},
  }).rank();
  const auto ranked = ranks.features("x").as_series().to_vec<double>();
  near(ranked[0], 2.5, 1e-9, "average tied rank");
  near(ranked[1], 1.0, 1e-9, "lowest rank");
  near(ranked[2], 2.5, 1e-9, "average tied rank repeated");

  near(frame.skew().as_series().to_vec<double>(0), 0.0, 1e-9,
       "unbiased skew");
  near(frame.kurtosis().as_series().to_vec<double>(0), -1.2, 1e-8,
       "unbiased excess kurtosis");
  const auto constant = DataFrame({
      {"x", Thread(pluss::make_series(
                std::vector<double>{1, 1, 1, 1}))},
  });
  near(constant.skew().as_series().to_vec<double>(0),
       0.0, 1e-12, "constant skewness");
  near(constant.kurtosis().as_series().to_vec<double>(0),
       0.0, 1e-12, "constant excess kurtosis");
  near(frame.se().as_series().to_vec<double>(0),
       std::sqrt(5.0 / 3.0) / 2.0, 1e-9, "standard error");
  near(frame.var(Axis::Feature, {.ddof = 0, .min_count = 4})
           .as_series().to_vec<double>(0),
       1.25, 1e-9, "configurable variance ddof");
  near(frame.std(Axis::Feature, {.ddof = 0, .min_count = 4})
           .as_series().to_vec<double>(0),
       std::sqrt(1.25), 1e-9, "configurable deviation ddof");

  const auto cumulative = frame.scan(ScanStatistic::Sum);
  expect(cumulative.features("x").as_series().to_vec<double>() ==
             std::vector<double>({1, 3, 6, 10}),
         "running sum values");
  expect(frame.scan(ScanStatistic::Product).features("x").as_series().to_vec<double>() ==
             std::vector<double>({1, 2, 6, 24}),
         "running product values");
  auto nullable_valid = pluss::Series::ones(
      {4}, pluss::codem::ScalarType::Bool);
  reinterpret_cast<bool*>(nullable_valid.raw_data())[1] = false;
  const auto nullable = DataFrame({
      {"x", Thread(
                pluss::make_series(std::vector<double>{1, 0, 3, 4}),
                nullable_valid)},
  });
  expect(nullable.scan(ScanStatistic::Sum, true).features("x").validity_mask()->to_vec<bool>() ==
             std::vector<bool>({true, false, true, true}),
         "skip-null cumulative validity resumes after a null");
  expect(nullable.scan(ScanStatistic::Sum, false).features("x").validity_mask()->to_vec<bool>() ==
             std::vector<bool>({true, false, false, false}),
         "strict cumulative validity stops after a null");

  const auto related = frame.corr_with(frame);
  near(related.as_series().to_vec<double>(0), 1.0, 1e-9,
       "corr_with self");

  const auto left = DataFrame({
      {"a", Thread(pluss::make_series(std::vector<double>{1, 3}))},
      {"b", Thread(pluss::make_series(std::vector<double>{2, 4}))},
  });
  const auto right = DataFrame({
      {"z", Thread(pluss::make_series(std::vector<double>{5, 6}))},
  });
  const auto product = left.matmul(right);
  expect(product.features("z").as_series().to_vec<double>() ==
             std::vector<double>({17, 39}),
         "matrix product values");
}

void test_windows() {
  const auto frame = numeric_frame();
  const auto moving = frame.ts().rolling(3, WindowStatistic::Mean);
  const auto mean = moving.features("x").as_series().to_vec<double>();
  near(mean[0], 1.0, 1e-9, "moving mean row 0");
  near(mean[1], 1.5, 1e-9, "moving mean row 1");
  near(mean[2], 2.0, 1e-9, "moving mean row 2");
  near(mean[3], 3.0, 1e-9, "moving mean row 3");

  const auto variance = frame.ts().rolling(
      2, WindowStatistic::Var, WindowOptions{.min_count = 2, .ddof = 1});
  expect(!variance.features("x").validity_mask()->to_vec<bool>(0),
         "moving variance min_count");
  near(variance.features("x").as_series().to_vec<double>(1), 0.5, 1e-9,
       "moving variance");

  const auto growing = frame.ts().expanding(WindowStatistic::Mean);
  near(growing.features("x").as_series().to_vec<double>(3), 2.5, 1e-9,
       "growing mean");
  const auto moving_error = frame.ts().rolling(
      2, WindowStatistic::Sem, WindowOptions{.min_count = 2, .ddof = 1});
  near(moving_error.features("x").as_series().to_vec<double>(1), 0.5, 1e-9,
       "moving standard error");
  const auto moving_skew = frame.ts().rolling(
      3, WindowStatistic::Skew, WindowOptions{.min_count = 3});
  near(moving_skew.features("x").as_series().to_vec<double>(2), 0.0, 1e-9,
       "moving skew");
  const auto moving_kurt = frame.ts().rolling(
      4, WindowStatistic::Kurtosis, WindowOptions{.min_count = 4});
  near(moving_kurt.features("x").as_series().to_vec<double>(3), -1.2, 1e-8,
       "moving kurtosis");
  near(frame.ts().expanding(WindowStatistic::Sem, {.ddof = 1, .min_count = 2})
           .features("x").as_series().to_vec<double>(3),
       std::sqrt(5.0 / 3.0) / 2.0, 1e-9, "growing standard error");
  near(frame.ts().expanding(WindowStatistic::Skew, {.ddof = 1, .min_count = 3}).features("x").as_series().to_vec<double>(3),
       0.0, 1e-9, "growing skew");
  near(frame.ts().expanding(WindowStatistic::Kurtosis, {.ddof = 1, .min_count = 4}).features("x").as_series().to_vec<double>(3),
       -1.2, 1e-8, "growing kurtosis");

  const auto exponential = frame.ts().ewm(0.5, WindowStatistic::Mean, true);
  near(exponential.features("x").as_series().to_vec<double>(1), 5.0 / 3.0, 1e-5,
       "adjusted exponential mean");
  const auto exponential_variance =
      frame.ts().ewm(0.5, WindowStatistic::Var, true, true).features("x");
  expect(!exponential_variance.validity_mask()->to_vec<bool>(0),
         "bias-corrected exponential variance needs two values");
  const auto exponential_variance_values =
      exponential_variance.as_series().to_vec<double>();
  near(exponential_variance_values[1], 0.5, 1e-9,
       "adjusted exponential variance row 1");
  near(exponential_variance_values[2], 0.9285714285714284, 1e-9,
       "adjusted exponential variance row 2");
  near(exponential_variance_values[3], 1.385714285714286, 1e-9,
       "adjusted exponential variance row 3");
  near(frame.ts().ewm(0.5, WindowStatistic::Var, false, true)
           .features("x").as_series().to_vec<double>(3),
       1.6904761904761905, 1e-9,
       "recursive exponential variance");

  const auto grouped_frame = DataFrame({
      {"group", Thread(pluss::make_note({"a", "b", "a", "b"}))},
      {"value", Thread(pluss::make_series(std::vector<double>{1, 10, 3, 20}))},
  });
  const auto grouped = grouped_frame.features("value").ts().grouped(
      grouped_frame.features("group"), 2, WindowStatistic::Mean);
  const auto grouped_values = grouped.as_series().to_vec<double>();
  near(grouped_values[0], 1.0, 1e-9, "grouped moving first a");
  near(grouped_values[1], 10.0, 1e-9, "grouped moving first b");
  near(grouped_values[2], 2.0, 1e-9, "grouped moving second a");
  near(grouped_values[3], 15.0, 1e-9, "grouped moving second b");
  const auto grouped_sum_frame = DataFrame({
      {"group", Thread(pluss::make_note({"a", "b", "a", "b"}))},
      {"value", Thread(pluss::make_series(std::vector<double>{1, 10, 3, 20}))},
  });
  const auto grouped_sum = grouped_sum_frame.features("value").ts().grouped(
      grouped_sum_frame.features("group"), 2, pluss::table::WindowStatistic::Sum);
  expect(grouped_sum.as_series().to_vec<double>() ==
             std::vector<double>({1, 10, 4, 30}),
         "grouped moving sum");

  const auto timed_frame = DataFrame({
      {"time", Thread(pluss::make_series(std::vector<std::int64_t>{0, 1, 3, 4}))},
      {"value", Thread(pluss::make_series(std::vector<double>{1, 2, 3, 4}))},
  });
  const auto timed = timed_frame.features("value").ts().timed(
      timed_frame.features("time"), 3, TimeWindowStatistic::Mean);
  const auto timed_values = timed.as_series().to_vec<double>();
  near(timed_values[0], 1.0, 1e-9, "time window row 0");
  near(timed_values[1], 1.5, 1e-9, "time window row 1");
  near(timed_values[2], 2.5, 1e-9, "time window excludes left boundary");
  near(timed_values[3], 3.5, 1e-9, "time window row 3");
  const auto timed_variance_frame = DataFrame({
      {"time", Thread(pluss::make_series(std::vector<std::int64_t>{0, 1, 3, 4}))},
      {"value", Thread(pluss::make_series(std::vector<double>{1, 2, 3, 4}))},
  });
  const auto timed_variance = timed_variance_frame.features("value").ts().timed(
      timed_variance_frame.features("time"), 3, pluss::table::TimeWindowStatistic::Var,
      StatisticOptions{.ddof = 1, .min_count = 2});
  near(timed_variance.as_series().to_vec<double>(2),
       0.5, 1e-9, "time moving variance");
}

void test_thread_delegation_and_advanced_windows() {
  const auto source =
      Thread(pluss::make_series(std::vector<double>{1, 2, 3, 4}));
  const auto frame = DataFrame({{"x", source}});

  near(source.skew().as_series().to_vec<double>(0),
       frame.skew().as_series().to_vec<double>(0), 1e-12,
       "DataFrame skew delegates to Thread");
  expect(source.scan(ScanStatistic::Sum).as_series().to_vec<double>() ==
             frame.scan(ScanStatistic::Sum).features("x").as_series().to_vec<double>(),
         "DataFrame cumulative operation delegates to Thread");
  expect(source.ts().rolling(2, WindowStatistic::Sum)
             .as_series().to_vec<double>() ==
             frame.ts().rolling(2, WindowStatistic::Sum).features("x").as_series().to_vec<double>(),
         "DataFrame .ts() fixed window delegates to Thread");

  const auto centered = source.ts().rolling(
      3, WindowStatistic::Mean,
      WindowOptions{.min_count = 1,
                    .ddof = 1,
                    .centered = true,
                    .closure = WindowClosure::Right});
  near(centered.as_series().to_vec<double>(0), 1.5, 1e-9,
       "centered window row 0");
  near(centered.as_series().to_vec<double>(2), 3.0, 1e-9,
       "centered window row 2");

  const auto left_closed = source.ts().rolling(
      2, WindowStatistic::Sum,
      WindowOptions{.min_count = 1,
                    .ddof = 1,
                    .centered = false,
                    .closure = WindowClosure::Left});
  expect(!left_closed.is_valid(0),
         "left-closed window excludes the current first case");
  near(left_closed.as_series().to_vec<double>(2), 3.0, 1e-9,
       "left-closed window contains prior cases");
  const auto both_closed_skew = source.ts().rolling(
      3, WindowStatistic::Skew,
      WindowOptions{.min_count = 3,
                    .ddof = 1,
                    .centered = false,
                    .closure = WindowClosure::Both});
  near(both_closed_skew.as_series().to_vec<double>(2), 0.0, 1e-9,
       "closed moment window");

  const auto weighted =
      source.weigh({1.0, 2.0}, WindowStatistic::Mean);
  near(weighted.as_series().to_vec<double>(1), 5.0 / 3.0, 1e-9,
       "weighted moving mean");
  const auto weighted_frame =
      frame.weigh({1.0, 2.0}, WindowStatistic::Mean);
  near(weighted_frame.features("x").as_series().to_vec<double>(3),
       11.0 / 3.0, 1e-9, "DataFrame weighted window delegation");

  const auto custom = source.bounded(
      [](std::int64_t size, const WindowOptions&) {
        std::vector<std::int64_t> starts(
            static_cast<std::size_t>(size), 0);
        std::vector<std::int64_t> ends;
        ends.reserve(static_cast<std::size_t>(size));
        for (std::int64_t row = 0; row < size; ++row) {
          ends.push_back(row + 1);
        }
        return std::pair{std::move(starts), std::move(ends)};
      },
      WindowStatistic::Mean);
  expect(custom.as_series().to_vec<double>() ==
             std::vector<double>({1, 1.5, 2, 2.5}),
         "custom indexer expanding mean");

  const auto both_time = Thread(
      pluss::make_series(std::vector<double>{1, 2, 3, 4}))
      .ts().timed(
          Thread(pluss::make_series(
              std::vector<std::int64_t>{0, 1, 3, 4})),
          3, pluss::table::TimeWindowStatistic::Sum,
          StatisticOptions{}, WindowClosure::Both);
  near(both_time.as_series().to_vec<double>(2), 6.0, 1e-9,
       "both-closed time window includes its left boundary");

  const auto ewm = source.ts().ewm(0.5, WindowStatistic::Mean, true, true, 3);
  expect(!ewm.is_valid(1) && ewm.is_valid(2),
         "EWM min_count is enforced");
}

void test_udfs_and_text() {
  const auto text = Thread(
      pluss::make_note({"alpha-10", "Beta-20", "red|blue"}));
  expect(text.upper().as_note().at(0) == "ALPHA-10",
         "text upper");
  expect(text.note().length().as_series().to_vec<std::int64_t>(0) == 8,
         "Unicode character length");
  expect(text.note().matches("[a-z]+").as_series().to_vec<bool>(0),
         "regex prefix match");
  expect(!text.note().fullmatch("[a-z]+").as_series().to_vec<bool>(0),
         "regex full match");
  bool invalid_regex_rejected = false;
  try {
    (void)text.note().matches("(");
  } catch (const pluss::Error&) {
    invalid_regex_rejected = true;
  }
  expect(invalid_regex_rejected,
         "invalid regex uses the centralized Pluss error path");
  const auto extracted =
      text.note().extract("([A-Za-z]+)-([0-9]+)", {"word", "number"});
  expect(extracted.features("word").as_note().at(1) == "Beta",
         "regex capture");
  const auto split = text.note().split("-");
  expect(split.features("part_1").as_note().at(0) == "10",
         "text split");
  const auto partition = text.note().partition("-");
  expect(partition.features("part_2").as_note().at(1) == "20",
         "text partition");
  expect(text.note().pad(10, pluss::table::TextAlign::Right, '.')
             .as_note().at(0) == "..alpha-10",
         "text padding");
  expect(text.note().chars(0, 5).as_note().at(0) == "alpha",
         "text slicing");
  expect(text.note().encode("hex").as_note().at(0) ==
             "616c7068612d3130",
         "hex encoding");
  const auto unicode = Thread(
      pluss::make_note({"Cafe\u0301", "Straße"}));
  expect(unicode.note().normalize(pluss::table::UnicodeForm::NFC)
             .as_note().at(0) == "Café",
         "canonical Unicode normalization");
  expect(unicode.casefold().as_note().at(1) == "strasse",
         "Unicode case folding");
  const auto indicators = text.note().indicate("|");
  expect(indicators.features("red").as_series().to_vec<bool>(2) &&
             indicators.features("blue").as_series().to_vec<bool>(2),
         "delimited text indicators");

  const auto mapped = Thread(
      pluss::make_series(std::vector<double>{1, 2, 3}))
      .map([](const ThreadValue& value) {
        return ThreadValue(Thread(value.as_series() * 2.0));
      });
  expect(mapped.as_series().to_vec<double>() ==
             std::vector<double>({2, 4, 6}),
         "Thread element UDF");
  auto map_validity = pluss::Series::ones(
      {3}, pluss::codem::ScalarType::Bool);
  reinterpret_cast<bool*>(map_validity.raw_data())[1] = false;
  std::int64_t callback_count = 0;
  const auto mapped_nullable = Thread(
      pluss::make_series(std::vector<double>{1, 0, 3}),
      map_validity).map([&](const ThreadValue& value) {
        ++callback_count;
        return ThreadValue(Thread(value.as_series() * 2.0));
      });
  expect(callback_count == 2 && !mapped_nullable.is_valid(1),
         "element UDF skips and preserves null values");

  const auto frame = DataFrame({
      {"x", Thread(pluss::make_series(std::vector<double>{1, 2, 3}))},
      {"label", Thread(pluss::make_note({"a", "bb", "ccc"}))},
  });
  const auto case_values = frame.transform([](const DataFrame& row) {
    return ThreadValue(Thread(
        row.features("x").as_series() * 10.0));
  }, Axis::Case);
  expect(case_values.as_series().to_vec<double>() ==
             std::vector<double>({10, 20, 30}),
         "case-wise UDF");
  const auto mapped_frame = frame.map(
      [](const ThreadValue& value) {
        return ThreadValue(Thread(value.as_series() * 3.0));
      },
      {"x"});
  expect(mapped_frame.features("x").as_series().to_vec<double>() ==
             std::vector<double>({3, 6, 9}) &&
             mapped_frame.features("label").as_note().at(1) == "bb",
         "DataFrame element UDF selects features");
  near(frame.fold([](const DataFrame& value) {
        return Thread(value.features("x").sum().as_series());
      }).as_series().to_vec<double>(0),
       6.0, 1e-9, "arbitrary aggregation UDF");
  expect(frame.flow([](const DataFrame& value) {
           return value.note("label").upper();
         }).features("label").as_note().at(1) == "BB",
         "chainable DataFrame flow");
  const auto preserved = frame.transform(
      [](const DataFrame& row) { return row; },
      Axis::Case, pluss::table::ResultShape::Preserve);
  expect(preserved.ncases() == frame.ncases() &&
             preserved.feature_names() == frame.feature_names(),
         "case apply preserve shape");
  const auto expanded = frame.transform(
      [](const DataFrame& row) {
        return row.extend_cases(row, pluss::table::FrameMatchRule::Unite,
                                true, false);
      },
      Axis::Case, pluss::table::ResultShape::Expand);
  expect(expanded.ncases() == frame.ncases() * 2,
         "case apply expand shape");

  const auto atlas = Atlas({frame, frame});
  const auto atlas_moving =
      atlas.ts({"x"}).rolling(2, WindowStatistic::Sum);
  expect(atlas_moving.depth() == 2 &&
             atlas_moving.frame(1).features("x")
                     .as_series().to_vec<double>() ==
                 std::vector<double>({1, 3, 5}),
         "Atlas delegates windows to DataFrame");
  expect(atlas.flow([](const Atlas& value) {
           return value.map_frames([](const DataFrame& part) {
             return part.note("label").upper();
           });
         }).frame(0).features("label").as_note().at(0) == "A",
         "chainable Atlas flow and DataFrame delegation");
}

void test_streamed_csv_schema() {
  const auto path =
      std::filesystem::temp_directory_path() / "plussmic_stream_schema.csv";
  {
    std::ofstream out(path);
    out << "value\n1\n2\nthree\n";
  }
  pluss::loader::CsvCaseReader reader(path.string(), 2);
  DataFrame batch;
  expect(reader.next(batch), "first CSV batch");
  bool rejected = false;
  try {
    (void)reader.next(batch);
  } catch (...) {
    rejected = true;
  }
  expect(rejected, "streaming reader must reject inferred schema drift");

  pluss::loader::ReadCsvOptions options;
  options.dtype["value"] = "string";
  pluss::loader::CsvCaseReader explicit_reader(path.string(), 2, options);
  expect(explicit_reader.next(batch), "explicit schema first batch");
  expect(explicit_reader.next(batch), "explicit schema second batch");
  expect(batch.features("value").kind() == pluss::table::ThreadKind::Note,
         "explicit streamed schema remains stable");
  std::filesystem::remove(path);

  const auto null_path =
      std::filesystem::temp_directory_path() / "plussmic_stream_null_schema.csv";
  {
    std::ofstream out(null_path);
    out << "value\n1\n2\nNA\nNA\n";
  }
  pluss::loader::CsvCaseReader null_reader(null_path.string(), 2);
  expect(null_reader.next(batch), "numeric schema batch");
  const auto first_dtype = batch.features("value").as_series().scalar_type();
  expect(null_reader.next(batch), "all-null batch should conform to prior schema");
  expect(batch.features("value").kind() == pluss::table::ThreadKind::Series &&
             batch.features("value").as_series().scalar_type() == first_dtype,
         "all-null batch should retain the established numeric schema");
  expect(batch.features("value").isnull().as_series().all().item<bool>(),
         "all-null batch remains null after schema conversion");
  std::filesystem::remove(null_path);
}

void test_mps_parity() {
  if (!pluss::codem::mps_backend_available()) {
    return;
  }
  const auto cpu = numeric_frame();
  const auto mps = numeric_frame(pluss::codem::Platform::MPS);
  const auto cpu_values =
      cpu.ts().rolling(3, WindowStatistic::Mean).features("x").as_series().to_vec<double>();
  const auto mps_result = mps.ts().rolling(3, WindowStatistic::Mean).features("x");
  expect(mps_result.as_series().platform() == pluss::codem::Platform::MPS,
         "moving window must remain on MPS");
  const auto mps_values =
      mps_result.as_series().to(pluss::codem::Platform::CPU).to_vec<float>();
  for (std::size_t i = 0; i < cpu_values.size(); ++i) {
    near(mps_values[i], cpu_values[i], 1e-5, "CPU/MPS moving mean parity");
  }
  const auto mps_cov = mps.covar().features("x");
  expect(mps_cov.as_series().platform() == pluss::codem::Platform::MPS,
         "covariance must remain on MPS");
  near(mps_cov.as_series().to(pluss::codem::Platform::CPU).to_vec<float>(0),
       5.0 / 3.0, 1e-4, "CPU/MPS covariance parity");
  const auto mps_correlation = mps.corr_with(mps);
  expect(mps_correlation.as_series().platform() ==
             pluss::codem::Platform::MPS,
         "feature correlations must remain on MPS");
  near(mps_correlation.as_series()
           .to(pluss::codem::Platform::CPU).to_vec<float>(0),
       1.0, 1e-5, "CPU/MPS feature correlation parity");

  const auto mps_percentiles = mps.percentiles({0.0, 0.5, 1.0}).features("x");
  expect(mps_percentiles.as_series().platform() == pluss::codem::Platform::MPS,
         "percentiles must return to MPS after the sorting fallback");
  near(mps_percentiles.as_series()
           .to(pluss::codem::Platform::CPU).to_vec<float>(1),
       2.5, 1e-5, "CPU/MPS percentile parity");

  const auto mps_running = mps.scan(ScanStatistic::Sum).features("x");
  expect(mps_running.as_series().platform() == pluss::codem::Platform::MPS,
         "cumulative statistics must remain on MPS");
  near(mps_running.as_series().to(pluss::codem::Platform::CPU).to_vec<float>(3),
       10.0, 1e-5, "CPU/MPS cumulative parity");

  const auto mps_skew = mps.skew();
  expect(mps_skew.as_series().platform() == pluss::codem::Platform::MPS,
         "skewness must remain on MPS");
  near(mps_skew.as_series().to(pluss::codem::Platform::CPU).to_vec<float>(0),
       0.0, 1e-5, "CPU/MPS skewness parity");
  const auto mps_variance =
      mps.var(Axis::Feature, {.ddof = 0, .min_count = 4});
  expect(mps_variance.as_series().platform() ==
             pluss::codem::Platform::MPS,
         "configurable variance must remain on MPS");
  near(mps_variance.as_series()
           .to(pluss::codem::Platform::CPU).to_vec<float>(0),
       1.25, 1e-5, "CPU/MPS configurable variance parity");

  const auto mps_moment = mps.ts().rolling(
      4, WindowStatistic::Kurtosis, WindowOptions{.min_count = 4}).features("x");
  expect(mps_moment.as_series().platform() == pluss::codem::Platform::MPS,
         "moving moments must remain on MPS");
  near(mps_moment.as_series().to(pluss::codem::Platform::CPU).to_vec<float>(3),
       -1.2, 1e-4, "CPU/MPS moving moment parity");

  const auto mps_exponential = mps.ts().ewm(0.5, WindowStatistic::Mean).features("x");
  expect(mps_exponential.as_series().platform() == pluss::codem::Platform::MPS,
         "exponential windows must remain on MPS");
  near(mps_exponential.as_series()
           .to(pluss::codem::Platform::CPU).to_vec<float>(1),
       5.0 / 3.0, 1e-4, "CPU/MPS exponential parity");

  const auto grouped_mps_frame = DataFrame({
      {"group", Thread(pluss::make_note({"a", "b", "a", "b"}))},
      {"value", Thread(pluss::make_series(
                    std::vector<float>{1, 10, 3, 20}).to(pluss::codem::Platform::MPS))},
  });
  const auto grouped_mps = grouped_mps_frame.features("value").ts().grouped(
      grouped_mps_frame.features("group"), 2, pluss::table::WindowStatistic::Sum);
  expect(grouped_mps.as_series().platform() ==
             pluss::codem::Platform::MPS,
         "grouped windows must remain on MPS");
  near(grouped_mps.as_series()
           .to(pluss::codem::Platform::CPU).to_vec<float>(3),
       30.0, 1e-4, "CPU/MPS grouped window parity");

  const auto timed_mps_frame = DataFrame({
      {"time", Thread(pluss::make_series(std::vector<std::int64_t>{0, 1, 3, 4}))},
      {"value", Thread(pluss::make_series(
                    std::vector<float>{1, 2, 3, 4}).to(pluss::codem::Platform::MPS))},
  });
  const auto timed_mps = timed_mps_frame.features("value").ts().timed(
      timed_mps_frame.features("time"), 3, pluss::table::TimeWindowStatistic::Mean);
  expect(timed_mps.as_series().platform() ==
             pluss::codem::Platform::MPS,
         "time windows must remain on MPS");
  near(timed_mps.as_series()
           .to(pluss::codem::Platform::CPU).to_vec<float>(3),
       3.5, 1e-4, "CPU/MPS time window parity");
}

} // namespace

int main() {
  try {
    test_statistics();
    test_windows();
    test_thread_delegation_and_advanced_windows();
    test_udfs_and_text();
    test_streamed_csv_schema();
    test_mps_parity();
    std::cout << "statistics/window tests passed\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
