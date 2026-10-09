#include <pluss/ai/lymph.h>
#include <pluss/mic.h>

#include <iostream>
#include <vector>

#ifdef PLUSS_AI_HAS_CATCH2
#include <catch2/catch_test_macros.hpp>
#endif

namespace {

using pluss::Config;
using pluss::ScalarType;
using pluss::Series;
using pluss::table::DataFrame;
using pluss::table::Slate;
using namespace pluss::ai::lymph;

Config f64() {
  return Config{}.with_dtype(ScalarType::Float64).with_platform(pluss::pCPU);
}

DataFrame frame() {
  const Series x = pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8}, {8}, f64());
  const Series y = pluss::make_series(std::vector<double>{0, 0, 0, 0, 1, 1, 1, 1}, {8}, f64());
  return DataFrame({
      {"x", Slate(x)},
      {"y", Slate(y)},
  });
}

DataFrame timed_frame() {
  const Series value = pluss::make_series(std::vector<double>{40, 10, 80, 20, 60, 30, 70, 50}, {8}, f64());
  return DataFrame({
      {"stamp", Slate(pluss::make_timeline({
                    "2024-01-04T00:00:00Z",
                    "2024-01-01T00:00:00Z",
                    "2024-01-08T00:00:00Z",
                    "2024-01-02T00:00:00Z",
                    "2024-01-06T00:00:00Z",
                    "2024-01-03T00:00:00Z",
                    "2024-01-07T00:00:00Z",
                    "2024-01-05T00:00:00Z",
                }))},
      {"value", Slate(value)},
  });
}

LabeledBatch batch() {
  const DataFrame data = frame();
  return resolve(labeled_batch_from_dataframe(data, DataFrameBatchOptions{.label_feature = "y"}),
                 "split_surface.labeled_batch_from_dataframe");
}

bool run_split_checks(std::ostream* err) {
  const DataFrame data = frame();
  const LabeledBatch labeled = batch();

  const prep::HoldoutSplit frame_holdout = prep::holdout_split(data, prep::HoldoutSplitOptions{.test_size = 0.25});
  if (!frame_holdout.has_frame() || frame_holdout.has_batch()
      || frame_holdout.train_set.ncases() != 6
      || frame_holdout.test_set.ncases() != 2) {
    if (err) *err << "holdout_split(frame) should populate frame fields only\n";
    return false;
  }

  prep::HoldoutSplitOptions batch_options{};
  batch_options.test_size = 0.25;
  batch_options.stratify = labeled.y;
  const prep::HoldoutSplit batch_holdout = prep::holdout_split(labeled, batch_options);
  if (batch_holdout.has_frame() || !batch_holdout.has_batch()
      || batch_holdout.train_batch.features.sample_count() != 6
      || batch_holdout.test_batch.features.sample_count() != 2) {
    if (err) *err << "holdout_split(batch) should return HoldoutSplit with batch fields populated\n";
    return false;
  }

  prep::TimeSeriesSplitOptions ts{};
  ts.folds = 2;
  ts.test_size = 2;
  ts.gap = 0;
  const std::vector<prep::TimeSeriesSplit> frame_ts = prep::timeseries_split(data, ts);
  const std::vector<prep::TimeSeriesSplit> batch_ts = prep::timeseries_split(labeled, ts);
  if (frame_ts.size() != 2 || batch_ts.size() != 2
      || !frame_ts.front().has_frame() || frame_ts.front().has_batch()
      || batch_ts.front().has_frame() || !batch_ts.front().has_batch()
      || frame_ts.back().test_set.ncases() != 2
      || batch_ts.back().test_batch.features.sample_count() != 2) {
    if (err) *err << "time_series_split should materialize frame and labeled-batch temporal folds\n";
    return false;
  }

  const DataFrame timed = timed_frame();
  const std::vector<prep::TimeSeriesSplit> ordered_ts = prep::timeseries_split(timed, "stamp", ts);
  if (ordered_ts.size() != 2 || ordered_ts.back().test_set.ncases() != 2) {
    if (err) *err << "timeseries_split(frame,time_feature,...) should produce the requested number of ordered folds\n";
    return false;
  }
  const auto ordered_values = ordered_ts.back().test_set.features("value").as_series().to_vec<double>();
  if (ordered_values.size() != 2 || ordered_values[0] != 70.0 || ordered_values[1] != 80.0) {
    if (err) *err << "timeseries_split(frame,time_feature,...) should sort by the temporal feature before taking the last fold\n";
    return false;
  }
  const prep::TimeSeriesSplit ordered_holdout = prep::time_holdout(timed, "stamp", ts);
  const auto holdout_values = ordered_holdout.test_set.features("value").as_series().to_vec<double>();
  if (holdout_values.size() != 2 || holdout_values[0] != 70.0 || holdout_values[1] != 80.0) {
    if (err) *err << "time_holdout(frame,time_feature,...) should return the last chronological block\n";
    return false;
  }

  return true;
}

} // namespace

#ifdef PLUSS_AI_HAS_CATCH2
TEST_CASE("lymph split surfaces carry frame and batch payloads", "[lymph][split]") {
  REQUIRE(run_split_checks(&std::cerr));
}
#else
int main() {
  if (!run_split_checks(&std::cerr)) {
    return 1;
  }
  std::cout << "lymph split surface checks passed\n";
  return 0;
}
#endif
