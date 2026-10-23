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
using pluss::table::Slate;
using pluss::table::SlateCastOptions;
using pluss::table::SlateCastTarget;
using pluss::table::SlateKind;
using pluss::table::TsAlignMode;
using pluss::table::TsOptions;
using pluss::table::TsReindexFill;
using pluss::table::WindowStatistic;

void expect(bool ok, const std::string& message) {
  if (!ok) {
    throw std::runtime_error(message);
  }
}

template <typename Fn>
void expect_throw(Fn&& fn, const std::string& message) {
  bool threw = false;
  try {
    fn();
  } catch (...) {
    threw = true;
  }
  expect(threw, message);
}

std::vector<double> to_f64(const Slate& t) {
  return t.as_series().to(pluss::pCPU, ScalarType::Float64).reshape({t.as_series().numel()}).to_vec<double>();
}

void test_dataframe_ts_one_feature_and_set() {
  DataFrame df({
      {"Adj Close", Slate(pluss::make_series(std::vector<double>{100.0, 110.0, 121.0, 133.1}))},
  });

  auto lr = df.features({"Adj Close"}).ts().log_returns();
  expect(lr.nfeatures() == 1, "log_returns should return one feature for one selected column");
  const auto vals = to_f64(lr.features("Adj Close"));
  expect(std::isnan(vals[0]), "log_returns first value should be NaN");
  expect(std::abs(vals[1] - std::log(1.1)) < 1e-12, "log_returns value mismatch");

  DataFrame out = df;
  out.set("Log Returns") = df.features({"Adj Close"}).ts().log_returns();
  expect(out.get("Log Returns").has_value(), "set(name)=df.features(...).ts().log_returns() should create feature");
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
      {"Adj Close", Slate(pluss::make_series(std::vector<double>{1.0, 3.0, 2.0, 4.0}))},
      {"Volume", Slate(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0}))},
      {"date", Slate(timeline)},
  });

  const auto ts_opts = TsOptions{.axis = df.features("date")};
  auto daily = df.features({"Adj Close", "Volume"}).ts(ts_opts).resample("D", pluss::subsp::stats::ts::ResampleOp::Mean);
  auto daily_chain = df.features({"Adj Close", "Volume"}).ts(ts_opts).resample("D").mean();
  expect(daily.ncases() == 2, "daily resample should produce 2 bins");
  expect(daily.nfeatures() == 2, "daily resample should keep selected features");
  const auto daily_adj = to_f64(daily.features("Adj Close"));
  const auto daily_vol = to_f64(daily.features("Volume"));
  const auto daily_chain_adj = to_f64(daily_chain.features("Adj Close"));
  const auto daily_chain_vol = to_f64(daily_chain.features("Volume"));
  expect(daily_adj.size() == 2 && std::abs(daily_adj[0] - 2.0) < 1e-12 && std::abs(daily_adj[1] - 3.0) < 1e-12,
         "daily mean resample should match bin averages");
  expect(daily_vol.size() == 2 && std::abs(daily_vol[0] - 15.0) < 1e-12 && std::abs(daily_vol[1] - 35.0) < 1e-12,
         "daily mean resample should keep per-feature aggregation");
  expect(daily_chain_adj == daily_adj && daily_chain_vol == daily_vol,
         "chained resample(...).mean() should match explicit ResampleOp::Mean");

  DataFrame counted({
      {"Adj Close", Slate(pluss::make_series(std::vector<double>{
                        1.0,
                        std::numeric_limits<double>::quiet_NaN(),
                        2.0,
                        4.0,
                    }))},
      {"date", Slate(timeline)},
  });
  auto daily_count = counted.features({"Adj Close"})
                         .ts({.axis = counted.features("date")})
                         .resample("D", pluss::subsp::stats::ts::ResampleOp::Count);
  const auto counts = daily_count.features("Adj Close").as_series().to(pluss::pCPU, ScalarType::Int64).to_vec<std::int64_t>();
  expect(counts.size() == 2 && counts[0] == 1 && counts[1] == 2,
         "count resample should exclude missing values inside each bin");

  auto six_hour_exact = df.features({"Adj Close"}).ts(ts_opts).resample("6H").asfreq();
  const auto six_hour_exact_vals = to_f64(six_hour_exact.features("Adj Close"));
  expect(six_hour_exact_vals.size() == 7, "asfreq should materialize the resampled axis");
  expect(std::abs(six_hour_exact_vals[0] - 1.0) < 1e-12
             && std::isnan(six_hour_exact_vals[1])
             && std::abs(six_hour_exact_vals[2] - 3.0) < 1e-12
             && std::isnan(six_hour_exact_vals[3])
             && std::abs(six_hour_exact_vals[4] - 2.0) < 1e-12
             && std::isnan(six_hour_exact_vals[5])
             && std::abs(six_hour_exact_vals[6] - 4.0) < 1e-12,
         "asfreq should behave like an exact reindex onto the target frequency");

  auto six_hour_fill = df.features({"Adj Close"}).ts(ts_opts).resample("6H").asfreq(0.0);
  const auto six_hour_fill_vals = to_f64(six_hour_fill.features("Adj Close"));
  expect(six_hour_fill_vals.size() == 7
             && std::abs(six_hour_fill_vals[1] - 0.0) < 1e-12
             && std::abs(six_hour_fill_vals[3] - 0.0) < 1e-12
             && std::abs(six_hour_fill_vals[5] - 0.0) < 1e-12,
         "asfreq(fill_value) should fill only the newly introduced timestamps");

  auto six_hour_ffill = df.features({"Adj Close"}).ts(ts_opts).resample("6H").ffill();
  const auto six_hour_ffill_vals = to_f64(six_hour_ffill.features("Adj Close"));
  expect(six_hour_ffill_vals.size() == 7
             && std::abs(six_hour_ffill_vals[0] - 1.0) < 1e-12
             && std::abs(six_hour_ffill_vals[1] - 1.0) < 1e-12
             && std::abs(six_hour_ffill_vals[2] - 3.0) < 1e-12
             && std::abs(six_hour_ffill_vals[3] - 3.0) < 1e-12
             && std::abs(six_hour_ffill_vals[4] - 2.0) < 1e-12
             && std::abs(six_hour_ffill_vals[5] - 2.0) < 1e-12
             && std::abs(six_hour_ffill_vals[6] - 4.0) < 1e-12,
         "ffill should use the last observed source value at or before each resampled timestamp");

  auto six_hour_bfill = df.features({"Adj Close"}).ts(ts_opts).resample("6H").bfill();
  const auto six_hour_bfill_vals = to_f64(six_hour_bfill.features("Adj Close"));
  expect(six_hour_bfill_vals.size() == 7
             && std::abs(six_hour_bfill_vals[0] - 1.0) < 1e-12
             && std::abs(six_hour_bfill_vals[1] - 3.0) < 1e-12
             && std::abs(six_hour_bfill_vals[2] - 3.0) < 1e-12
             && std::abs(six_hour_bfill_vals[3] - 2.0) < 1e-12
             && std::abs(six_hour_bfill_vals[4] - 2.0) < 1e-12
             && std::abs(six_hour_bfill_vals[5] - 4.0) < 1e-12
             && std::abs(six_hour_bfill_vals[6] - 4.0) < 1e-12,
         "bfill should use the next observed source value at or after each resampled timestamp");

  auto weekly_max = df.features({"Adj Close", "Volume"}).ts(ts_opts).resample("W").max();
  expect(weekly_max.ncases() == 1, "weekly resample should group the sample into one weekly bin");
  const auto weekly_adj = to_f64(weekly_max.features("Adj Close"));
  const auto weekly_vol = to_f64(weekly_max.features("Volume"));
  expect(weekly_adj.size() == 1 && std::abs(weekly_adj[0] - 4.0) < 1e-12,
         "weekly max resample should honor W frequency aliases");
  expect(weekly_vol.size() == 1 && std::abs(weekly_vol[0] - 40.0) < 1e-12,
         "weekly max resample should aggregate each selected feature");

  auto monthly_max = df.features({"Adj Close"}).ts(ts_opts).resample("M").max();
  expect(monthly_max.ncases() == 1, "monthly resample should group the sample into one monthly bin");
  const auto monthly_adj = to_f64(monthly_max.features("Adj Close"));
  expect(monthly_adj.size() == 1 && std::abs(monthly_adj[0] - 4.0) < 1e-12,
         "monthly max resample should honor M frequency aliases");

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

  auto reindexed = df.features({"Adj Close"})
                       .ts(ts_opts)
                       .align_to(Slate(target_timeline), TsAlignMode::Exact, TsReindexFill::Forward);
  expect(reindexed.ncases() == static_cast<std::int64_t>(target_iso.size()), "reindexed row count mismatch");

  auto timed_daily = df.features({"Adj Close"}).ts(ts_opts).timed(
      24LL * 60LL * 60LL * 1'000'000'000LL,
      pluss::table::TimeWindowStatistic::Sum,
      {},
      pluss::table::WindowClosure::Right);
  expect(to_f64(timed_daily.features("Adj Close")) ==
             std::vector<double>({1.0, 4.0, 5.0, 6.0}),
         "features(...).ts({.axis}).timed(duration_nanos, ...) should delegate timed windows over the bound temporal axis");

  DataFrame timed_numeric({
      {"time_nanos", Slate(pluss::make_series(std::vector<std::int64_t>{0, 1, 3, 4}))},
      {"value", Slate(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}))},
  });
  auto timed_numeric_df = timed_numeric.features({"value"}).ts().timed(
      timed_numeric.features("time_nanos"),
      3,
      pluss::table::TimeWindowStatistic::Mean);
  expect(to_f64(timed_numeric_df.features("value")) ==
             std::vector<double>({1.0, 1.5, 2.5, 3.5}),
         "features(...).ts().timed(axis, duration_nanos, ...) should accept an explicit Int64 nanosecond axis");

  auto lag_block = df.features({"Adj Close", "Volume"}).ts().lag_block({1, 2});
  expect(lag_block.nfeatures() == 4, "lag_block should emit feature x lag cartesian output");

  auto rolling_block = df.features({"Adj Close"}).ts().rolling_block({2, 3});
  expect(rolling_block.nfeatures() == 4, "rolling_block default should emit mean/std per window");

  auto cal = df.features({"Adj Close"}).ts(ts_opts).calendar();
  expect(cal.get("axis_year").has_value(), "calendar should emit axis_year");
  expect(cal.get("axis_weekday").has_value(), "calendar should emit axis_weekday");

  const std::vector<std::string_view> grouped_iso{
      "2024-01-01T00:00:00Z",
      "2024-01-02T00:00:00Z",
      "2024-01-01T00:00:00Z",
      "2024-01-03T00:00:00Z",
  };
  const std::array<std::int64_t, 1> grouped_shape{static_cast<std::int64_t>(grouped_iso.size())};
  const auto grouped_timeline = pluss::codeh::ops_internal::timeline_from_utf8(
      std::span<const std::string_view>(grouped_iso.data(), grouped_iso.size()),
      std::span<const std::int64_t>(grouped_shape.data(), grouped_shape.size()));
  DataFrame grouped_roll({
      {"date", Slate(grouped_timeline)},
      {"group", Slate(pluss::make_note({"a", "a", "b", "a"}))},
      {"cohort", Slate(pluss::make_note({"x", "x", "x", "y"}))},
      {"value", Slate(pluss::make_series(std::vector<double>{1.0, 3.0, 10.0, 5.0}))},
  });
  auto grouped_timed = grouped_roll.features({"value"})
                           .ts({.axis = grouped_roll.features("date"),
                                .group = grouped_roll.features("group")})
                           .rolling("2D", WindowStatistic::Mean);
  const auto grouped_timed_values = to_f64(grouped_timed.features("value"));
  expect(grouped_timed_values.size() == 4
             && std::abs(grouped_timed_values[0] - 1.0) < 1e-12
             && std::abs(grouped_timed_values[1] - 2.0) < 1e-12
             && std::abs(grouped_timed_values[2] - 10.0) < 1e-12
             && std::abs(grouped_timed_values[3] - 4.0) < 1e-12,
         "features(...).ts({.axis, .group}).rolling(duration) should roll within groups");
  auto grouped_timed_multi = grouped_roll.features({"value"})
                                 .ts({.axis = grouped_roll.features("date"),
                                      .group = grouped_roll.features({"group", "cohort"})})
                                 .rolling("2D", WindowStatistic::Mean);
  const auto grouped_timed_multi_values = to_f64(grouped_timed_multi.features("value"));
  expect(grouped_timed_multi_values.size() == 4
             && std::abs(grouped_timed_multi_values[0] - 1.0) < 1e-12
             && std::abs(grouped_timed_multi_values[1] - 2.0) < 1e-12
             && std::abs(grouped_timed_multi_values[2] - 10.0) < 1e-12
             && std::abs(grouped_timed_multi_values[3] - 5.0) < 1e-12,
         "features(...).ts({.axis, .group = multi-feature DataFrame}).rolling(duration) should roll within composite groups");
  auto grouped_fixed_multi = grouped_roll.features({"value"})
                                 .ts({.group = grouped_roll.features({"group", "cohort"})})
                                 .grouped(2, WindowStatistic::Mean);
  const auto grouped_fixed_multi_values = to_f64(grouped_fixed_multi.features("value"));
  expect(grouped_fixed_multi_values.size() == 4
             && std::abs(grouped_fixed_multi_values[0] - 1.0) < 1e-12
             && std::abs(grouped_fixed_multi_values[1] - 2.0) < 1e-12
             && std::abs(grouped_fixed_multi_values[2] - 10.0) < 1e-12
             && std::abs(grouped_fixed_multi_values[3] - 5.0) < 1e-12,
         "features(...).ts({.group = multi-feature DataFrame}).grouped(window) should roll within composite groups");

  const std::vector<std::string_view> unsorted_iso{
      "2024-01-03T00:00:00Z",
      "2024-01-01T00:00:00Z",
      "2024-01-02T00:00:00Z",
  };
  const std::array<std::int64_t, 1> unsorted_shape{static_cast<std::int64_t>(unsorted_iso.size())};
  const auto unsorted_timeline = pluss::codeh::ops_internal::timeline_from_utf8(
      std::span<const std::string_view>(unsorted_iso.data(), unsorted_iso.size()),
      std::span<const std::int64_t>(unsorted_shape.data(), unsorted_shape.size()));
  DataFrame unsorted({
      {"date", Slate(unsorted_timeline)},
      {"value", Slate(pluss::make_series(std::vector<double>{30.0, 10.0, 20.0}))},
  });
  const auto sorted_df = unsorted.features({"value"})
                             .ts({.axis = unsorted.features("date")})
                             .sort();
  expect(to_f64(sorted_df.features("value")) == std::vector<double>({10.0, 20.0, 30.0}),
         "features(...).ts({.axis}).sort() should batch-delegate sorting by the time axis");
  const auto sorted_df_iso = sorted_df.labels().values().tl().iso();
  expect(sorted_df_iso.as_note().vector() ==
             std::vector<std::string_view>({
                 "2024-01-01T00:00:00.000000000+00:00",
                 "2024-01-02T00:00:00.000000000+00:00",
                 "2024-01-03T00:00:00.000000000+00:00",
             }),
         "features(...).ts({.axis}).sort() should carry the sorted time axis as case labels");

  const auto sorted_slate = unsorted.features("value").ts().sort(unsorted.features("date"));
  expect(to_f64(sorted_slate) == std::vector<double>({10.0, 20.0, 30.0}),
         "Slate::ts().sort(axis) should sort values by the supplied time axis");
  const auto sorted_slate_iso = sorted_slate.label().values().tl().iso();
  expect(sorted_slate_iso.as_note().vector() ==
             std::vector<std::string_view>({
                 "2024-01-01T00:00:00.000000000+00:00",
                 "2024-01-02T00:00:00.000000000+00:00",
                 "2024-01-03T00:00:00.000000000+00:00",
             }),
         "Slate::ts().sort(axis) should preserve the sorted time axis as labels");

  DataFrame lhs_frame({
      {"x", Slate(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}))},
  }, Slate(unsorted_timeline));
  DataFrame rhs_frame({
      {"x", Slate(pluss::make_series(std::vector<double>{15.0, 20.0, 25.0}))},
  }, Slate(pluss::codeh::ops_internal::timeline_from_utf8(
         std::span<const std::string_view>(
             std::array<std::string_view, 3>{
                 "2024-01-01T00:00:00Z",
                 "2024-01-02T00:00:00Z",
                 "2024-01-03T00:00:00Z",
             }
                 .data(),
             3),
         std::span<const std::int64_t>(
             std::array<std::int64_t, 1>{3}.data(),
             1))));
  const auto frame_ts_gt = lhs_frame.features({"x"}).ts().gt(rhs_frame);
  expect(frame_ts_gt.features("x").as_series().to_vec<bool>() ==
             std::vector<bool>({false, true, true}),
         "features(...).ts().gt(frame) should align on temporal case labels before comparison");
}

