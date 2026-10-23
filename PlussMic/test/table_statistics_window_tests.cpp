#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "pluss/pluss.h"

namespace {

using pluss::table::DataFrame;
using pluss::table::Atlas;
using pluss::table::Axis;
using pluss::table::ScanStatistic;
using pluss::table::StatisticOptions;
using pluss::table::Slate;
using pluss::table::SlateValue;
using pluss::table::TimeWindowStatistic;
using pluss::table::TsOptions;
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
      {"x", Slate(x)},
      {"y", Slate(y)},
  });
}

void test_statistics() {
  const auto frame = numeric_frame();
  expect(frame.shape() == std::make_pair<std::int64_t, std::int64_t>(4, 2),
         "DataFrame::shape should report cases by features");
  expect(frame.features("x").ncases() == 4,
         "Slate::ncases should match Slate length");
  expect(frame.features("x").shape() == std::make_pair<std::int64_t, std::int64_t>(4, 1),
         "Slate::shape should report cases by one feature");
  const auto logged_x = frame.log().features("x").as_series().to_vec<double>();
  near(logged_x[0], 0.0, 1e-6, "DataFrame::log should delegate numeric features to Slate::log");
  near(logged_x[1], std::log(2.0), 1e-6, "DataFrame::log should preserve numeric log values");
  const auto exp_x = frame.exp().features("x").as_series().to_vec<double>();
  near(exp_x[0], std::exp(1.0), 1e-5, "DataFrame::exp should delegate numeric features to Slate::exp");
  const auto temporal = Slate(pluss::timeline::from_epoch(pluss::make_series(
      std::vector<std::int64_t>{
          1'704'067'200'000'000'000LL,
          pluss::codeh::Timeline::kNaT,
          1'704'153'600'000'000'000LL,
      },
      {3})));
  expect(temporal.isnull().as_series().to_vec<bool>() ==
             std::vector<bool>({false, true, false}),
         "Slate::isnull should treat Timeline NaT as null");
  expect(temporal.notnull().as_series().to_vec<bool>() ==
             std::vector<bool>({true, false, true}),
         "Slate::notnull should treat Timeline NaT as missing");
  expect(temporal.invalid().as_series().to_vec<bool>() ==
             std::vector<bool>({false, true, false}),
         "Slate::invalid should treat Timeline NaT as invalid");
  const auto temporal_frame = DataFrame({{"when", temporal}});
  expect(temporal_frame.isnull().features("when").as_series().to_vec<bool>() ==
             std::vector<bool>({false, true, false}),
         "DataFrame::isnull should delegate Timeline NaT handling through Slate::isnull");
  expect(temporal_frame.invalid().features("when").as_series().to_vec<bool>() ==
             std::vector<bool>({false, true, false}),
         "DataFrame::invalid should delegate Timeline NaT handling through Slate::invalid");
  const auto numeric_invalid = Slate(pluss::make_series(
      std::vector<double>{1.0, std::numeric_limits<double>::quiet_NaN(),
                          std::numeric_limits<double>::infinity(),
                          -std::numeric_limits<double>::infinity()}));
  expect(numeric_invalid.invalid().as_series().to_vec<bool>() ==
             std::vector<bool>({false, true, true, true}),
         "Slate::invalid should catch NaN and infinities for numeric Series");

  const auto axis_frame = DataFrame({
      {"A1", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40}))},
      {"A2", Slate(pluss::make_series(std::vector<double>{1, 2, 3, 4}))},
  }, Slate(pluss::make_note({"1001", "1002", "1003", "1004"})));
  expect(pluss::table::detail::label_kind(axis_frame.labels().values()) == pluss::table::LabelKind::Case,
         "DataFrame constructor labels should be case label provenance");
  const auto column_sum = axis_frame.features({"A1", "A2"}).sum();
  expect(column_sum.size() == 2,
         "default DataFrame::sum should reduce down cases per feature");
  expect(column_sum.as_series().to_vec<double>() ==
             std::vector<double>({100.0, 10.0}),
         "default DataFrame::sum values should match pandas axis=0");
  expect(column_sum.label().values().as_note().vector() ==
             std::vector<std::string_view>({"A1", "A2"}),
         "Axis::Case reductions should label the result by feature names");
  expect(pluss::table::detail::label_kind(column_sum) == pluss::table::LabelKind::Feature,
         "Axis::Case reductions should carry feature-label provenance");
  const auto row_sum = axis_frame.features({"A1", "A2"}).sum(Axis::Feature);
  expect(row_sum.size() == axis_frame.ncases(),
         "Axis::Feature DataFrame::sum should reduce across features per case");
  expect(row_sum.label().values().as_note().vector() ==
             std::vector<std::string_view>({"1001", "1002", "1003", "1004"}),
         "Axis::Feature reductions should label the result by original cases");
  expect(row_sum.as_series().to_vec<double>() ==
             std::vector<double>({11.0, 22.0, 33.0, 44.0}),
         "Axis::Feature DataFrame::sum values should match pandas axis=1");
  expect(pluss::table::detail::label_kind(row_sum) == pluss::table::LabelKind::Case,
         "Axis::Feature reductions should carry case-label provenance");
  auto assigned_axis = axis_frame;
  assigned_axis.set("Assignments") = row_sum;
  expect(assigned_axis.features("Assignments").as_series().to_vec<double>() ==
             std::vector<double>({11.0, 22.0, 33.0, 44.0}),
         "row-wise reduction result should assign as a same-length feature");
  auto direct_assigned_axis = axis_frame;
  direct_assigned_axis.set("Assignments2") = row_sum;
  expect(direct_assigned_axis.features("Assignments2").as_series().to_vec<double>() ==
             std::vector<double>({11.0, 22.0, 33.0, 44.0}),
         "set_features should accept a same-length row-wise reduction result");
  bool feature_labeled_default_threw = false;
  try {
    assigned_axis.set("ColumnTotals") = column_sum;
  } catch (const std::exception&) {
    feature_labeled_default_threw = true;
  }
  expect(feature_labeled_default_threw,
         "default assignment should reject feature-labeled reductions with case-length mismatch");
  assigned_axis.set("ColumnTotals", {.align = pluss::table::AlignBy::Caselbl}) = column_sum;
  expect(pluss::table::detail::validity_mask(assigned_axis.features("ColumnTotals"))->to_vec<bool>() ==
             std::vector<bool>({false, false, false, false}),
         "explicit case-label assignment should null-fill feature-indexed reduction labels");
  bool set_features_feature_labeled_default_threw = false;
  try {
    auto should_throw = axis_frame;
    should_throw.set("ColumnTotals2") = column_sum;
  } catch (const std::exception&) {
    set_features_feature_labeled_default_threw = true;
  }
  expect(set_features_feature_labeled_default_threw,
         "set_features default should reject feature-labeled reductions with case-length mismatch");
  auto direct_feature_indexed = axis_frame;
  direct_feature_indexed.set("ColumnTotals2", {.align = pluss::table::AlignBy::Caselbl}) = column_sum;
  expect(pluss::table::detail::validity_mask(direct_feature_indexed.features("ColumnTotals2"))->to_vec<bool>() ==
             std::vector<bool>({false, false, false, false}),
         "set_features with explicit case-label alignment should null-fill misses");

  auto scholarship = Slate(pluss::make_series(std::vector<double>{1500, 2500, 1000}))
                         .label().set(Slate(pluss::make_note({"1003", "1001", "1004"})));
  auto aligned_axis = axis_frame;
  bool scholarship_default_threw = false;
  try {
    aligned_axis.set("scholarship") = scholarship;
  } catch (const std::exception&) {
    scholarship_default_threw = true;
  }
  expect(scholarship_default_threw,
         "default assignment should reject partial labeled rhs instead of silently aligning");
  aligned_axis.set("scholarship", {.align = pluss::table::AlignBy::Caselbl}) = scholarship;
  expect(aligned_axis.features("scholarship").as_series().to_vec<double>()[0] == 2500.0 &&
             aligned_axis.features("scholarship").as_series().to_vec<double>()[2] == 1500.0 &&
             aligned_axis.features("scholarship").as_series().to_vec<double>()[3] == 1000.0,
         "case-aligned assignment should gather rhs values by case label");
  expect(pluss::table::detail::validity_mask(aligned_axis.features("scholarship"))->to_vec<bool>() ==
             std::vector<bool>({true, false, true, true}),
         "case-aligned assignment should mark unmatched cases null");
  aligned_axis.set("constant") = pluss::Series::full({}, 7235.0, pluss::codem::ScalarType::Float64);
  expect(aligned_axis.features("constant").as_series().to_vec<double>() ==
             std::vector<double>({7235.0, 7235.0, 7235.0, 7235.0}),
         "0D Series assignment should broadcast");
  bool one_row_default_threw = false;
  try {
    aligned_axis.set("one_row_series") = Slate(pluss::make_series(std::vector<double>{7235.0}));
  } catch (const std::exception&) {
    one_row_default_threw = true;
  }
  expect(one_row_default_threw,
         "one-element 1D Slate assignment should not broadcast or align by default");
  const auto default_cases = Slate(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0})).label().values();
  expect(default_cases.as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1, 2}),
         "Slate without explicit cases should expose default RangeIndex-style cases");
  const auto default_label_values = Slate(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0})).label().values();
  expect(default_label_values.as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1, 2}),
         "Slate::label().values should expose default RangeIndex-style labels");
  expect(pluss::table::detail::label_kind(Slate(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}))) ==
             pluss::table::LabelKind::Default,
         "Slate without explicit labels should carry default-label provenance");
  const auto range_labeled =
      Slate(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}), {.rangelbl = true});
  expect(range_labeled.label().values().as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1, 2}),
         "Slate(..., {.rangelbl=true}) should materialize explicit range labels");
  expect(pluss::table::detail::label_kind(range_labeled) == pluss::table::LabelKind::Case,
         "Slate(..., {.rangelbl=true}) should carry case-label provenance");
  const auto chain_range_labeled =
      Slate(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0})).label().range();
  expect(chain_range_labeled.label().values().as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1, 2}),
         "Slate::label().range() should materialize explicit range labels");
  const auto label_range_labeled =
      Slate(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0})).label().range();
  expect(label_range_labeled.label().values().as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1, 2}),
         "Slate::label().range should materialize explicit range labels");
  expect(pluss::table::detail::label_kind(chain_range_labeled) == pluss::table::LabelKind::Case,
         "Slate::label().range() should carry case-label provenance");
  expect(pluss::table::detail::label_kind(label_range_labeled) == pluss::table::LabelKind::Case,
         "Slate::label().range should carry case-label provenance");
  expect(pluss::table::detail::label_kind(
             Slate(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}))
                 .label().set(Slate(pluss::make_note({"a", "b", "c"})))) == pluss::table::LabelKind::Custom,
         "Slate::label().set(labels) should carry custom-label provenance");
  const auto label_set = Slate(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}))
                             .label().set(Slate(pluss::make_note({"a", "b", "c"})));
  expect(label_set.label().values().as_note().vector() ==
             std::vector<std::string_view>({"a", "b", "c"}),
         "Slate::label().set should attach custom labels");
  expect(label_set.label().reset().label().values().as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1, 2}),
         "Slate::label().reset should clear explicit labels back to default range labels");

  const auto label_df = DataFrame({
                           {"score", Slate(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0}))},
                       },
                       Slate(pluss::make_note({"b", "a", "b", "c"})));
  expect(label_df.labels().distinct().as_note().vector() ==
             std::vector<std::string_view>({"b", "a", "c"}),
         "DataFrame::labels().distinct should return distinct label values");
  expect(label_df.labels().duplicated().as_series().to_vec<bool>() ==
             std::vector<bool>({false, false, true, false}),
         "DataFrame::labels().duplicated should mark duplicate follower labels");
  expect(label_df.labels().repeated().as_series().to_vec<bool>() ==
             std::vector<bool>({true, false, true, false}),
         "DataFrame::labels().repeated should mark every member of repeated label groups");
  expect(label_df.labels().drop({.duplicated = true}).ncases() == 3,
         "DataFrame::labels().drop({.duplicated=true}) should drop duplicate follower label rows");
  expect(label_df.labels().drop({.repeated = true}).ncases() == 2,
         "DataFrame::labels().drop({.repeated=true}) should drop all members of repeated label groups");
  expect(label_df.features({"score"}).repeated().as_series().to_vec<bool>() ==
             std::vector<bool>({false, false, false, false}),
         "DataFrame::repeated(feature) should mark all members of repeated row-value groups");
  const auto repeated_rows = DataFrame({
      {"name", Slate(pluss::make_note({"A", "B", "A", "C", "B"}))},
  });
  expect(repeated_rows.features({"name"}).duplicated().as_series().to_vec<bool>() ==
             std::vector<bool>({false, false, true, false, true}),
         "DataFrame::duplicated should mark duplicate followers");
  expect(repeated_rows.features({"name"}).repeated().as_series().to_vec<bool>() ==
             std::vector<bool>({true, true, true, false, true}),
         "DataFrame::repeated should mark every member of repeated groups");
  expect(label_df.labels().has("a").as_series().to_vec<bool>() ==
             std::vector<bool>({false, true, false, false}),
         "DataFrame::labels().has should test label membership");
  expect(label_df.labels().isnull().as_series().to_vec<bool>() ==
             std::vector<bool>({false, false, false, false}),
         "DataFrame::labels().isnull should evaluate label nullness");
  expect(label_df.labels().notnull().as_series().to_vec<bool>() ==
             std::vector<bool>({true, true, true, true}),
         "DataFrame::labels().notnull should evaluate label non-nullness");
  expect(label_df.labels().slice(1, 3).as_note().vector() ==
             std::vector<std::string_view>({"a", "b"}),
         "DataFrame::labels().slice should slice label values");
  const std::array<std::int64_t, 2> label_take_idx{3, 0};
  expect(label_df.labels().take(label_take_idx).as_note().vector() ==
             std::vector<std::string_view>({"c", "b"}),
         "DataFrame::labels().take should take label values by position");
  expect(!label_df.labels().monotonic(),
         "DataFrame::labels().monotonic should detect unsorted labels");
  expect(label_df.labels().sort().labels().monotonic(),
         "DataFrame::labels().sort should produce monotonic labels");
  expect(label_df.labels().depth() == 0,
         "DataFrame::labels().depth should be zero for unsorted flat labels");
  expect(label_df.labels().codes().features("label").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1, 0, 2}),
         "DataFrame::labels().codes should expose factor codes for labels");
  expect(label_df.labels().levels().features("label").as_note().vector() ==
             std::vector<std::string_view>({"b", "a", "c"}),
         "DataFrame::labels().levels should expose distinct label levels");
  const auto peer_labels = Slate(pluss::make_note({"b", "d", "a"}));
  expect(label_df.labels().unite(peer_labels).as_note().vector() ==
             std::vector<std::string_view>({"b", "a", "c", "d"}),
         "DataFrame::labels().unite should preserve left order then append right-only labels");
  expect(label_df.labels().intersect(peer_labels).as_note().vector() ==
             std::vector<std::string_view>({"b", "a"}),
         "DataFrame::labels().intersect should preserve left order for common labels");
  expect(label_df.labels().diff(peer_labels).as_note().vector() ==
             std::vector<std::string_view>({"c"}),
         "DataFrame::labels().diff should return left labels missing from the target");
  expect(label_df.labels().symdiff(peer_labels).as_note().vector() ==
             std::vector<std::string_view>({"c", "d"}),
         "DataFrame::labels().symdiff should return labels exclusive to either side");
  expect(label_df.labels().find(Slate(pluss::make_note({"a", "missing", "b"})))
             .as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({1, -1, 0}),
         "DataFrame::labels().find should return exact-match positions and -1 for misses");
  expect(label_df.labels().pos("b").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 2}),
         "DataFrame::labels().pos should return all positions for duplicate labels");
  expect(label_df.labels().slice("a", "c").as_note().vector() ==
             std::vector<std::string_view>({"a", "b", "c"}),
         "DataFrame::labels().slice(label,label) should return inclusive label-value slices");
  const auto slate_label_ops =
      Slate(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}))
          .label().set(Slate(pluss::make_note({"x", "y", "x"})));
  expect(slate_label_ops.label().find(Slate(pluss::make_note({"y", "z"})))
             .as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({1, -1}),
         "Slate::label().find should expose Index.get_indexer-style lookup");
  expect(slate_label_ops.label().pos("x").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 2}),
         "Slate::label().pos should expose Index.get_loc-style duplicate positions");
  expect(slate_label_ops.label().duplicated().as_series().to_vec<bool>() ==
             std::vector<bool>({false, false, true}),
         "Slate::label().duplicated should mark duplicate follower labels");
  expect(slate_label_ops.label().repeated().as_series().to_vec<bool>() ==
             std::vector<bool>({true, false, true}),
         "Slate::label().repeated should mark every member of repeated label groups");
  expect(slate_label_ops.label().distinct().as_note().vector() ==
             std::vector<std::string_view>({"x", "y"}),
         "Slate::label().distinct should return distinct label values");
  expect(slate_label_ops.label().has(Slate(pluss::make_note({"y"}))).as_series().to_vec<bool>() ==
             std::vector<bool>({false, true, false}),
         "Slate::label().has(Slate) should test set membership against current labels");
  expect(slate_label_ops.label().take({2, 0}).as_note().vector() ==
             std::vector<std::string_view>({"x", "x"}),
         "Slate::label().take should take label values by position");
  expect(slate_label_ops.label().slice(1, 3).as_note().vector() ==
             std::vector<std::string_view>({"y", "x"}),
         "Slate::label().slice should slice label values by position");
  expect(!slate_label_ops.label().monotonic(),
         "Slate::label().monotonic should detect unsorted labels");
  const auto numeric_label_ops =
      Slate(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}))
          .label().set(Slate(pluss::make_series(std::vector<std::int64_t>{10, 20, 10})));
  const auto numeric_united = numeric_label_ops.label().unite(
      Slate(pluss::make_series(std::vector<std::int64_t>{20, 30})));
  expect(numeric_united.kind() == pluss::table::SlateKind::Series &&
             numeric_united.as_series().scalar_type() == pluss::codem::ScalarType::Int64 &&
             numeric_united.as_series().to_vec<std::int64_t>() == std::vector<std::int64_t>({10, 20, 30}),
         "Slate::label().unite should preserve same numeric label dtype");
  const auto mixed_numeric_united = numeric_label_ops.label().unite(
      Slate(pluss::make_series(std::vector<double>{20.5, 30.5})));
  expect(mixed_numeric_united.kind() == pluss::table::SlateKind::Series &&
             mixed_numeric_united.as_series().scalar_type() == pluss::codem::ScalarType::Float64 &&
             mixed_numeric_united.as_series().to_vec<double>() == std::vector<double>({10.0, 20.0, 20.5, 30.5}),
         "Slate::label().unite should upcast mixed numeric label dtypes instead of falling back to Note");
  const auto timeline_left_values = std::vector<std::int64_t>{
      1'704'067'200'000'000'000LL,
      1'704'153'600'000'000'000LL,
      1'704'067'200'000'000'000LL,
  };
  const auto timeline_right_values = std::vector<std::int64_t>{
      1'704'153'600'000'000'000LL,
      1'704'240'000'000'000'000LL,
  };
  const auto timeline_label_ops =
      Slate(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}))
          .label().set(Slate(pluss::timeline::from_epoch(
              pluss::make_series(timeline_left_values, {3}))));
  const auto timeline_united = timeline_label_ops.label().unite(
      Slate(pluss::timeline::from_epoch(pluss::make_series(timeline_right_values, {2}))));
  expect(timeline_united.kind() == pluss::table::SlateKind::Timeline &&
             timeline_united.as_timeline().ticks() ==
                 std::vector<std::int64_t>({
                     1'704'067'200'000'000'000LL,
                     1'704'153'600'000'000'000LL,
                     1'704'240'000'000'000'000LL,
                 }),
         "Slate::label().unite should preserve Timeline label dtype");
  const auto numeric_label_df = DataFrame({
      {"score", Slate(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}))},
  }, Slate(pluss::make_series(std::vector<std::int64_t>{10, 20, 10})));
  const auto df_numeric_intersect = numeric_label_df.labels().intersect(
      Slate(pluss::make_series(std::vector<std::int64_t>{20, 30, 10})));
  expect(df_numeric_intersect.kind() == pluss::table::SlateKind::Series &&
             df_numeric_intersect.as_series().scalar_type() == pluss::codem::ScalarType::Int64 &&
             df_numeric_intersect.as_series().to_vec<std::int64_t>() == std::vector<std::int64_t>({10, 20}),
         "DataFrame::labels().intersect should preserve numeric label dtype through Slate delegation");

  const auto source_labeled = Slate(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}))
                                  .label().set(Slate(pluss::make_note({"b", "a", "c"})));
  const auto target_case_labels = Slate(pluss::make_note({"a", "b", "d"}));
  const auto aligned_by_case = source_labeled.align(
      target_case_labels,
      pluss::table::AlignOptions{.align = pluss::table::AlignBy::Caselbl});
  const auto aligned_by_label_surface = source_labeled.label().align(
      target_case_labels,
      pluss::table::AlignOptions{.align = pluss::table::AlignBy::Caselbl});
  expect(aligned_by_case.label().values().as_note().vector() ==
             std::vector<std::string_view>({"a", "b", "d"}),
         "Slate::align(Caselbl) should carry target case labels");
  expect(aligned_by_case.as_series().to_vec<double>()[0] == 20.0 &&
             aligned_by_case.as_series().to_vec<double>()[1] == 10.0,
         "Slate::align(Caselbl) should reorder values by label");
  expect(aligned_by_label_surface.label().values().as_note().vector() ==
             std::vector<std::string_view>({"a", "b", "d"}) &&
             aligned_by_label_surface.as_series().to_vec<double>()[0] == 20.0 &&
             aligned_by_label_surface.as_series().to_vec<double>()[1] == 10.0,
         "Slate::label().align should delegate to Slate label alignment");
  expect(pluss::table::detail::validity_mask(aligned_by_case)->to_vec<bool>() ==
             std::vector<bool>({true, true, false}),
         "Slate::align(Caselbl) should null-mark missing target labels by default");
  expect(pluss::table::detail::label_kind(aligned_by_case) == pluss::table::LabelKind::Case,
         "Slate::align(Caselbl) should carry case-label provenance");

  const auto filled_by_case = source_labeled.align(
      target_case_labels,
      pluss::table::AlignOptions{.align = pluss::table::AlignBy::Caselbl, .fill = 0.0});
  expect(filled_by_case.as_series().to_vec<double>() ==
             std::vector<double>({20.0, 10.0, 0.0}),
         "Slate::align(Caselbl, fill) should fill missing values");
  expect(!pluss::table::detail::has_validity_mask(filled_by_case),
         "Slate::align(Caselbl, fill) should clear validity mask after filling");

  const auto feature_slate = Slate(pluss::make_series(std::vector<double>{1.5, 2.5}))
                                  .label().set(Slate(pluss::make_note({"A2", "A1"})));
  const auto feature_labels = Slate(pluss::make_note({"A1", "A2"}));
  const auto aligned_by_feature = feature_slate.align(
      feature_labels,
      pluss::table::AlignOptions{.align = pluss::table::AlignBy::Feature});
  expect(aligned_by_feature.as_series().to_vec<double>() ==
             std::vector<double>({2.5, 1.5}),
         "Slate::align(Feature) should align one value per feature by feature name");
  expect(pluss::table::detail::label_kind(aligned_by_feature) == pluss::table::LabelKind::Feature,
         "Slate::align(Feature) should carry feature-label provenance");

  bool positional_mismatch_threw = false;
  try {
    (void)source_labeled.align(
        Slate(pluss::make_note({"a", "b"})),
        pluss::table::AlignOptions{.align = pluss::table::AlignBy::Pos});
  } catch (const std::exception&) {
    positional_mismatch_threw = true;
  }
  expect(positional_mismatch_threw,
         "Slate::align(Pos) should throw on length mismatch");

  bool both_threw = false;
  try {
    (void)source_labeled.align(
        target_case_labels,
        pluss::table::AlignOptions{.align = pluss::table::AlignBy::Both});
  } catch (const std::exception&) {
    both_threw = true;
  }
  expect(both_threw,
         "Slate::align(Both) should reject DataFrame-only alignment mode");
  const auto default_left = Slate(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}));
  const auto default_right = Slate(pluss::make_series(std::vector<double>{1.0, 2.0}));
  bool default_length_mismatch_threw = false;
  try {
    (void)(default_left + default_right);
  } catch (const std::exception&) {
    default_length_mismatch_threw = true;
  }
  expect(default_length_mismatch_threw,
         "Slate arithmetic should be strict positional by default");
  const auto one_value_1d = Slate(pluss::make_series(std::vector<double>{1.0}));
  bool one_value_1d_threw = false;
  try {
    (void)(default_left + one_value_1d);
  } catch (const std::exception&) {
    one_value_1d_threw = true;
  }
  expect(one_value_1d_threw,
         "one-element 1D Slate should not broadcast or align by default");
  const auto scalar_cmp = default_left == pluss::make_series(20.0);
  expect(scalar_cmp.as_series().to_vec<bool>() ==
             std::vector<bool>({false, true, false}),
         "0D Series payload comparison should broadcast as a scalar");
  bool one_value_cmp_threw = false;
  try {
    (void)(default_left == pluss::make_series(std::vector<double>{20.0}));
  } catch (const std::exception&) {
    one_value_cmp_threw = true;
  }
  expect(one_value_cmp_threw,
         "one-element 1D Series payload comparison should not broadcast or align by default");

  const auto s1 = Slate(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}))
                      .label().set(Slate(pluss::make_note({"Alice", "Bob", "Carol"})));
  const auto s2 = Slate(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}))
                      .label().set(Slate(pluss::make_note({"Carol", "Alice", "Bob"})));
  const auto positional_sum = s1 + s2;
  expect(positional_sum.as_series().to_vec<double>() ==
             std::vector<double>({11.0, 22.0, 33.0}),
         "Slate arithmetic should use positional order by default");
  expect(s1.add(s2).as_series().to_vec<double>() == positional_sum.as_series().to_vec<double>(),
         "Slate::add should match Slate operator+");
  expect(s1.sub(s2).as_series().to_vec<double>() ==
             std::vector<double>({9.0, 18.0, 27.0}),
         "Slate::sub should expose method-form arithmetic");
  expect(s1.mul(2.0).as_series().to_vec<double>() ==
             std::vector<double>({20.0, 40.0, 60.0}),
         "Slate::mul(double) should expose scalar arithmetic");
  expect(s1.gt(s2).as_series().to_vec<bool>() ==
             std::vector<bool>({true, true, true}),
         "Slate::gt should match comparison operator semantics");
  expect((s1.gt(15.0) & s1.lt(35.0)).as_series().to_vec<bool>() ==
             s1.between(15.0, 35.0, false).as_series().to_vec<bool>(),
         "Slate::between should compose comparison methods");
  expect(Slate(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}))
             .cumsum()
             .as_series()
             .to_vec<double>() == std::vector<double>({1.0, 3.0, 6.0}),
         "Slate::cumsum should delegate to Slate::scan");
  near(Slate(pluss::make_series(std::vector<double>{2.0, 3.0, 4.0}))
           .prod()
           .to_dbl(),
       24.0,
       1e-12,
       "Slate::prod should expose the 1D product reduction");
  const auto labelled_scores =
      Slate(pluss::make_series(std::vector<double>{20.0, 10.0, 30.0}))
          .label().set(Slate(pluss::make_note({"Alice", "Bob", "Carol"})));
  expect(labelled_scores.csmin().to_str() == "Bob",
         "Slate::csmin should return the case label at the minimum value");
  expect(labelled_scores.csmax().to_str() == "Carol",
         "Slate::csmax should return the case label at the maximum value");
  expect(labelled_scores.top(2, false).as_series().to_vec<double>() ==
             std::vector<double>({30.0, 20.0}),
         "Slate::top should return the largest values in descending order");
  expect(labelled_scores.bottom(2, false).as_series().to_vec<double>() ==
             std::vector<double>({10.0, 20.0}),
         "Slate::bottom should return the smallest values in ascending order");
  const auto appended_scores =
      labelled_scores.append(Slate(pluss::make_series(std::vector<double>{40.0}))
                                 .label().set(Slate(pluss::make_note({"David"}))));
  expect(appended_scores.as_series().to_vec<double>() ==
             std::vector<double>({20.0, 10.0, 30.0, 40.0}),
         "Slate::append should concatenate 1D Series payloads");
  expect(appended_scores.label().values().as_note().vector() ==
             std::vector<std::string_view>({"Alice", "Bob", "Carol", "David"}),
         "Slate::append should preserve and concatenate case labels");
  expect(Slate(pluss::make_note({"x", "y"}))
             .append(Slate(pluss::make_note({"z"})))
             .as_note()
             .vector() == std::vector<std::string_view>({"x", "y", "z"}),
         "Slate::append should concatenate Note payloads");
  expect(labelled_scores.sample(2, std::uint64_t{7}).ncases() == 2,
         "Slate::sample should expose deterministic 1D sampling");
  expect(labelled_scores.sample(5, std::uint64_t{7}, true).ncases() == 5,
         "Slate::sample should support replacement");
  expect(labelled_scores.shuffle(7).ncases() == labelled_scores.ncases(),
         "Slate::shuffle should preserve the 1D row count");
  expect(Slate(pluss::make_note({"alpha", "beta", "gamma"}))
             .has("beta")
             .as_series()
             .to_vec<bool>() == std::vector<bool>({false, true, false}),
         "Slate::has should return an exact-membership Slate mask");
  expect(positional_sum.label().values().as_note().vector() ==
             std::vector<std::string_view>({"Alice", "Bob", "Carol"}),
         "Slate arithmetic should preserve left labels under positional semantics");
  const auto aligned_sum = s1 + s2.align(s1.label().values(), {.align = pluss::table::AlignBy::Caselbl});
  expect(aligned_sum.as_series().to_vec<double>() ==
             std::vector<double>({12.0, 23.0, 31.0}),
         "Slate arithmetic should support explicit case-label alignment via Slate::align");

  const auto left_missing = Slate(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}))
                                .label().set(Slate(pluss::make_note({"A", "B", "C"})));
  const auto right_missing = Slate(pluss::make_series(std::vector<double>{1.0, 2.0}))
                                 .label().set(Slate(pluss::make_note({"B", "D"})));
  bool missing_sum_threw = false;
  try {
    (void)(left_missing + right_missing);
  } catch (const std::exception&) {
    missing_sum_threw = true;
  }
  expect(missing_sum_threw,
         "Slate arithmetic should reject unequal lengths unless the user explicitly aligns first");

  const auto bool_a = Slate(pluss::make_series(std::vector<bool>{true, false, true}))
                          .label().set(Slate(pluss::make_note({"x", "y", "z"})));
  const auto bool_b = Slate(pluss::make_series(std::vector<bool>{true, true}))
                          .label().set(Slate(pluss::make_note({"z", "x"})));
  bool bool_and_threw = false;
  try {
    (void)(bool_a & bool_b);
  } catch (const std::exception&) {
    bool_and_threw = true;
  }
  expect(bool_and_threw,
         "Slate logical ops should reject unequal lengths unless the user explicitly aligns first");

  const auto cmp = s1 > s2;
  expect(cmp.label().values().as_note().vector() ==
             std::vector<std::string_view>({"Alice", "Bob", "Carol"}),
         "Slate comparisons should carry left labels under positional semantics");
  expect(cmp.as_series().to_vec<bool>() ==
             std::vector<bool>({true, true, true}),
         "Slate comparisons should compare positional values by default");

  const auto corr_left = Slate(pluss::make_series(std::vector<double>{1.0, 2.0}))
                             .label().set(Slate(pluss::make_note({"A", "B"})));
  const auto corr_right = Slate(pluss::make_series(std::vector<double>{20.0, 10.0}))
                              .label().set(Slate(pluss::make_note({"B", "A"})));
  near(corr_left.corr(corr_right), 1.0, 1e-12,
       "Slate::corr should align by case labels before computing");
  near(corr_left.corr_with(corr_right).to_dbl(), 1.0, 1e-12,
       "Slate::corr_with should align by case labels before computing");
  near(corr_left.covar(corr_right, {.ddof = 0, .min_count = 2}).to_dbl(), 2.5, 1e-12,
       "Slate::covar should align by case labels before computing");
  near(corr_left.matmul(corr_right).to_dbl(), 50.0, 1e-12,
       "Slate::matmul should align by case labels before computing");
  const auto filter_mask = Slate(pluss::make_series(std::vector<bool>{true, false}))
                               .label().set(Slate(pluss::make_note({"1003", "1001"})));
  const auto filtered_axis = axis_frame.filter(filter_mask);
  expect(filtered_axis.labels().values().as_note().vector() ==
             std::vector<std::string_view>({"1003"}),
         "DataFrame::filter(Slate) should align Slate masks by case labels by default");
  bool filter_pos_threw = false;
  try {
    (void)axis_frame.filter(filter_mask, {.align = pluss::table::AlignBy::Pos});
  } catch (const std::exception&) {
    filter_pos_threw = true;
  }
  expect(filter_pos_threw,
         "DataFrame::filter(mask, Pos) should reject mismatched positional mask length");
  auto masked_axis = axis_frame;
  masked_axis.put_("A1", filter_mask, 999.0);
  expect(masked_axis.features("A1").as_series().to_vec<double>() ==
             std::vector<double>({10.0, 20.0, 999.0, 40.0}),
         "DataFrame::put_(Slate mask) should align Slate masks by case labels by default");
  auto masked_axis_pos = axis_frame;
  bool put_pos_threw = false;
  try {
    masked_axis_pos.put_("A1", filter_mask, 999.0, {.align = pluss::table::AlignBy::Pos});
  } catch (const std::exception&) {
    put_pos_threw = true;
  }
  expect(put_pos_threw,
         "DataFrame::put_(mask, Pos) should reject mismatched positional mask length");

  const auto frame_left = DataFrame({
      {"A", Slate(pluss::make_series(std::vector<double>{10.0, 20.0}))},
      {"B", Slate(pluss::make_series(std::vector<double>{100.0, 200.0}))},
  }, Slate(pluss::make_note({"r1", "r2"})));
  const auto frame_right_rows = DataFrame({
      {"A", Slate(pluss::make_series(std::vector<double>{1.0, 2.0}))},
      {"B", Slate(pluss::make_series(std::vector<double>{10.0, 20.0}))},
  }, Slate(pluss::make_note({"r2", "r1"})));
  const auto frame_pos = frame_left.add(frame_right_rows);
  expect(frame_pos.features("A").as_series().to_vec<double>() ==
             std::vector<double>({11.0, 22.0}),
         "DataFrame::add(DataFrame) should be positional by default");
  expect((frame_left + frame_right_rows).features("A").as_series().to_vec<double>() ==
             std::vector<double>({11.0, 22.0}),
         "DataFrame operator+ should delegate to DataFrame::add");
  expect((frame_left * 2.0).features("B").as_series().to_vec<double>() ==
             std::vector<double>({200.0, 400.0}),
         "DataFrame operator* should delegate to DataFrame::mul");
  const auto frame_case = frame_left.add(frame_right_rows, {.align = pluss::table::AlignBy::Caselbl});
  expect(frame_case.features("A").as_series().to_vec<double>() ==
             std::vector<double>({12.0, 21.0}),
         "DataFrame::add(DataFrame, Caselbl) should align rows by case label");

  const auto frame_right_features = DataFrame({
      {"B", Slate(pluss::make_series(std::vector<double>{1.0, 2.0}))},
      {"A", Slate(pluss::make_series(std::vector<double>{10.0, 20.0}))},
  }, Slate(pluss::make_note({"r1", "r2"})));
  const auto frame_feature = frame_left.add(frame_right_features, {.align = pluss::table::AlignBy::Feature});
  expect(frame_feature.features("A").as_series().to_vec<double>() ==
             std::vector<double>({20.0, 40.0}) &&
             frame_feature.features("B").as_series().to_vec<double>() ==
             std::vector<double>({101.0, 202.0}),
         "DataFrame::add(DataFrame, Feature) should align columns by feature label");

  const auto feature_offsets = Slate(pluss::make_series(std::vector<double>{10.0, 100.0}))
                                   .label().set(Slate(pluss::make_note({"B", "A"})));
  const auto feature_pos = frame_left.add(feature_offsets);
  expect(feature_pos.features("A").as_series().to_vec<double>() ==
             std::vector<double>({20.0, 30.0}) &&
             feature_pos.features("B").as_series().to_vec<double>() ==
             std::vector<double>({200.0, 300.0}),
         "DataFrame::add(Slate) should broadcast one value per feature positionally by default");
  const auto feature_aligned = frame_left.add(feature_offsets, {.align = pluss::table::AlignBy::Feature});
  expect(feature_aligned.features("A").as_series().to_vec<double>() ==
             std::vector<double>({110.0, 120.0}) &&
             feature_aligned.features("B").as_series().to_vec<double>() ==
             std::vector<double>({110.0, 210.0}),
         "DataFrame::add(Slate, Feature) should align one value per feature by feature label");

  const auto case_offsets = Slate(pluss::make_series(std::vector<double>{1000.0, 100.0}))
                                .label().set(Slate(pluss::make_note({"r2", "r1"})));
  const auto case_pos = frame_left.add(case_offsets, Axis::Case);
  expect(case_pos.features("A").as_series().to_vec<double>() ==
             std::vector<double>({1010.0, 120.0}),
         "DataFrame::add(Slate, Axis::Case) should broadcast one value per row positionally by default");
  const auto case_aligned =
      frame_left.add(case_offsets, Axis::Case, {.align = pluss::table::AlignBy::Caselbl});
  expect(case_aligned.features("A").as_series().to_vec<double>() ==
             std::vector<double>({110.0, 1020.0}),
         "DataFrame::add(Slate, Axis::Case, Caselbl) should align row values by case label");
  const auto cmp_feature = frame_left.gt(feature_offsets, {.align = pluss::table::AlignBy::Feature});
  expect(cmp_feature.features("A").as_series().to_vec<bool>() ==
             std::vector<bool>({false, false}) &&
             cmp_feature.features("B").as_series().to_vec<bool>() ==
             std::vector<bool>({true, true}),
         "DataFrame comparison with Slate should use the same feature-alignment policy");
  const auto text_frame = DataFrame({
      {"name", Slate(pluss::make_note({"alpha", "beta"}))},
      {"score", Slate(pluss::make_series(std::vector<double>{1.0, 2.0}))},
  }, Slate(pluss::make_note({"r1", "r2"})));
  const auto text_has = text_frame.has(Slate(pluss::make_note({"alpha", "2"})));
  expect(text_has.features("name").as_series().to_vec<bool>() ==
             std::vector<bool>({true, false}) &&
             text_has.features("score").as_series().to_vec<bool>() ==
             std::vector<bool>({false, true}),
         "DataFrame::has should delegate membership checks to each Slate");
  expect(text_frame.eq("beta").features("name").as_series().to_vec<bool>() ==
             std::vector<bool>({false, true}),
         "DataFrame::eq(string) should delegate text comparisons to Slate");
  expect(text_frame.le("beta").features("name").as_series().to_vec<bool>() ==
             std::vector<bool>({true, true}),
         "DataFrame::le(string) should delegate text ordering to Slate");
  const auto bool_frame = DataFrame({
      {"a", Slate(pluss::make_series(std::vector<bool>{true, false}))},
      {"b", Slate(pluss::make_series(std::vector<bool>{false, true}))},
  }, Slate(pluss::make_note({"r1", "r2"})));
  const auto bool_mask = Slate(pluss::make_series(std::vector<bool>{true, false}));
  expect(bool_frame.land(bool_mask).features("a").as_series().to_vec<bool>() ==
             std::vector<bool>({true, false}) &&
             bool_frame.lor(bool_frame).features("b").as_series().to_vec<bool>() ==
             std::vector<bool>({false, true}),
         "DataFrame logical methods should delegate to Slate logical methods");

  const auto cov = frame.covar();
  near(cov.features("x").as_series().to_vec<double>(0), 5.0 / 3.0, 1e-9,
       "sample covariance x/x");
  near(cov.features("y").as_series().to_vec<double>(0), 10.0 / 3.0, 1e-9,
       "sample covariance x/y");
  near(frame.covar({.ddof = 0, .min_count = 1})
           .features("x").as_series().to_vec<double>(0),
       1.25, 1e-9, "population covariance");
  const auto offset_cov = DataFrame({
      {"x", Slate(pluss::make_series(std::vector<double>{
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
      {"x", Slate(pluss::make_series(std::vector<double>{3, 1, 3, 2}))},
  }).mode();
  near(modes.features("x").as_series().to_vec<double>(0), 3.0, 1e-9,
       "most frequent numeric value");
  const auto tied_modes = DataFrame({
      {"x", Slate(pluss::make_series(std::vector<double>{1, 1, 2, 2}))},
  }).mode();
  expect(tied_modes.ncases() == 2, "all tied modes should be returned");
  expect(tied_modes.features("x").as_series().to_vec<double>() ==
             std::vector<double>({1, 2}),
         "tied modes should be sorted");
  const auto time_mode = Slate(pluss::make_timeline({
      "2024-01-02T00:00:00Z",
      "2024-01-01T00:00:00Z",
      "2024-01-02T00:00:00Z",
  })).mode();
  expect(time_mode.kind() == pluss::table::SlateKind::Timeline &&
             time_mode.size() == 1 &&
             time_mode.as_timeline().day().to_vec<std::int32_t>(0) == 2,
         "timeline mode preserves its Slate kind");
  const auto mixed_modes = DataFrame({
      {"x", Slate(pluss::make_series(
                std::vector<double>{1, 1, 2, 2}))},
      {"when", Slate(pluss::make_timeline({
                   "2024-01-02T00:00:00Z",
                   "2024-01-01T00:00:00Z",
                   "2024-01-02T00:00:00Z",
                   "2024-01-02T00:00:00Z",
               }))},
  }).mode();
  expect(mixed_modes.features("when").kind() ==
             pluss::table::SlateKind::Timeline &&
             !pluss::table::detail::valid_at(mixed_modes.features("when"), 1),
         "DataFrame mode pads time-like Slates without changing kind");

  const auto ranks = DataFrame({
      {"x", Slate(pluss::make_series(std::vector<double>{20, 10, 20}))},
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
      {"x", Slate(pluss::make_series(
                std::vector<double>{1, 1, 1, 1}))},
  });
  near(constant.skew().as_series().to_vec<double>(0),
       0.0, 1e-12, "constant skewness");
  near(constant.kurtosis().as_series().to_vec<double>(0),
       0.0, 1e-12, "constant excess kurtosis");
  near(frame.se().as_series().to_vec<double>(0),
       std::sqrt(5.0 / 3.0) / 2.0, 1e-9, "standard error");
  near(frame.var(Axis::Case, {.ddof = 0, .min_count = 4})
           .as_series().to_vec<double>(0),
       1.25, 1e-9, "configurable variance ddof");
  near(frame.std(Axis::Case, {.ddof = 0, .min_count = 4})
           .as_series().to_vec<double>(0),
       std::sqrt(1.25), 1e-9, "configurable deviation ddof");

  const auto cumulative = frame.scan(ScanStatistic::Sum);
  expect(cumulative.features("x").as_series().to_vec<double>() ==
             std::vector<double>({1, 3, 6, 10}),
         "running sum values");
  expect(frame.scan(ScanStatistic::Product).features("x").as_series().to_vec<double>() ==
             std::vector<double>({1, 2, 6, 24}),
         "running product values");
  const auto row_scan = DataFrame({
      {"a", Slate(pluss::make_series(std::vector<double>{1, 10}))},
      {"b", Slate(pluss::make_series(std::vector<double>{2, 20}))},
      {"c", Slate(pluss::make_series(std::vector<double>{3, 30}))},
  }).cumsum(Axis::Feature);
  expect(row_scan.features("a").as_series().to_vec<double>() ==
             std::vector<double>({1, 10}) &&
             row_scan.features("b").as_series().to_vec<double>() ==
                 std::vector<double>({3, 30}) &&
             row_scan.features("c").as_series().to_vec<double>() ==
                 std::vector<double>({6, 60}),
         "Axis::Feature cumsum should scan across features per case like pandas axis=1");
  const auto row_max = DataFrame({
      {"a", Slate(pluss::make_series(std::vector<double>{3, 10}))},
      {"b", Slate(pluss::make_series(std::vector<double>{2, 40}))},
      {"c", Slate(pluss::make_series(std::vector<double>{5, 30}))},
  }).cummax(Axis::Feature);
  expect(row_max.features("a").as_series().to_vec<double>() ==
             std::vector<double>({3, 10}) &&
             row_max.features("b").as_series().to_vec<double>() ==
                 std::vector<double>({3, 40}) &&
             row_max.features("c").as_series().to_vec<double>() ==
                 std::vector<double>({5, 40}),
         "Axis::Feature cummax should scan across features per case");
  auto nullable_valid = pluss::Series::ones(
      {4}, pluss::codem::ScalarType::Bool);
  reinterpret_cast<bool*>(nullable_valid.raw_data())[1] = false;
  const auto nullable = DataFrame({
      {"x", Slate(
                pluss::make_series(std::vector<double>{1, 0, 3, 4}),
                nullable_valid)},
  });
  expect(pluss::table::detail::validity_mask(nullable.scan(ScanStatistic::Sum, true).features("x"))->to_vec<bool>() ==
             std::vector<bool>({true, false, true, true}),
         "skip-null cumulative validity resumes after a null");
  expect(pluss::table::detail::validity_mask(nullable.scan(ScanStatistic::Sum, false).features("x"))->to_vec<bool>() ==
             std::vector<bool>({true, false, false, false}),
         "strict cumulative validity stops after a null");
  auto row_valid = pluss::Series::ones({2}, pluss::codem::ScalarType::Bool);
  reinterpret_cast<bool*>(row_valid.raw_data())[0] = false;
  const auto row_nullable = DataFrame({
      {"a", Slate(pluss::make_series(std::vector<double>{1, 10}))},
      {"b", Slate(pluss::make_series(std::vector<double>{2, 20}), row_valid)},
      {"c", Slate(pluss::make_series(std::vector<double>{3, 30}))},
  });
  expect(pluss::table::detail::validity_mask(row_nullable.cumsum(Axis::Feature, true).features("c"))->to_vec<bool>() ==
             std::vector<bool>({true, true}),
         "Axis::Feature skip-null cumulative validity should resume after row nulls");
  expect(pluss::table::detail::validity_mask(row_nullable.cumsum(Axis::Feature, false).features("c"))->to_vec<bool>() ==
             std::vector<bool>({false, true}),
         "Axis::Feature strict cumulative validity should propagate row nulls to later features");

  const auto related = frame.corr_with(frame);
  near(related.as_series().to_vec<double>(0), 1.0, 1e-9,
       "corr_with self");

  const auto left = DataFrame({
      {"a", Slate(pluss::make_series(std::vector<double>{1, 3}))},
      {"b", Slate(pluss::make_series(std::vector<double>{2, 4}))},
  });
  const auto right = DataFrame({
      {"z", Slate(pluss::make_series(std::vector<double>{5, 6}))},
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
  expect(!pluss::table::detail::validity_mask(variance.features("x"))->to_vec<bool>(0),
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
  expect(!pluss::table::detail::validity_mask(exponential_variance)->to_vec<bool>(0),
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
      {"group", Slate(pluss::make_note({"a", "b", "a", "b"}))},
      {"value", Slate(pluss::make_series(std::vector<double>{1, 10, 3, 20}))},
  });
  const auto grouped = grouped_frame.features("value").ts().grouped(
      grouped_frame.features("group"), 2, WindowStatistic::Mean);
  const auto grouped_values = grouped.as_series().to_vec<double>();
  near(grouped_values[0], 1.0, 1e-9, "grouped moving first a");
  near(grouped_values[1], 10.0, 1e-9, "grouped moving first b");
  near(grouped_values[2], 2.0, 1e-9, "grouped moving second a");
  near(grouped_values[3], 15.0, 1e-9, "grouped moving second b");
  const auto grouped_df = grouped_frame.features({"value"})
                              .ts({.group = grouped_frame.features("group")})
                              .grouped(2, WindowStatistic::Mean);
  expect(grouped_df.features("value").as_series().to_vec<double>() == grouped_values,
         "DataFrame::ts().grouped should delegate to Slate::ts().grouped");
  const auto grouped_sum_frame = DataFrame({
      {"group", Slate(pluss::make_note({"a", "b", "a", "b"}))},
      {"value", Slate(pluss::make_series(std::vector<double>{1, 10, 3, 20}))},
  });
  const auto grouped_sum = grouped_sum_frame.features("value").ts().grouped(
      grouped_sum_frame.features("group"), 2, pluss::table::WindowStatistic::Sum);
  expect(grouped_sum.as_series().to_vec<double>() ==
             std::vector<double>({1, 10, 4, 30}),
         "grouped moving sum");

  const auto timed_frame = DataFrame({
      {"time", Slate(pluss::make_series(std::vector<std::int64_t>{0, 1, 3, 4}))},
      {"value", Slate(pluss::make_series(std::vector<double>{1, 2, 3, 4}))},
  });
  const auto timed = timed_frame.features("value").ts().timed(
      timed_frame.features("time"), 3, TimeWindowStatistic::Mean);
  const auto timed_values = timed.as_series().to_vec<double>();
  near(timed_values[0], 1.0, 1e-9, "time window row 0");
  near(timed_values[1], 1.5, 1e-9, "time window row 1");
  near(timed_values[2], 2.5, 1e-9, "time window excludes left boundary");
  near(timed_values[3], 3.5, 1e-9, "time window row 3");
  const auto grouped_timed_frame = DataFrame({
      {"group", Slate(pluss::make_note({"a", "a", "b", "b"}))},
      {"time", Slate(pluss::make_timeline({
                   "2024-01-01T00:00:00Z",
                   "2024-01-02T00:00:00Z",
                   "2024-01-03T00:00:00Z",
                   "2024-01-04T00:00:00Z",
               }))},
      {"value", Slate(pluss::make_series(std::vector<double>{1.0, 3.0, 10.0, 20.0}))},
  });
  const auto grouped_timed = grouped_timed_frame.features("value").ts().rolling(
      grouped_timed_frame.features("time"),
      grouped_timed_frame.features("group"),
      "2D",
      WindowStatistic::Mean);
  expect(grouped_timed.as_series().to_vec<double>() ==
             std::vector<double>({1.0, 2.0, 10.0, 15.0}),
         "Slate::ts().rolling(axis, groups, duration) should compute grouped timed windows");
  const auto grouped_timed_df = grouped_timed_frame.features({"value"})
                                    .ts({
                                        .axis = grouped_timed_frame.features("time"),
                                        .group = grouped_timed_frame.features("group"),
                                    })
                                    .rolling("2D", WindowStatistic::Mean);
  expect(grouped_timed_df.features("value").as_series().to_vec<double>() ==
             grouped_timed.as_series().to_vec<double>(),
         "DataFrame::ts().rolling(duration) should delegate grouped timed windows to Slate");
  const auto timed_variance_frame = DataFrame({
      {"time", Slate(pluss::make_series(std::vector<std::int64_t>{0, 1, 3, 4}))},
      {"value", Slate(pluss::make_series(std::vector<double>{1, 2, 3, 4}))},
  });
  const auto timed_variance = timed_variance_frame.features("value").ts().timed(
      timed_variance_frame.features("time"), 3, pluss::table::TimeWindowStatistic::Var,
      StatisticOptions{.ddof = 1, .min_count = 2});
  near(timed_variance.as_series().to_vec<double>(2),
       0.5, 1e-9, "time moving variance");

  const auto grouped_window_frame = DataFrame({
      {"customer", Slate(pluss::make_note({"a", "a", "b", "b", "b"}))},
      {"event_time", Slate(pluss::make_timeline({
          "2024-01-01T00:00:00Z",
          "2024-01-20T00:00:00Z",
          "2024-01-01T00:00:00Z",
          "2024-01-10T00:00:00Z",
          "2024-02-15T00:00:00Z",
      }))},
      {"anchor_time", Slate(pluss::make_timeline({
          "2024-01-31T00:00:00Z",
          "2024-01-31T00:00:00Z",
          "2024-01-31T00:00:00Z",
          "2024-01-31T00:00:00Z",
          "2024-01-31T00:00:00Z",
      }))},
      {"spend", Slate(pluss::make_series(std::vector<double>{1, 4, 2, 5, 8}))},
  });
  const auto grouped_window = grouped_window_frame.features({"spend"})
                                  .ts({.axis = grouped_window_frame.features("event_time"),
                                       .group = grouped_window_frame.features("customer")})
                                  .window("30D", WindowClosure::Both)
                                  .sum();
  expect(grouped_window.features().size() == 2 &&
             grouped_window.get("customer").has_value() &&
             grouped_window.get("sum_spend").has_value(),
         "DataFrame ts grouped window should emit keys plus the aggregate feature");
  expect(grouped_window.features("customer").as_note().vector() ==
             std::vector<std::string_view>({"a", "b"}) &&
             grouped_window.features("sum_spend").as_series().to_vec<double>() ==
                 std::vector<double>({5.0, 8.0}),
         "DataFrame ts grouped window should anchor to the latest time in each group by default");

  const auto anchored_window = grouped_window_frame.features({"spend"})
                                   .ts({.axis = grouped_window_frame.features("event_time"),
                                        .group = grouped_window_frame.features("customer"),
                                        .anchor = grouped_window_frame.features("anchor_time")})
                                   .window("30D", WindowClosure::Both)
                                   .sum();
  expect(anchored_window.features("sum_spend").as_series().to_vec<double>() ==
             std::vector<double>({5.0, 7.0}),
         "DataFrame ts grouped window should support explicit anchor columns");
}

void test_slate_delegation_and_advanced_windows() {
  const auto source =
      Slate(pluss::make_series(std::vector<double>{1, 2, 3, 4}));
  const auto frame = DataFrame({{"x", source}});

  near(source.skew().as_series().to_vec<double>(0),
       frame.skew().as_series().to_vec<double>(0), 1e-12,
       "DataFrame skew delegates to Slate");
  expect(source.scan(ScanStatistic::Sum).as_series().to_vec<double>() ==
             frame.scan(ScanStatistic::Sum).features("x").as_series().to_vec<double>(),
         "DataFrame cumulative operation delegates to Slate");
  expect(source.ts().rolling(2, WindowStatistic::Sum)
             .as_series().to_vec<double>() ==
             frame.ts().rolling(2, WindowStatistic::Sum).features("x").as_series().to_vec<double>(),
         "DataFrame .ts() fixed window delegates to Slate");

  const auto resample_axis = Slate(pluss::make_timeline({
      "2024-01-01T00:00:00Z",
      "2024-01-01T12:00:00Z",
      "2024-01-02T00:00:00Z",
  }));
  const auto resample_values =
      Slate(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}));
  const auto right_labeled = resample_values.ts().resample(
      resample_axis,
      pluss::Timeline::ResampleOptions{
          .freq = "D",
          .label = pluss::Timeline::ResampleLabel::Right,
      },
      pluss::subsp::stats::ts::ResampleOp::Sum);
  expect(right_labeled.labels.as_timeline().day().to_vec<std::int32_t>() ==
             std::vector<std::int32_t>({2, 3}),
         "Slate ts resample should support right bin labels");
  expect(right_labeled.values.as_series().to_vec<double>() ==
             std::vector<double>({3.0, 3.0}),
         "Slate ts resample options should preserve aggregation");
  const auto chained_daily = resample_values.ts().resample(
      resample_axis,
      pluss::Timeline::ResampleOptions{.freq = "D"}).sum();
  expect(chained_daily.label().values().as_timeline().day().to_vec<std::int32_t>() ==
             std::vector<std::int32_t>({1, 2}) &&
             chained_daily.as_series().to_vec<double>() ==
             std::vector<double>({3.0, 3.0}),
         "Slate ts resample(...).sum() should materialize labels and aggregated values");
  const auto shifted_bins = resample_values.ts().resample(
      resample_axis,
      pluss::Timeline::ResampleOptions{
          .freq = "D",
          .offset_nanos = 12LL * 60LL * 60LL * 1'000'000'000LL,
      },
      pluss::subsp::stats::ts::ResampleOp::Sum);
  expect(shifted_bins.values.as_series().to_vec<double>() ==
             std::vector<double>({1.0, 5.0}),
         "Slate ts resample should support offset bin anchors");
  const auto six_hour_exact = resample_values.ts().resample(
      resample_axis,
      pluss::Timeline::ResampleOptions{.freq = "6H"}).asfreq();
  const auto six_hour_exact_vals = six_hour_exact.as_series().to_vec<double>();
  expect(six_hour_exact.label().values().as_timeline().hour().to_vec<std::int32_t>() ==
                 std::vector<std::int32_t>({0, 6, 12, 18, 0}) &&
             six_hour_exact_vals.size() == 5 &&
             std::abs(six_hour_exact_vals[0] - 1.0) < 1e-12 &&
             std::isnan(six_hour_exact_vals[1]) &&
             std::abs(six_hour_exact_vals[2] - 2.0) < 1e-12 &&
             std::isnan(six_hour_exact_vals[3]) &&
             std::abs(six_hour_exact_vals[4] - 3.0) < 1e-12,
         "Slate ts resample(...).asfreq() should reindex exactly onto the target frequency");
  const auto six_hour_ffill = resample_values.ts().resample(
      resample_axis,
      pluss::Timeline::ResampleOptions{.freq = "6H"}).ffill();
  expect(six_hour_ffill.as_series().to_vec<double>() ==
             std::vector<double>({1.0, 1.0, 2.0, 2.0, 3.0}),
         "Slate ts resample(...).ffill() should forward fill across inserted bins");
  const auto six_hour_bfill = resample_values.ts().resample(
      resample_axis,
      pluss::Timeline::ResampleOptions{.freq = "6H"}).bfill();
  expect(six_hour_bfill.as_series().to_vec<double>() ==
             std::vector<double>({1.0, 2.0, 2.0, 3.0, 3.0}),
         "Slate ts resample(...).bfill() should backward fill across inserted bins");
  const auto resample_calendar = resample_values.ts().calendar(resample_axis);
  expect(resample_calendar.get("axis_year").has_value() &&
             resample_calendar.get("axis_weekday").has_value() &&
             resample_calendar.labels().values().as_timeline().day().to_vec<std::int32_t>() ==
                 std::vector<std::int32_t>({1, 1, 2}),
         "Slate ts calendar(axis) should expand the supplied time axis into calendar features");
  const auto resample_frame = DataFrame({
      {"when", resample_axis},
      {"x", resample_values},
  });
  const auto frame_right_labeled = resample_frame.features({"x"}).ts({.axis = resample_frame.features("when")}).resample(
      pluss::Timeline::ResampleOptions{
          .freq = "D",
          .label = pluss::Timeline::ResampleLabel::Right,
      },
      pluss::subsp::stats::ts::ResampleOp::Sum);
  expect(frame_right_labeled.labels().values().as_timeline().day().to_vec<std::int32_t>() ==
             std::vector<std::int32_t>({2, 3}) &&
             frame_right_labeled.features("x").as_series().to_vec<double>() ==
             std::vector<double>({3.0, 3.0}),
         "DataFrame ts resample should delegate options through Slate");

  const auto lhs_axis = Slate(pluss::make_timeline({
      "2024-01-01T00:00:00Z",
      "2024-01-02T00:00:00Z",
      "2024-01-03T00:00:00Z",
  }));
  const auto rhs_axis = Slate(pluss::make_timeline({
      "2024-01-03T00:00:00Z",
      "2024-01-01T00:00:00Z",
  }));
  const auto lhs_values =
      Slate(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}));
  const auto rhs_values =
      Slate(pluss::make_series(std::vector<double>{1.0, 2.0}));
  const auto ts_sum = lhs_values.ts().add(rhs_values, lhs_axis, rhs_axis);
  expect(pluss::table::detail::valid_at(ts_sum, 0) &&
             !pluss::table::detail::valid_at(ts_sum, 1) &&
             pluss::table::detail::valid_at(ts_sum, 2),
         "Slate ts binary add should null-mark unmatched exact timestamps");
  expect(std::fabs(ts_sum.as_series().to_vec<double>(0) - 12.0) < 1e-9 &&
             std::fabs(ts_sum.as_series().to_vec<double>(2) - 31.0) < 1e-9,
         "Slate ts binary add should align by timestamp labels");

  const auto lhs_frame = DataFrame({
      {"when", lhs_axis},
      {"x", lhs_values},
  }).labels().set("when", true);
  const auto rhs_frame = DataFrame({
      {"when", rhs_axis},
      {"x", rhs_values},
  }).labels().set("when", true);
  const auto frame_ts_sum = lhs_frame.features({"x"}).ts().add(rhs_frame);
  expect(pluss::table::detail::valid_at(frame_ts_sum.features("x"), 0) &&
             !pluss::table::detail::valid_at(frame_ts_sum.features("x"), 1) &&
             pluss::table::detail::valid_at(frame_ts_sum.features("x"), 2) &&
             std::fabs(frame_ts_sum.features("x").as_series().to_vec<double>(0) - 12.0) < 1e-9 &&
             std::fabs(frame_ts_sum.features("x").as_series().to_vec<double>(2) - 31.0) < 1e-9,
         "DataFrame ts binary add should align on time case labels");

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
  expect(!pluss::table::detail::valid_at(left_closed, 0),
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
      frame.ts().weigh({1.0, 2.0}, WindowStatistic::Mean);
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

  const auto both_time = Slate(
      pluss::make_series(std::vector<double>{1, 2, 3, 4}))
      .ts().timed(
          Slate(pluss::make_series(
              std::vector<std::int64_t>{0, 1, 3, 4})),
          3, pluss::table::TimeWindowStatistic::Sum,
          StatisticOptions{}, WindowClosure::Both);
  near(both_time.as_series().to_vec<double>(2), 6.0, 1e-9,
       "both-closed time window includes its left boundary");

  const auto ewm = source.ts().ewm(0.5, WindowStatistic::Mean, true, true, 3);
  expect(!pluss::table::detail::valid_at(ewm, 1) &&
             pluss::table::detail::valid_at(ewm, 2),
         "EWM min_count is enforced");
}

void test_udfs_and_text() {
  const auto text = Slate(
      pluss::make_note({"alpha-10", "Beta-20", "red|blue"}));
  expect(text.upper().as_note().at(0) == "ALPHA-10",
         "text upper");
  expect(text.nt().length().as_series().to_vec<std::int64_t>(0) == 8,
         "Unicode character length");
  expect(text.nt().matches("[a-z]+").as_series().to_vec<bool>(0),
         "regex prefix match");
  expect(!text.nt().fullmatch("[a-z]+").as_series().to_vec<bool>(0),
         "regex full match");
  expect(text.nt().has("BETA", {.case_sensitive = false})
             .as_series().to_vec<bool>(1),
         "case-insensitive literal text has");
  expect(text.nt().has("[A-Z][a-z]+-[0-9]+", {.regex = true})
             .as_series().to_vec<bool>(1),
         "regex text has");
  expect(text.nt().count("[0-9]").as_series().to_vec<std::int64_t>(0) == 2,
         "regex text count");
  bool invalid_regex_rejected = false;
  try {
    (void)text.nt().matches("(");
  } catch (const pluss::Error&) {
    invalid_regex_rejected = true;
  }
  expect(invalid_regex_rejected,
         "invalid regex uses the centralized Pluss error path");
  const auto extracted =
      text.nt().extract("([A-Za-z]+)-([0-9]+)", {"word", "number"});
  expect(extracted.features("word").as_note().at(1) == "Beta",
         "regex capture");
  const auto found_digits = text.nt().findall("[0-9]");
  expect(found_digits.features("match_1").as_note().at(1) == "0",
         "regex findall expansion");
  const auto split = text.nt().split("-");
  expect(split.features("part_1").as_note().at(0) == "10",
         "text split");
  const auto regex_split =
      text.nt().split("[|-]", {.regex = true, .prefix = "piece"});
  expect(regex_split.features("piece_1").as_note().at(2) == "blue",
         "regex text split");
  const auto partition = text.nt().partition("-");
  expect(partition.features("part_2").as_note().at(1) == "20",
         "text partition");
  expect(text.nt().replace("[0-9]+", "NUM", {.regex = true})
             .as_note().at(0) == "alpha-NUM",
         "regex text replace");
  expect(text.nt().pad(10, pluss::table::TextAlign::Right, '.')
             .as_note().at(0) == "..alpha-10",
         "text padding");
  expect(text.nt().chars(0, 5).as_note().at(0) == "alpha",
         "text slicing");
  expect(text.nt().encode("hex").as_note().at(0) ==
             "616c7068612d3130",
         "hex encoding");
  const auto unicode = Slate(
      pluss::make_note({"Cafe\u0301", "Straße"}));
  expect(unicode.nt().normalize(pluss::table::UnicodeForm::NFC)
             .as_note().at(0) == "Café",
         "canonical Unicode normalization");
  expect(unicode.casefold().as_note().at(1) == "strasse",
         "Unicode case folding");
  const auto indicators = text.nt().indicate("|");
  expect(indicators.features("red").as_series().to_vec<bool>(2) &&
             indicators.features("blue").as_series().to_vec<bool>(2),
         "delimited text indicators");
  const DataFrame text_frame({
      {"raw", text},
  });
  expect(text_frame.features({"raw"}).nt().has("red").features("raw").as_series().to_vec<bool>(2),
         "DataFrame note has delegates to Slate");

  const auto mapped = Slate(
      pluss::make_series(std::vector<double>{1, 2, 3}))
      .map([](const SlateValue& value) {
        return SlateValue(Slate(value.as_series() * 2.0));
      });
  expect(mapped.as_series().to_vec<double>() ==
             std::vector<double>({2, 4, 6}),
         "Slate element UDF");
  auto map_validity = pluss::Series::ones(
      {3}, pluss::codem::ScalarType::Bool);
  reinterpret_cast<bool*>(map_validity.raw_data())[1] = false;
  std::int64_t callback_count = 0;
  const auto mapped_nullable = Slate(
      pluss::make_series(std::vector<double>{1, 0, 3}),
      map_validity).map([&](const SlateValue& value) {
        ++callback_count;
        return SlateValue(Slate(value.as_series() * 2.0));
      });
  expect(callback_count == 2 &&
             !pluss::table::detail::valid_at(mapped_nullable, 1),
         "element UDF skips and preserves null values");

  const auto frame = DataFrame({
      {"x", Slate(pluss::make_series(std::vector<double>{1, 2, 3}))},
      {"label", Slate(pluss::make_note({"a", "bb", "ccc"}))},
  });
  const auto case_values = frame.transform([](const DataFrame& row) {
    return SlateValue(Slate(
        row.features("x").as_series() * 10.0));
  }, Axis::Case);
  expect(case_values.as_series().to_vec<double>() ==
             std::vector<double>({10, 20, 30}),
         "case-wise UDF");
  const auto mapped_frame = frame.features({"x"}).map(
      [](const SlateValue& value) {
        return SlateValue(Slate(value.as_series() * 3.0));
      });
  expect(mapped_frame.features("x").as_series().to_vec<double>() ==
             std::vector<double>({3, 6, 9}),
         "DataFrame element UDF should apply through selected features");
  near(frame.fold([](const DataFrame& value) {
        return Slate(value.features("x").sum().as_series());
      }).as_series().to_vec<double>(0),
       6.0, 1e-9, "arbitrary aggregation UDF");
  expect(frame.flow([](const DataFrame& value) {
           return value.features({"label"}).nt().upper();
         }).features("label").as_note().at(1) == "BB",
         "chainable DataFrame flow");
  const auto preserved = frame.transform(
      [](const DataFrame& row) { return row; },
      Axis::Case, pluss::table::ResultShape::Preserve);
  expect(preserved.ncases() == frame.ncases() &&
             preserved.features() == frame.features(),
         "case apply preserve shape");
  const auto expanded = frame.transform(
      [](const DataFrame& row) {
        return row.extend(row, pluss::table::FrameMatchRule::Unite,
                                true, false);
      },
      Axis::Case, pluss::table::ResultShape::Expand);
  expect(expanded.ncases() == frame.ncases() * 2,
         "case apply expand shape");

  const auto atlas = Atlas({frame, frame});
  const auto atlas_moving =
      atlas.features({"x"}).ts().rolling(2, WindowStatistic::Sum);
  expect(atlas_moving.depth() == 2 &&
             atlas_moving.frame(1).features("x")
                     .as_series().to_vec<double>() ==
                 std::vector<double>({1, 3, 5}),
         "Atlas delegates windows to DataFrame");
  expect(atlas.flow([](const Atlas& value) {
           return value.map_frames([](const DataFrame& part) {
             return part.features({"label"}).nt().upper();
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
  expect(batch.features("value").kind() == pluss::table::SlateKind::Note,
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
  expect(batch.features("value").kind() == pluss::table::SlateKind::Series &&
             batch.features("value").as_series().scalar_type() == first_dtype,
         "all-null batch should retain the established numeric schema");
  expect(batch.features("value").isnull().as_series().all().item<bool>(),
         "all-null batch remains null after schema conversion");
  std::filesystem::remove(null_path);
}

void test_axis_dtype_and_host_exports() {
  const auto timeline = pluss::make_timeline({
      "2024-01-01T00:00:00Z",
      "2024-01-02T00:00:00Z",
  });
  const auto duration = pluss::duration::nanos({1'000'000'000LL, 2'000'000'000LL});
  const auto frame = DataFrame({
      {"amount", Slate(pluss::make_series(std::vector<double>{1.0, 2.0}))},
      {"city", Slate(pluss::make_note({"Accra", "Tema"}))},
      {"event", Slate(timeline)},
      {"axis", Slate(duration)},
  });

  expect(frame.features() ==
             std::vector<std::string>({"amount", "city", "event", "axis"}),
         "DataFrame::features() should expose feature names");

  const auto ptypes = frame.ptypes();
  expect(ptypes.size() == 4, "DataFrame::ptypes size mismatch");
  expect(ptypes["amount"] == "Series", "numeric ptype should report Series payload");
  expect(ptypes["city"] == "Note", "note ptype should be Note");
  expect(ptypes["event"] == "Timeline", "timeline ptype should be Timeline");
  expect(ptypes["axis"] == "Duration", "duration ptype should be Duration");

  const auto payloads = ptypes.payloads();
  expect(payloads.size() == 4, "DataFrame::ptypes payloads size mismatch");
  expect(payloads == std::vector<std::string>({"Series", "Note", "Timeline", "Duration"}),
         "DataFrame::ptypes payloads should preserve payload-family order");

  const auto dtypes = frame.dtypes();
  expect(dtypes.size() == 4, "DataFrame::dtypes size mismatch");
  expect(dtypes["amount"].find("Series<") == 0, "numeric dtype should include Series concrete dtype");
  expect(dtypes["city"] == "Note<utf8>", "note dtype should include Note concrete dtype");
  expect(dtypes["event"].find("Timeline<datetime64[ns, ") == 0, "timeline dtype should include timezone-aware concrete dtype");
  expect(dtypes["axis"] == "Duration<duration64[ns]>",
         "duration dtype should distinguish Duration concrete dtype");

  expect(frame.features("amount").ptype() == "Series", "Slate::ptype should expose Series payload family");
  expect(frame.features("axis").ptype() == "Duration", "Slate::ptype should expose Duration payload family");

  std::ostringstream stream;
  stream << ptypes;
  expect(stream.str().find("amount") != std::string::npos && stream.str().find("Series") != std::string::npos,
         "DataFrame::ptypes should be streamable");

  const auto scalar_note = pluss::make_note("hello").host();
  expect(scalar_note.is_scalar() && scalar_note.scalar() == "hello",
         "Note::host should export scalar string");
  const auto vector_note = pluss::make_note({"a", "b"}).host();
  expect(vector_note.is_vector() &&
             vector_note.vector() == std::vector<std::string>({"a", "b"}),
         "Note::host should export string vector");

  const auto timeline_host = timeline.host();
  expect(timeline_host.is_vector() &&
             timeline_host.vector().size() == 2 &&
             timeline_host.tz() == "UTC",
         "Timeline::host should export epoch-nanos vector with timezone metadata");

  const auto duration_host = duration.host();
  expect(duration_host.size() == 2 &&
             duration_host == std::vector<std::int64_t>({1'000'000'000LL, 2'000'000'000LL}),
         "Duration::host should export epoch-nanos vector");

  const auto amount_value = frame.features("amount").sum();
  near(amount_value.to_dbl(), 3.0, 1e-12,
       "SlateValue::to_dbl should extract a numeric scalar");
  near(amount_value.host().to_dbl(), 3.0, 1e-12,
       "SlateValue::host().to_dbl should extract a numeric scalar");
  expect(amount_value.to_int() == 3,
         "SlateValue::to_int should extract an integral numeric scalar");
  std::ostringstream raw_amount;
  raw_amount << amount_value.host();
  expect(raw_amount.str() == "3",
         "SlateValue::host stream should print raw numeric scalar without Series prose");

  const auto note_value = Slate(pluss::make_note({"Accra"})).value();
  expect(note_value.to_str() == "Accra",
         "SlateValue::to_str should extract a Note scalar");
  std::ostringstream raw_note;
  raw_note << note_value.host();
  expect(raw_note.str() == "Accra",
         "SlateValue::host stream should print raw Note scalar");

  const auto timeline_value = Slate(pluss::make_timeline({"2024-01-01T00:00:00Z"})).value();
  const auto timeline_nanos = pluss::make_timeline({"2024-01-01T00:00:00Z"}).ticks().front();
  expect(timeline_value.to_int() == timeline_nanos,
         "SlateValue::to_int should extract epoch nanoseconds from Timeline");
  expect(timeline_value.to_str().find("2024-01-01") != std::string::npos,
         "SlateValue::to_str should print Timeline as ISO text");
  std::ostringstream raw_time;
  raw_time << timeline_value.host();
  expect(raw_time.str().find("2024-01-01") != std::string::npos,
         "SlateValue::host stream should print raw Timeline ISO text");

  const auto duration_value = Slate(pluss::duration::nanos({5'000'000'000LL})).value();
  expect(duration_value.to_int() == 5'000'000'000LL,
         "SlateValue::to_int should extract nanoseconds from Duration");
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
      mps.var(Axis::Case, {.ddof = 0, .min_count = 4});
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
      {"group", Slate(pluss::make_note({"a", "b", "a", "b"}))},
      {"value", Slate(pluss::make_series(
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
      {"time", Slate(pluss::make_series(std::vector<std::int64_t>{0, 1, 3, 4}))},
      {"value", Slate(pluss::make_series(
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

int main(int argc, char** argv) {
  using Test = std::pair<std::string, std::function<void()>>;
  const std::vector<Test> tests{
      {"statistics", test_statistics},
      {"windows", test_windows},
      {"slate_delegation_and_advanced_windows", test_slate_delegation_and_advanced_windows},
      {"udfs_and_text", test_udfs_and_text},
      {"streamed_csv_schema", test_streamed_csv_schema},
      {"axis_dtype_and_host_exports", test_axis_dtype_and_host_exports},
      {"mps_parity", test_mps_parity},
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
      std::cout << "[PASS] " << name << '\n';
    } catch (const std::exception& error) {
      std::cerr << "[FAIL] " << name << '\n' << error.what() << '\n';
      return 1;
    }
  }

  if (argc > 1 && passed == 0) {
    std::cerr << "[FAIL] no requested test name matched\n";
    return 1;
  }
  std::cout << passed << " statistics/window tests passed\n";
  return 0;
}
