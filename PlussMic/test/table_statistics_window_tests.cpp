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
  expect(frame.shape() == std::make_pair<std::int64_t, std::int64_t>(4, 2),
         "DataFrame::shape should report cases by features");
  expect(frame.features("x").ncases() == 4,
         "Thread::ncases should match Thread length");
  expect(frame.features("x").shape() == std::make_pair<std::int64_t, std::int64_t>(4, 1),
         "Thread::shape should report cases by one feature");
  const auto logged_x = frame.log().features("x").as_series().to_vec<double>();
  near(logged_x[0], 0.0, 1e-6, "DataFrame::log should delegate numeric features to Thread::log");
  near(logged_x[1], std::log(2.0), 1e-6, "DataFrame::log should preserve numeric log values");
  const auto exp_x = frame.exp().features("x").as_series().to_vec<double>();
  near(exp_x[0], std::exp(1.0), 1e-5, "DataFrame::exp should delegate numeric features to Thread::exp");
  const auto temporal = Thread(pluss::codeh::Timeline::fromSeries(pluss::make_series(
      std::vector<std::int64_t>{
          1'704'067'200'000'000'000LL,
          pluss::codeh::Timeline::kNaT,
          1'704'153'600'000'000'000LL,
      },
      {3})));
  expect(temporal.isnull().as_series().to_vec<bool>() ==
             std::vector<bool>({false, true, false}),
         "Thread::isnull should treat Timeline NaT as null");
  expect(temporal.notnull().as_series().to_vec<bool>() ==
             std::vector<bool>({true, false, true}),
         "Thread::notnull should treat Timeline NaT as missing");
  expect(temporal.invalid().as_series().to_vec<bool>() ==
             std::vector<bool>({false, true, false}),
         "Thread::invalid should treat Timeline NaT as invalid");
  const auto temporal_frame = DataFrame({{"when", temporal}});
  expect(temporal_frame.isnull().features("when").as_series().to_vec<bool>() ==
             std::vector<bool>({false, true, false}),
         "DataFrame::isnull should delegate Timeline NaT handling through Thread::isnull");
  expect(temporal_frame.invalid().features("when").as_series().to_vec<bool>() ==
             std::vector<bool>({false, true, false}),
         "DataFrame::invalid should delegate Timeline NaT handling through Thread::invalid");
  const auto numeric_invalid = Thread(pluss::make_series(
      std::vector<double>{1.0, std::numeric_limits<double>::quiet_NaN(),
                          std::numeric_limits<double>::infinity(),
                          -std::numeric_limits<double>::infinity()}));
  expect(numeric_invalid.invalid().as_series().to_vec<bool>() ==
             std::vector<bool>({false, true, true, true}),
         "Thread::invalid should catch NaN and infinities for numeric Series");

  const auto axis_frame = DataFrame({
      {"A1", Thread(pluss::make_series(std::vector<double>{10, 20, 30, 40}))},
      {"A2", Thread(pluss::make_series(std::vector<double>{1, 2, 3, 4}))},
  }, Thread(pluss::make_note({"1001", "1002", "1003", "1004"})));
  expect(pluss::table::detail::label_kind(axis_frame.label().values()) == pluss::table::LabelKind::Case,
         "DataFrame constructor labels should be case label provenance");
  const auto column_sum = axis_frame.features({"A1", "A2"}).sum();
  expect(column_sum.size() == 2,
         "default DataFrame::sum should reduce down cases per feature");
  expect(column_sum.as_series().to_vec<double>() ==
             std::vector<double>({100.0, 10.0}),
         "default DataFrame::sum values should match pandas axis=0");
  expect(column_sum.caselbl().as_note().vector() ==
             std::vector<std::string_view>({"A1", "A2"}),
         "Axis::Case reductions should label the result by feature names");
  expect(pluss::table::detail::label_kind(column_sum) == pluss::table::LabelKind::Feature,
         "Axis::Case reductions should carry feature-label provenance");
  const auto row_sum = axis_frame.features({"A1", "A2"}).sum(Axis::Feature);
  expect(row_sum.size() == axis_frame.ncases(),
         "Axis::Feature DataFrame::sum should reduce across features per case");
  expect(row_sum.caselbl().as_note().vector() ==
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
  const auto direct_assigned_axis = axis_frame.set_features("Assignments2", row_sum);
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
    (void)axis_frame.set_features("ColumnTotals2", column_sum);
  } catch (const std::exception&) {
    set_features_feature_labeled_default_threw = true;
  }
  expect(set_features_feature_labeled_default_threw,
         "set_features default should reject feature-labeled reductions with case-length mismatch");
  const auto direct_feature_indexed = axis_frame.set_features(
      "ColumnTotals2",
      column_sum,
      {.align = pluss::table::AlignBy::Caselbl});
  expect(pluss::table::detail::validity_mask(direct_feature_indexed.features("ColumnTotals2"))->to_vec<bool>() ==
             std::vector<bool>({false, false, false, false}),
         "set_features with explicit case-label alignment should null-fill misses");

  auto scholarship = Thread(pluss::make_series(std::vector<double>{1500, 2500, 1000}))
                         .caselbl(Thread(pluss::make_note({"1003", "1001", "1004"})));
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
    aligned_axis.set("one_row_series") = Thread(pluss::make_series(std::vector<double>{7235.0}));
  } catch (const std::exception&) {
    one_row_default_threw = true;
  }
  expect(one_row_default_threw,
         "one-element 1D Thread assignment should not broadcast or align by default");
  const auto default_cases = Thread(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0})).caselbl();
  expect(default_cases.as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1, 2}),
         "Thread without explicit cases should expose default RangeIndex-style cases");
  expect(pluss::table::detail::label_kind(Thread(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}))) ==
             pluss::table::LabelKind::Default,
         "Thread without explicit labels should carry default-label provenance");
  const auto range_labeled =
      Thread(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}), {.rangelbl = true});
  expect(range_labeled.caselbl().as_note().vector() ==
             std::vector<std::string_view>({"0", "1", "2"}),
         "Thread(..., {.rangelbl=true}) should materialize explicit range labels");
  expect(pluss::table::detail::label_kind(range_labeled) == pluss::table::LabelKind::Case,
         "Thread(..., {.rangelbl=true}) should carry case-label provenance");
  const auto chain_range_labeled =
      Thread(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0})).rangelbl();
  expect(chain_range_labeled.caselbl().as_note().vector() ==
             std::vector<std::string_view>({"0", "1", "2"}),
         "Thread::rangelbl() should materialize explicit range labels");
  expect(pluss::table::detail::label_kind(chain_range_labeled) == pluss::table::LabelKind::Case,
         "Thread::rangelbl() should carry case-label provenance");
  expect(pluss::table::detail::label_kind(
             Thread(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}))
                 .caselbl(Thread(pluss::make_note({"a", "b", "c"})))) == pluss::table::LabelKind::Custom,
         "Thread::caselbl(labels) should carry custom-label provenance");

  const auto source_labeled = Thread(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}))
                                  .caselbl(Thread(pluss::make_note({"b", "a", "c"})));
  const auto target_case_labels = Thread(pluss::make_note({"a", "b", "d"}));
  const auto aligned_by_case = source_labeled.align(
      target_case_labels,
      pluss::table::AlignOptions{.align = pluss::table::AlignBy::Caselbl});
  expect(aligned_by_case.caselbl().as_note().vector() ==
             std::vector<std::string_view>({"a", "b", "d"}),
         "Thread::align(Caselbl) should carry target case labels");
  expect(aligned_by_case.as_series().to_vec<double>()[0] == 20.0 &&
             aligned_by_case.as_series().to_vec<double>()[1] == 10.0,
         "Thread::align(Caselbl) should reorder values by label");
  expect(pluss::table::detail::validity_mask(aligned_by_case)->to_vec<bool>() ==
             std::vector<bool>({true, true, false}),
         "Thread::align(Caselbl) should null-mark missing target labels by default");
  expect(pluss::table::detail::label_kind(aligned_by_case) == pluss::table::LabelKind::Case,
         "Thread::align(Caselbl) should carry case-label provenance");

  const auto filled_by_case = source_labeled.align(
      target_case_labels,
      pluss::table::AlignOptions{.align = pluss::table::AlignBy::Caselbl, .fill = 0.0});
  expect(filled_by_case.as_series().to_vec<double>() ==
             std::vector<double>({20.0, 10.0, 0.0}),
         "Thread::align(Caselbl, fill) should fill missing values");
  expect(!pluss::table::detail::has_validity_mask(filled_by_case),
         "Thread::align(Caselbl, fill) should clear validity mask after filling");

  const auto feature_thread = Thread(pluss::make_series(std::vector<double>{1.5, 2.5}))
                                  .caselbl(Thread(pluss::make_note({"A2", "A1"})));
  const auto feature_labels = Thread(pluss::make_note({"A1", "A2"}));
  const auto aligned_by_feature = feature_thread.align(
      feature_labels,
      pluss::table::AlignOptions{.align = pluss::table::AlignBy::Feature});
  expect(aligned_by_feature.as_series().to_vec<double>() ==
             std::vector<double>({2.5, 1.5}),
         "Thread::align(Feature) should align one value per feature by feature name");
  expect(pluss::table::detail::label_kind(aligned_by_feature) == pluss::table::LabelKind::Feature,
         "Thread::align(Feature) should carry feature-label provenance");

  bool positional_mismatch_threw = false;
  try {
    (void)source_labeled.align(
        Thread(pluss::make_note({"a", "b"})),
        pluss::table::AlignOptions{.align = pluss::table::AlignBy::Pos});
  } catch (const std::exception&) {
    positional_mismatch_threw = true;
  }
  expect(positional_mismatch_threw,
         "Thread::align(Pos) should throw on length mismatch");

  bool both_threw = false;
  try {
    (void)source_labeled.align(
        target_case_labels,
        pluss::table::AlignOptions{.align = pluss::table::AlignBy::Both});
  } catch (const std::exception&) {
    both_threw = true;
  }
  expect(both_threw,
         "Thread::align(Both) should reject DataFrame-only alignment mode");
  const auto default_left = Thread(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}));
  const auto default_right = Thread(pluss::make_series(std::vector<double>{1.0, 2.0}));
  bool default_length_mismatch_threw = false;
  try {
    (void)(default_left + default_right);
  } catch (const std::exception&) {
    default_length_mismatch_threw = true;
  }
  expect(default_length_mismatch_threw,
         "Thread arithmetic should be strict positional by default");
  const auto one_value_1d = Thread(pluss::make_series(std::vector<double>{1.0}));
  bool one_value_1d_threw = false;
  try {
    (void)(default_left + one_value_1d);
  } catch (const std::exception&) {
    one_value_1d_threw = true;
  }
  expect(one_value_1d_threw,
         "one-element 1D Thread should not broadcast or align by default");
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

  const auto s1 = Thread(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}))
                      .caselbl(Thread(pluss::make_note({"Alice", "Bob", "Carol"})));
  const auto s2 = Thread(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}))
                      .caselbl(Thread(pluss::make_note({"Carol", "Alice", "Bob"})));
  const auto positional_sum = s1 + s2;
  expect(positional_sum.as_series().to_vec<double>() ==
             std::vector<double>({11.0, 22.0, 33.0}),
         "Thread arithmetic should use positional order by default");
  expect(s1.add(s2).as_series().to_vec<double>() == positional_sum.as_series().to_vec<double>(),
         "Thread::add should match Thread operator+");
  expect(s1.sub(s2).as_series().to_vec<double>() ==
             std::vector<double>({9.0, 18.0, 27.0}),
         "Thread::sub should expose method-form arithmetic");
  expect(s1.mul(2.0).as_series().to_vec<double>() ==
             std::vector<double>({20.0, 40.0, 60.0}),
         "Thread::mul(double) should expose scalar arithmetic");
  expect(s1.gt(s2).as_series().to_vec<bool>() ==
             std::vector<bool>({true, true, true}),
         "Thread::gt should match comparison operator semantics");
  expect((s1.gt(15.0) & s1.lt(35.0)).as_series().to_vec<bool>() ==
             s1.between(15.0, 35.0, false).as_series().to_vec<bool>(),
         "Thread::between should compose comparison methods");
  expect(Thread(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}))
             .cumsum()
             .as_series()
             .to_vec<double>() == std::vector<double>({1.0, 3.0, 6.0}),
         "Thread::cumsum should delegate to Thread::scan");
  near(Thread(pluss::make_series(std::vector<double>{2.0, 3.0, 4.0}))
           .prod()
           .to_dbl(),
       24.0,
       1e-12,
       "Thread::prod should expose the 1D product reduction");
  const auto labelled_scores =
      Thread(pluss::make_series(std::vector<double>{20.0, 10.0, 30.0}))
          .caselbl(Thread(pluss::make_note({"Alice", "Bob", "Carol"})));
  expect(labelled_scores.csmin().to_str() == "Bob",
         "Thread::csmin should return the case label at the minimum value");
  expect(labelled_scores.csmax().to_str() == "Carol",
         "Thread::csmax should return the case label at the maximum value");
  expect(labelled_scores.top(2, false).as_series().to_vec<double>() ==
             std::vector<double>({30.0, 20.0}),
         "Thread::top should return the largest values in descending order");
  expect(labelled_scores.bottom(2, false).as_series().to_vec<double>() ==
             std::vector<double>({10.0, 20.0}),
         "Thread::bottom should return the smallest values in ascending order");
  const auto appended_scores =
      labelled_scores.append(Thread(pluss::make_series(std::vector<double>{40.0}))
                                 .caselbl(Thread(pluss::make_note({"David"}))));
  expect(appended_scores.as_series().to_vec<double>() ==
             std::vector<double>({20.0, 10.0, 30.0, 40.0}),
         "Thread::append should concatenate 1D Series payloads");
  expect(appended_scores.caselbl().as_note().vector() ==
             std::vector<std::string_view>({"Alice", "Bob", "Carol", "David"}),
         "Thread::append should preserve and concatenate case labels");
  expect(Thread(pluss::make_note({"x", "y"}))
             .append(Thread(pluss::make_note({"z"})))
             .as_note()
             .vector() == std::vector<std::string_view>({"x", "y", "z"}),
         "Thread::append should concatenate Note payloads");
  expect(labelled_scores.sample(2, std::uint64_t{7}).ncases() == 2,
         "Thread::sample should expose deterministic 1D sampling");
  expect(labelled_scores.sample(5, std::uint64_t{7}, true).ncases() == 5,
         "Thread::sample should support replacement");
  expect(labelled_scores.shuffle(7).ncases() == labelled_scores.ncases(),
         "Thread::shuffle should preserve the 1D row count");
  expect(Thread(pluss::make_note({"alpha", "beta", "gamma"}))
             .has("beta")
             .as_series()
             .to_vec<bool>() == std::vector<bool>({false, true, false}),
         "Thread::has should return an exact-membership Thread mask");
  expect(positional_sum.caselbl().as_note().vector() ==
             std::vector<std::string_view>({"Alice", "Bob", "Carol"}),
         "Thread arithmetic should preserve left labels under positional semantics");
  const auto aligned_sum = s1 + s2.align(s1.caselbl(), {.align = pluss::table::AlignBy::Caselbl});
  expect(aligned_sum.as_series().to_vec<double>() ==
             std::vector<double>({12.0, 23.0, 31.0}),
         "Thread arithmetic should support explicit case-label alignment via Thread::align");

  const auto left_missing = Thread(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}))
                                .caselbl(Thread(pluss::make_note({"A", "B", "C"})));
  const auto right_missing = Thread(pluss::make_series(std::vector<double>{1.0, 2.0}))
                                 .caselbl(Thread(pluss::make_note({"B", "D"})));
  bool missing_sum_threw = false;
  try {
    (void)(left_missing + right_missing);
  } catch (const std::exception&) {
    missing_sum_threw = true;
  }
  expect(missing_sum_threw,
         "Thread arithmetic should reject unequal lengths unless the user explicitly aligns first");

  const auto bool_a = Thread(pluss::make_series(std::vector<bool>{true, false, true}))
                          .caselbl(Thread(pluss::make_note({"x", "y", "z"})));
  const auto bool_b = Thread(pluss::make_series(std::vector<bool>{true, true}))
                          .caselbl(Thread(pluss::make_note({"z", "x"})));
  bool bool_and_threw = false;
  try {
    (void)(bool_a & bool_b);
  } catch (const std::exception&) {
    bool_and_threw = true;
  }
  expect(bool_and_threw,
         "Thread logical ops should reject unequal lengths unless the user explicitly aligns first");

  const auto cmp = s1 > s2;
  expect(cmp.caselbl().as_note().vector() ==
             std::vector<std::string_view>({"Alice", "Bob", "Carol"}),
         "Thread comparisons should carry left labels under positional semantics");
  expect(cmp.as_series().to_vec<bool>() ==
             std::vector<bool>({true, true, true}),
         "Thread comparisons should compare positional values by default");

  const auto corr_left = Thread(pluss::make_series(std::vector<double>{1.0, 2.0}))
                             .caselbl(Thread(pluss::make_note({"A", "B"})));
  const auto corr_right = Thread(pluss::make_series(std::vector<double>{20.0, 10.0}))
                              .caselbl(Thread(pluss::make_note({"B", "A"})));
  near(corr_left.corr(corr_right), 1.0, 1e-12,
       "Thread::corr should align by case labels before computing");
  near(corr_left.corr_with(corr_right).to_dbl(), 1.0, 1e-12,
       "Thread::corr_with should align by case labels before computing");
  near(corr_left.covar(corr_right, {.ddof = 0, .min_count = 2}).to_dbl(), 2.5, 1e-12,
       "Thread::covar should align by case labels before computing");
  near(corr_left.matmul(corr_right).to_dbl(), 50.0, 1e-12,
       "Thread::matmul should align by case labels before computing");
  const auto filter_mask = Thread(pluss::make_series(std::vector<bool>{true, false}))
                               .caselbl(Thread(pluss::make_note({"1003", "1001"})));
  const auto filtered_axis = axis_frame.filter(filter_mask);
  expect(filtered_axis.label().values().as_note().vector() ==
             std::vector<std::string_view>({"1003"}),
         "DataFrame::filter(Thread) should align Thread masks by case labels by default");
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
         "DataFrame::put_(Thread mask) should align Thread masks by case labels by default");
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
      {"A", Thread(pluss::make_series(std::vector<double>{10.0, 20.0}))},
      {"B", Thread(pluss::make_series(std::vector<double>{100.0, 200.0}))},
  }, Thread(pluss::make_note({"r1", "r2"})));
  const auto frame_right_rows = DataFrame({
      {"A", Thread(pluss::make_series(std::vector<double>{1.0, 2.0}))},
      {"B", Thread(pluss::make_series(std::vector<double>{10.0, 20.0}))},
  }, Thread(pluss::make_note({"r2", "r1"})));
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
      {"B", Thread(pluss::make_series(std::vector<double>{1.0, 2.0}))},
      {"A", Thread(pluss::make_series(std::vector<double>{10.0, 20.0}))},
  }, Thread(pluss::make_note({"r1", "r2"})));
  const auto frame_feature = frame_left.add(frame_right_features, {.align = pluss::table::AlignBy::Feature});
  expect(frame_feature.features("A").as_series().to_vec<double>() ==
             std::vector<double>({20.0, 40.0}) &&
             frame_feature.features("B").as_series().to_vec<double>() ==
             std::vector<double>({101.0, 202.0}),
         "DataFrame::add(DataFrame, Feature) should align columns by feature label");

  const auto feature_offsets = Thread(pluss::make_series(std::vector<double>{10.0, 100.0}))
                                   .caselbl(Thread(pluss::make_note({"B", "A"})));
  const auto feature_pos = frame_left.add(feature_offsets);
  expect(feature_pos.features("A").as_series().to_vec<double>() ==
             std::vector<double>({20.0, 30.0}) &&
             feature_pos.features("B").as_series().to_vec<double>() ==
             std::vector<double>({200.0, 300.0}),
         "DataFrame::add(Thread) should broadcast one value per feature positionally by default");
  const auto feature_aligned = frame_left.add(feature_offsets, {.align = pluss::table::AlignBy::Feature});
  expect(feature_aligned.features("A").as_series().to_vec<double>() ==
             std::vector<double>({110.0, 120.0}) &&
             feature_aligned.features("B").as_series().to_vec<double>() ==
             std::vector<double>({110.0, 210.0}),
         "DataFrame::add(Thread, Feature) should align one value per feature by feature label");

  const auto case_offsets = Thread(pluss::make_series(std::vector<double>{1000.0, 100.0}))
                                .caselbl(Thread(pluss::make_note({"r2", "r1"})));
  const auto case_pos = frame_left.add(case_offsets, Axis::Case);
  expect(case_pos.features("A").as_series().to_vec<double>() ==
             std::vector<double>({1010.0, 120.0}),
         "DataFrame::add(Thread, Axis::Case) should broadcast one value per row positionally by default");
  const auto case_aligned =
      frame_left.add(case_offsets, Axis::Case, {.align = pluss::table::AlignBy::Caselbl});
  expect(case_aligned.features("A").as_series().to_vec<double>() ==
             std::vector<double>({110.0, 1020.0}),
         "DataFrame::add(Thread, Axis::Case, Caselbl) should align row values by case label");
  const auto cmp_feature = frame_left.gt(feature_offsets, {.align = pluss::table::AlignBy::Feature});
  expect(cmp_feature.features("A").as_series().to_vec<bool>() ==
             std::vector<bool>({false, false}) &&
             cmp_feature.features("B").as_series().to_vec<bool>() ==
             std::vector<bool>({true, true}),
         "DataFrame comparison with Thread should use the same feature-alignment policy");
  const auto text_frame = DataFrame({
      {"name", Thread(pluss::make_note({"alpha", "beta"}))},
      {"score", Thread(pluss::make_series(std::vector<double>{1.0, 2.0}))},
  }, Thread(pluss::make_note({"r1", "r2"})));
  const auto text_has = text_frame.has(Thread(pluss::make_note({"alpha", "2"})));
  expect(text_has.features("name").as_series().to_vec<bool>() ==
             std::vector<bool>({true, false}) &&
             text_has.features("score").as_series().to_vec<bool>() ==
             std::vector<bool>({false, true}),
         "DataFrame::has should delegate membership checks to each Thread");
  expect(text_frame.eq("beta").features("name").as_series().to_vec<bool>() ==
             std::vector<bool>({false, true}),
         "DataFrame::eq(string) should delegate text comparisons to Thread");
  expect(text_frame.le("beta").features("name").as_series().to_vec<bool>() ==
             std::vector<bool>({true, true}),
         "DataFrame::le(string) should delegate text ordering to Thread");
  const auto bool_frame = DataFrame({
      {"a", Thread(pluss::make_series(std::vector<bool>{true, false}))},
      {"b", Thread(pluss::make_series(std::vector<bool>{false, true}))},
  }, Thread(pluss::make_note({"r1", "r2"})));
  const auto bool_mask = Thread(pluss::make_series(std::vector<bool>{true, false}));
  expect(bool_frame.land(bool_mask).features("a").as_series().to_vec<bool>() ==
             std::vector<bool>({true, false}) &&
             bool_frame.lor(bool_frame).features("b").as_series().to_vec<bool>() ==
             std::vector<bool>({false, true}),
         "DataFrame logical methods should delegate to Thread logical methods");

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
      {"a", Thread(pluss::make_series(std::vector<double>{1, 10}))},
      {"b", Thread(pluss::make_series(std::vector<double>{2, 20}))},
      {"c", Thread(pluss::make_series(std::vector<double>{3, 30}))},
  }).cumsum(Axis::Feature);
  expect(row_scan.features("a").as_series().to_vec<double>() ==
             std::vector<double>({1, 10}) &&
             row_scan.features("b").as_series().to_vec<double>() ==
                 std::vector<double>({3, 30}) &&
             row_scan.features("c").as_series().to_vec<double>() ==
                 std::vector<double>({6, 60}),
         "Axis::Feature cumsum should scan across features per case like pandas axis=1");
  const auto row_max = DataFrame({
      {"a", Thread(pluss::make_series(std::vector<double>{3, 10}))},
      {"b", Thread(pluss::make_series(std::vector<double>{2, 40}))},
      {"c", Thread(pluss::make_series(std::vector<double>{5, 30}))},
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
      {"x", Thread(
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
      {"a", Thread(pluss::make_series(std::vector<double>{1, 10}))},
      {"b", Thread(pluss::make_series(std::vector<double>{2, 20}), row_valid)},
      {"c", Thread(pluss::make_series(std::vector<double>{3, 30}))},
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
  const auto grouped_df = grouped_frame.ts("value").grouped("group", 2, WindowStatistic::Mean);
  expect(grouped_df.features("value").as_series().to_vec<double>() == grouped_values,
         "DataFrame::ts().grouped should delegate to Thread::ts().grouped");
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

  const auto resample_axis = Thread(pluss::make_timeline({
      "2024-01-01T00:00:00Z",
      "2024-01-01T12:00:00Z",
      "2024-01-02T00:00:00Z",
  }));
  const auto resample_values =
      Thread(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}));
  const auto right_labeled = resample_values.ts().resample(
      resample_axis,
      pluss::TimeIndex::ResampleOptions{
          .freq = "D",
          .label = pluss::TimeIndex::ResampleLabel::Right,
      },
      pluss::subsp::stats::ts::ResampleOp::Sum);
  expect(right_labeled.labels.as_timeline().day().to_vec<std::int32_t>() ==
             std::vector<std::int32_t>({2, 3}),
         "Thread ts resample should support right bin labels");
  expect(right_labeled.values.as_series().to_vec<double>() ==
             std::vector<double>({3.0, 3.0}),
         "Thread ts resample options should preserve aggregation");
  const auto shifted_bins = resample_values.ts().resample(
      resample_axis,
      pluss::TimeIndex::ResampleOptions{
          .freq = "D",
          .offset_nanos = 12LL * 60LL * 60LL * 1'000'000'000LL,
      },
      pluss::subsp::stats::ts::ResampleOp::Sum);
  expect(shifted_bins.values.as_series().to_vec<double>() ==
             std::vector<double>({1.0, 5.0}),
         "Thread ts resample should support offset bin anchors");
  const auto resample_frame = DataFrame({
      {"when", resample_axis},
      {"x", resample_values},
  });
  const auto frame_right_labeled = resample_frame.ts("x").resample(
      "when",
      pluss::TimeIndex::ResampleOptions{
          .freq = "D",
          .label = pluss::TimeIndex::ResampleLabel::Right,
      },
      pluss::subsp::stats::ts::ResampleOp::Sum);
  expect(frame_right_labeled.label().values().as_timeline().day().to_vec<std::int32_t>() ==
             std::vector<std::int32_t>({2, 3}) &&
             frame_right_labeled.features("x").as_series().to_vec<double>() ==
             std::vector<double>({3.0, 3.0}),
         "DataFrame ts resample should delegate options through Thread");

  const auto lhs_axis = Thread(pluss::make_timeline({
      "2024-01-01T00:00:00Z",
      "2024-01-02T00:00:00Z",
      "2024-01-03T00:00:00Z",
  }));
  const auto rhs_axis = Thread(pluss::make_timeline({
      "2024-01-03T00:00:00Z",
      "2024-01-01T00:00:00Z",
  }));
  const auto lhs_values =
      Thread(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}));
  const auto rhs_values =
      Thread(pluss::make_series(std::vector<double>{1.0, 2.0}));
  const auto ts_sum = lhs_values.ts().add(rhs_values, lhs_axis, rhs_axis);
  expect(ts_sum.is_valid(0) && !ts_sum.is_valid(1) && ts_sum.is_valid(2),
         "Thread ts binary add should null-mark unmatched exact timestamps");
  expect(std::fabs(ts_sum.as_series().to_vec<double>(0) - 12.0) < 1e-9 &&
             std::fabs(ts_sum.as_series().to_vec<double>(2) - 31.0) < 1e-9,
         "Thread ts binary add should align by timestamp labels");

  const auto lhs_frame = DataFrame({
      {"when", lhs_axis},
      {"x", lhs_values},
  }).label().set("when", true);
  const auto rhs_frame = DataFrame({
      {"when", rhs_axis},
      {"x", rhs_values},
  }).label().set("when", true);
  const auto frame_ts_sum = lhs_frame.ts("x").add(rhs_frame);
  expect(frame_ts_sum.features("x").is_valid(0) &&
             !frame_ts_sum.features("x").is_valid(1) &&
             frame_ts_sum.features("x").is_valid(2) &&
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
  expect(text.note().has("BETA", {.case_sensitive = false})
             .as_series().to_vec<bool>(1),
         "case-insensitive literal text has");
  expect(text.note().has("[A-Z][a-z]+-[0-9]+", {.regex = true})
             .as_series().to_vec<bool>(1),
         "regex text has");
  expect(text.note().count("[0-9]").as_series().to_vec<std::int64_t>(0) == 2,
         "regex text count");
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
  const auto found_digits = text.note().findall("[0-9]");
  expect(found_digits.features("match_1").as_note().at(1) == "0",
         "regex findall expansion");
  const auto split = text.note().split("-");
  expect(split.features("part_1").as_note().at(0) == "10",
         "text split");
  const auto regex_split =
      text.note().split("[|-]", {.regex = true, .prefix = "piece"});
  expect(regex_split.features("piece_1").as_note().at(2) == "blue",
         "regex text split");
  const auto partition = text.note().partition("-");
  expect(partition.features("part_2").as_note().at(1) == "20",
         "text partition");
  expect(text.note().replace("[0-9]+", "NUM", {.regex = true})
             .as_note().at(0) == "alpha-NUM",
         "regex text replace");
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
  const DataFrame text_frame({
      {"raw", text},
  });
  expect(text_frame.note("raw").has("red").features("raw").as_series().to_vec<bool>(2),
         "DataFrame note has delegates to Thread");

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
             preserved.features() == frame.features(),
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

void test_axis_dtype_and_host_exports() {
  const auto timeline = pluss::make_timeline({
      "2024-01-01T00:00:00Z",
      "2024-01-02T00:00:00Z",
  });
  const auto timeindex = pluss::make_timeindex({
      "2024-01-01T00:00:00Z",
      "2024-01-02T00:00:00Z",
  });
  const auto frame = DataFrame({
      {"amount", Thread(pluss::make_series(std::vector<double>{1.0, 2.0}))},
      {"city", Thread(pluss::make_note({"Accra", "Tema"}))},
      {"event", Thread(timeline)},
      {"axis", Thread(timeindex)},
  });

  expect(frame.features() ==
             std::vector<std::string>({"amount", "city", "event", "axis"}),
         "DataFrame::features() should expose feature names");

  const auto dtypes = frame.dtypes();
  expect(dtypes.size() == 4, "DataFrame::dtypes size mismatch");
  expect(dtypes["amount"] == "Series", "numeric dtype should report Series payload");
  expect(dtypes["city"] == "Note", "note dtype should be Note");
  expect(dtypes["event"] == "Timeline", "timeline dtype should be Timeline");
  expect(dtypes["axis"] == "TimeIndex", "time index dtype should be TimeIndex");

  std::ostringstream stream;
  stream << dtypes;
  expect(stream.str().find("amount") != std::string::npos &&
             stream.str().find("Series") != std::string::npos,
         "DataFrame::dtypes should be streamable");

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

  const auto timeindex_host = timeindex.host();
  expect(timeindex_host.size() == 2 &&
             timeindex_host == timeline.to_epoch_nanos_vector(),
         "TimeIndex::host should export epoch-nanos vector");

  const auto amount_value = frame.features("amount").sum();
  near(amount_value.to_dbl(), 3.0, 1e-12,
       "ThreadValue::to_dbl should extract a numeric scalar");
  near(amount_value.host().to_dbl(), 3.0, 1e-12,
       "ThreadValue::host().to_dbl should extract a numeric scalar");
  expect(amount_value.to_int() == 3,
         "ThreadValue::to_int should extract an integral numeric scalar");
  std::ostringstream raw_amount;
  raw_amount << amount_value.host();
  expect(raw_amount.str() == "3",
         "ThreadValue::host stream should print raw numeric scalar without Series prose");

  const auto note_value = Thread(pluss::make_note({"Accra"})).value();
  expect(note_value.to_str() == "Accra",
         "ThreadValue::to_str should extract a Note scalar");
  std::ostringstream raw_note;
  raw_note << note_value.host();
  expect(raw_note.str() == "Accra",
         "ThreadValue::host stream should print raw Note scalar");

  const auto timeline_value = Thread(pluss::make_timeline({"2024-01-01T00:00:00Z"})).value();
  const auto timeline_nanos = pluss::make_timeline({"2024-01-01T00:00:00Z"}).to_epoch_nanos_vector().front();
  expect(timeline_value.to_int() == timeline_nanos,
         "ThreadValue::to_int should extract epoch nanoseconds from Timeline");
  expect(timeline_value.to_str().find("2024-01-01") != std::string::npos,
         "ThreadValue::to_str should print Timeline as ISO text");
  std::ostringstream raw_time;
  raw_time << timeline_value.host();
  expect(raw_time.str().find("2024-01-01") != std::string::npos,
         "ThreadValue::host stream should print raw Timeline ISO text");

  const auto timeindex_value = Thread(pluss::make_timeindex({"2024-01-02T00:00:00Z"})).value();
  const auto timeindex_nanos = pluss::make_timeindex({"2024-01-02T00:00:00Z"}).host().front();
  expect(timeindex_value.to_int() == timeindex_nanos,
         "ThreadValue::to_int should extract epoch nanoseconds from TimeIndex");
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

int main(int argc, char** argv) {
  using Test = std::pair<std::string, std::function<void()>>;
  const std::vector<Test> tests{
      {"statistics", test_statistics},
      {"windows", test_windows},
      {"thread_delegation_and_advanced_windows", test_thread_delegation_and_advanced_windows},
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