void test_dataframe_cast_timeline_from_yyyymmdd_ints() {
  DataFrame df({
      {"Date", Slate(pluss::make_series(std::vector<std::int64_t>{19900102, 19900103, 20000229}))},
  });

  const auto out = df.features({"Date"}).cast().timeline();
  const auto date_slate = out.features("Date");
  expect(date_slate.kind() == SlateKind::Timeline, "cast(Timeline) should materialize Timeline for YYYYMMDD integers");

  const auto tl = date_slate.as_timeline();
  const auto years = Slate(tl.year()).as_series().to(pluss::pCPU, ScalarType::Int64).to_vec<std::int64_t>();
  const auto months = Slate(tl.month()).as_series().to(pluss::pCPU, ScalarType::Int64).to_vec<std::int64_t>();
  const auto days = Slate(tl.day()).as_series().to(pluss::pCPU, ScalarType::Int64).to_vec<std::int64_t>();

  expect(years.size() == 3, "timeline year vector size mismatch");
  expect(years[0] == 1990 && months[0] == 1 && days[0] == 2, "first YYYYMMDD conversion mismatch");
  expect(years[1] == 1990 && months[1] == 1 && days[1] == 3, "second YYYYMMDD conversion mismatch");
  expect(years[2] == 2000 && months[2] == 2 && days[2] == 29, "leap-day YYYYMMDD conversion mismatch");
}

