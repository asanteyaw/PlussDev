#include "table/DataFrame.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using pluss::ScalarType;
using pluss::Series;
using pluss::table::DataFrame;
using pluss::table::Thread;
using pluss::table::TsAlignMode;
using pluss::table::TsReindexFill;

void expect(bool ok, const std::string& message) {
  if (!ok) {
    throw std::runtime_error(message);
  }
}

std::vector<double> to_f64(const Thread& t) {
  return t.as_series().to(pluss::pCPU, ScalarType::Float64).reshape({t.as_series().numel()}).to_vec<double>();
}

void test_dataframe_ts_one_feature_and_set() {
  DataFrame df({
      {"Adj Close", Thread(pluss::make_series(std::vector<double>{100.0, 110.0, 121.0, 133.1}))},
  });

  auto lr = df.ts("Adj Close").log_returns();
  expect(lr.nfeatures() == 1, "log_returns should return one feature for one selected column");
  const auto vals = to_f64(lr.features("Adj Close"));
  expect(std::isnan(vals[0]), "log_returns first value should be NaN");
  expect(std::abs(vals[1] - std::log(1.1)) < 1e-12, "log_returns value mismatch");

  DataFrame out = df;
  out.set("Log Returns") = df.ts("Adj Close").log_returns();
  expect(out.has_feature("Log Returns"), "set(name)=df.ts(...).log_returns() should create feature");
}

void test_dataframe_ts_resample_reindex_blocks() {
  const std::vector<std::string_view> iso{
      "2024-01-01T00:00:00Z",
      "2024-01-01T12:00:00Z",
      "2024-01-02T00:00:00Z",
      "2024-01-02T12:00:00Z",
  };
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(iso.size())};
  const auto timeline = pluss::codeh::ops_internal::timeline_from_utf8(
      std::span<const std::string_view>(iso.data(), iso.size()),
      std::span<const std::int64_t>(shape.data(), shape.size()));

  DataFrame df({
      {"Adj Close", Thread(pluss::make_series(std::vector<double>{1.0, 3.0, 2.0, 4.0}))},
      {"Volume", Thread(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0}))},
      {"date", Thread(timeline)},
  });

  auto daily = df.ts({"Adj Close", "Volume"}).resample("date", "D", pluss::subsp::stats::ts::ResampleOp::Mean);
  expect(daily.ncases() == 2, "daily resample should produce 2 bins");
  expect(daily.nfeatures() == 2, "daily resample should keep selected features");

  const std::vector<std::string_view> target_iso{
      "2024-01-01T00:00:00Z",
      "2024-01-01T06:00:00Z",
      "2024-01-01T12:00:00Z",
      "2024-01-01T18:00:00Z",
      "2024-01-02T00:00:00Z",
      "2024-01-02T06:00:00Z",
      "2024-01-02T12:00:00Z",
  };
  const std::array<std::int64_t, 1> t_shape{static_cast<std::int64_t>(target_iso.size())};
  const auto target_timeline = pluss::codeh::ops_internal::timeline_from_utf8(
      std::span<const std::string_view>(target_iso.data(), target_iso.size()),
      std::span<const std::int64_t>(t_shape.data(), t_shape.size()));

  auto reindexed = df.ts("Adj Close").reindex_time("date", Thread(target_timeline), TsAlignMode::Exact, TsReindexFill::Forward);
  expect(reindexed.ncases() == static_cast<std::int64_t>(target_iso.size()), "reindexed row count mismatch");

  auto lag_block = df.ts({"Adj Close", "Volume"}).lag_block({1, 2});
  expect(lag_block.nfeatures() == 4, "lag_block should emit feature x lag cartesian output");

  auto rolling_block = df.ts({"Adj Close"}).rolling_block({2, 3});
  expect(rolling_block.nfeatures() == 4, "rolling_block default should emit mean/std per window");

  auto cal = df.ts("Adj Close").calendar("date");
  expect(cal.has_feature("date_year"), "calendar should emit date_year");
  expect(cal.has_feature("date_weekday"), "calendar should emit date_weekday");
}

} // namespace

int main() {
  try {
    test_dataframe_ts_one_feature_and_set();
    test_dataframe_ts_resample_reindex_blocks();
    std::cout << "table_ts_accessor_tests passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "table_ts_accessor_tests failed: " << e.what() << '\n';
    return 1;
  }
}