void test_dataframe_cast_pair_payload_targets() {
  DataFrame raw({
      {"id", Slate(pluss::make_series(std::vector<std::int64_t>{1, 2, 3}))},
      {"amount_text", Slate(pluss::make_note({"10.5", "Alice", "30"}))},
      {"event_text", Slate(pluss::make_note({
          "2024-01-01T00:00:00Z",
          "2024-01-02T00:00:00Z",
          "2024-01-03T00:00:00Z",
      }))},
  });

  auto casted = raw;
  casted.set("id") = raw.features("id").cast().note();
  casted.set("amount_text") = raw.features("amount_text").cast().series();
  casted.set("event_text") = raw.features("event_text").cast().timeline();

  expect(casted.features("id").kind() == SlateKind::Note,
         "cast(pair target): Series -> Note should change payload kind");
  expect(casted.features("amount_text").kind() == SlateKind::Series,
         "cast(pair target): Note -> Series should change payload kind");
  expect(casted.features("event_text").kind() == SlateKind::Timeline,
         "cast(pair target): Note -> Timeline should change payload kind");
  expect(pluss::table::detail::has_validity_mask(casted.features("amount_text")),
         "cast(pair target): unparsable Note numeric value should set a validity mask");
  expect(!pluss::table::detail::validity_mask(casted.features("amount_text"))->to_vec<bool>(1),
         "cast(pair target): unparsable Note numeric value should become null");
  expect(casted.features("event_text").tl().year().as_series().to_vec<std::int32_t>() ==
             std::vector<std::int32_t>({2024, 2024, 2024}),
         "cast(pair target): Timeline values should parse correctly");

  auto back_to_note = casted;
  back_to_note.set("event_text") = casted.features("event_text").cast().note();
  expect(back_to_note.features("event_text").kind() == SlateKind::Note,
         "cast(pair target): Timeline -> Note should change payload kind");
}

void test_selection_first_note_tl_and_ts_time_rolling() {
  DataFrame raw({
      {"date_text", Slate(pluss::make_note({"2024-01-01", "bad", "2024-01-05"}))},
      {"price", Slate(pluss::make_series(std::vector<double>{10.0, 12.0, 20.0}))},
      {"volume", Slate(pluss::make_series(std::vector<double>{100.0, 200.0, 300.0}))},
  });

  auto parsed = raw.features({"date_text"})
                    .cast(SlateCastOptions{.format = "%Y-%m-%d", .invalid = pluss::table::ParseInvalid::Null})
                    .timeline();
  expect(parsed.features("date_text").kind() == SlateKind::Timeline,
         "cast(Timeline) should parse selected text feature into Timeline");
  expect(pluss::table::detail::valid_at(parsed.features("date_text"), 0), "valid parsed date should stay valid");
  expect(!pluss::table::detail::valid_at(parsed.features("date_text"), 1), "invalid parsed date should become null when requested");

  const std::vector<std::string_view> iso{
      "2024-01-01T00:00:00Z",
      "2024-01-02T00:00:00Z",
      "2024-01-05T00:00:00Z",
  };
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(iso.size())};
  const auto timeline = pluss::codeh::ops_internal::timeline_from_utf8(
      std::span<const std::string_view>(iso.data(), iso.size()),
      std::span<const std::int64_t>(shape.data(), shape.size()));

  DataFrame df({
      {"date", Slate(timeline)},
      {"price", Slate(pluss::make_series(std::vector<double>{10.0, 12.0, 20.0}))},
      {"volume", Slate(pluss::make_series(std::vector<double>{100.0, 200.0, 300.0}))},
  });
  auto values = df.labels().set("date", false).features({"price", "volume"});

  auto rolling = values.ts().rolling("2D", WindowStatistic::Sum);
  const auto price = to_f64(rolling.features("price"));
  const auto volume = to_f64(rolling.features("volume"));
  expect(price.size() == 3 && std::abs(price[0] - 10.0) < 1e-12 &&
             std::abs(price[1] - 22.0) < 1e-12 &&
             std::abs(price[2] - 20.0) < 1e-12,
         "selection-first ts().rolling(\"2D\", Sum) should use timeline cases");
  expect(volume.size() == 3 && std::abs(volume[0] - 100.0) < 1e-12 &&
             std::abs(volume[1] - 300.0) < 1e-12 &&
             std::abs(volume[2] - 300.0) < 1e-12,
         "selection-first ts().rolling should process every selected numeric feature");

  auto sorted = values.ts().sort(false);
  const auto sorted_price = to_f64(sorted.features("price"));
  expect(sorted_price.size() == 3 && std::abs(sorted_price[0] - 20.0) < 1e-12 &&
             std::abs(sorted_price[2] - 10.0) < 1e-12,
         "selection-first ts().sort(false) should sort selected values by timeline cases");
}

void test_payload_surfaces_reject_wrong_payloads() {
  const std::vector<std::string_view> iso{
      "2024-01-01T00:00:00Z",
      "2024-01-02T00:00:00Z",
  };
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(iso.size())};
  const auto timeline = pluss::codeh::ops_internal::timeline_from_utf8(
      std::span<const std::string_view>(iso.data(), iso.size()),
      std::span<const std::int64_t>(shape.data(), shape.size()));

  DataFrame df({
      {"date", Slate(timeline)},
      {"name", Slate(pluss::make_note({"Ada", "Grace"}))},
      {"price", Slate(pluss::make_series(std::vector<double>{10.0, 12.0}))},
  });

  expect_throw([&] { (void)df.features("price").nt().upper(); },
               "Slate.nt() should reject Series payloads");
  expect_throw([&] { (void)df.features("name").tl().year(); },
               "Slate.tl() should reject Note payloads");
  expect_throw([&] { (void)df.features({"price"}).nt().upper(); },
               "DataFrame.features(...).nt() should reject non-Note features");
  expect_throw([&] { (void)df.features({"name"}).tl().year(); },
               "DataFrame.features(...).tl() should reject non-Timeline features");
  expect_throw([&] { (void)df.nt().upper(); },
               "DataFrame.nt() should require an intentional all-Note selection");
  expect_throw([&] { (void)df.tl().year(); },
               "DataFrame.tl() should require an intentional all-Timeline selection");

  expect(df.features("name").nt().upper().kind() == SlateKind::Note,
         "Slate.nt() should still work for Note payloads");
  expect(df.features("date").tl().year().kind() == SlateKind::Series,
         "Slate.tl() should still work for Timeline payloads");
  expect(df.features("date").tl().period("month").tl().day().as_series().to_vec<std::int32_t>()
             == std::vector<std::int32_t>({1, 1}),
         "Slate.tl().period(month) should bucket timestamps to the first day of the month");
  expect(df.features({"date"}).tl().period("D").features("date").kind() == SlateKind::Timeline,
         "DataFrame.tl().period(D) should delegate period bucketing to selected Timeline features");
}

} // namespace

int main() {
  try {
    test_dataframe_ts_one_feature_and_set();
    test_dataframe_ts_resample_reindex_blocks();
    test_dataframe_cast_timeline_from_yyyymmdd_ints();
    test_dataframe_cast_pair_payload_targets();
    test_selection_first_note_tl_and_ts_time_rolling();
    test_payload_surfaces_reject_wrong_payloads();
    std::cout << "table_ts_accessor_tests passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "table_ts_accessor_tests failed: " << e.what() << '\n';
    return 1;
  }
}
