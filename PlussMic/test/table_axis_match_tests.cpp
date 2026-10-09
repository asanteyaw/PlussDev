#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "pluss/pluss.h"

namespace {

using pluss::table::DataFrame;
using pluss::table::Axis;
using pluss::table::FrameMatchOptions;
using pluss::table::FrameMatchRule;
using pluss::table::RepeatedCaseRule;
using pluss::table::Slate;
using pluss::table::Atlas;
using pluss::scribble::Registry;
using pluss::scribble::Axel;

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
  expect(threw, message);
}

DataFrame make_frame(
    std::initializer_list<std::pair<std::string, Slate>> features,
    std::initializer_list<std::string_view> cases) {
  return DataFrame(
      features,
      Slate(pluss::make_note(cases)));
}

void test_union_matching_for_frame_math() {
  const auto left = make_frame(
      {
          {"x", Slate(pluss::make_series(std::vector<double>{1.0, 2.0}))},
          {"y", Slate(pluss::make_series(std::vector<double>{10.0, 20.0}))},
      },
      {"A", "C"});
  const auto right = make_frame(
      {
          {"y", Slate(pluss::make_series(std::vector<double>{100.0, 200.0}))},
          {"z", Slate(pluss::make_series(std::vector<double>{7.0, 8.0}))},
      },
      {"B", "A"});

  const auto positional = left.add(right);
  expect(positional.features() == std::vector<std::string>({"x", "y"}),
         "default frame arithmetic should preserve left feature labels under positional semantics");
  expect(positional.label().values().as_note().vector() ==
             std::vector<std::string_view>({"A", "C"}),
         "default frame arithmetic should preserve left case labels under positional semantics");
  expect(positional.features("x").as_series().to_vec<double>() ==
             std::vector<double>({101.0, 202.0}),
         "default frame arithmetic should pair values by position");
  const auto result = left.add(right, {.align = pluss::table::AlignBy::Both});
  expect(result.features() == std::vector<std::string>({"x", "y", "z"}),
         "union matching should preserve left feature order and append right-only features");
  expect(result.label().values().as_note().vector() ==
             std::vector<std::string_view>({"A", "C", "B"}),
         "union matching should preserve left case order and append right-only cases");

  const auto y = result.features("y");
  expect(y.as_series().to_vec<double>().front() == 210.0,
         "matched arithmetic should pair values by case labels");
  expect(pluss::table::detail::validity_mask(y)->to_vec<bool>() ==
             std::vector<bool>({true, false, false}),
         "matched arithmetic should mark one-sided values missing");
  expect(pluss::table::detail::validity_mask(result.features("x"))->to_vec<bool>() ==
             std::vector<bool>({false, false, false}),
         "left-only feature arithmetic should be entirely missing");
  expect(pluss::table::detail::validity_mask(result.features("z"))->to_vec<bool>() ==
             std::vector<bool>({false, false, false}),
         "right-only feature arithmetic should be entirely missing");
}

void test_explicit_common_matching() {
  const auto left = make_frame(
      {
          {"x", Slate(pluss::make_series(std::vector<double>{1.0, 2.0}))},
          {"y", Slate(pluss::make_series(std::vector<double>{10.0, 20.0}))},
      },
      {"A", "C"});
  const auto right = make_frame(
      {
          {"y", Slate(pluss::make_series(std::vector<double>{100.0, 200.0}))},
          {"z", Slate(pluss::make_series(std::vector<double>{7.0, 8.0}))},
      },
      {"B", "A"});

  FrameMatchOptions options;
  options.cases = FrameMatchRule::Common;
  options.features = FrameMatchRule::Common;
  const auto matched = left.match(right, options);

  expect(matched.left.features() == std::vector<std::string>({"y"}),
         "common feature matching should retain shared features only");
  expect(matched.left.label().values().as_note().vector() ==
             std::vector<std::string_view>({"A"}),
         "common case matching should retain shared cases only");
  expect(matched.left.features("y").as_series().to_vec<double>() ==
             std::vector<double>({10.0}),
         "common matching should gather the left value");
  expect(matched.right.features("y").as_series().to_vec<double>() ==
             std::vector<double>({200.0}),
         "common matching should gather the corresponding right value");
}

void test_repeated_caselbl_pair_in_order() {
  const auto left = make_frame(
      {{"x", Slate(pluss::make_series(std::vector<double>{1.0, 2.0}))}},
      {"A", "A"});
  const auto right = make_frame(
      {{"x", Slate(pluss::make_series(std::vector<double>{10.0}))}},
      {"A"});

  expect_throw(
      [&]() { (void)left.add(right); },
      "default frame arithmetic should reject mismatched positional shape for repeated cases");
  const auto result = left.add(right, {.align = pluss::table::AlignBy::Both});
  expect(result.ncases() == 2, "repeated cases should pair by occurrence without expansion");
  expect(result.features("x").as_series().to_vec<double>().front() == 11.0,
         "the first repeated occurrence should pair with the first peer occurrence");
  expect(pluss::table::detail::validity_mask(result.features("x"))->to_vec<bool>() ==
             std::vector<bool>({true, false}),
         "an unmatched repeated occurrence should be missing");

  FrameMatchOptions reject;
  reject.repeated_cases = RepeatedCaseRule::Reject;
  expect_throw(
      [&]() { (void)left.match(right, reject); },
      "reject mode should diagnose repeated cases");
}

void test_matching_rejects_incompatible_case_types() {
  const auto text_cases = make_frame(
      {{"x", Slate(pluss::make_series(std::vector<double>{1.0}))}},
      {"A"});
  const auto numeric_cases = DataFrame(
      {{"x", Slate(pluss::make_series(std::vector<double>{2.0}))}},
      Slate(pluss::make_series(std::vector<std::int64_t>{0})));

  expect_throw(
      [&]() { (void)text_cases.match(numeric_cases); },
      "matching should reject case axes with different payload kinds");
}

void test_case_aware_horizontal_composition() {
  const auto left = make_frame(
      {{"x", Slate(pluss::make_series(std::vector<double>{1.0, 2.0}))}},
      {"A", "C"});
  const auto right = make_frame(
      {{"y", Slate(pluss::make_series(std::vector<double>{10.0, 20.0}))}},
      {"B", "A"});

  const auto combined = left.combine(right);
  expect(combined.label().values().as_note().vector() ==
             std::vector<std::string_view>({"A", "C", "B"}),
         "horizontal composition should match cases by label");
  expect(pluss::table::detail::validity_mask(combined.features("x"))->to_vec<bool>() ==
             std::vector<bool>({true, true, false}),
         "horizontal composition should null-mark missing left rows");
  expect(pluss::table::detail::validity_mask(combined.features("y"))->to_vec<bool>() ==
             std::vector<bool>({true, false, true}),
         "horizontal composition should null-mark missing right rows");
  const auto y = combined.features("y").as_series().to_vec<double>();
  expect(y[0] == 20.0 && y[2] == 10.0,
         "horizontal composition should gather right values by case label");

  const auto updated = left.update(
      make_frame(
          {{"x", Slate(pluss::make_series(std::vector<double>{9.0}))}},
          {"A"}));
  expect(updated.label().values().as_note().vector() ==
             std::vector<std::string_view>({"A", "C"}),
         "update should keep the left case axis");
  expect(updated.features("x").as_series().to_vec<double>() ==
             std::vector<double>({9.0, 2.0}),
         "update should preserve left values where the right frame is missing");
}

void test_vertical_case_extension() {
  const auto left = make_frame(
      {
          {"x", Slate(pluss::make_series(std::vector<double>{1.0, 2.0}))},
          {"y", Slate(pluss::make_series(std::vector<double>{3.0, 4.0}))},
      },
      {"A", "C"});
  const auto right = make_frame(
      {
          {"y", Slate(pluss::make_series(std::vector<double>{5.0, 6.0}))},
          {"z", Slate(pluss::make_series(std::vector<double>{7.0, 8.0}))},
      },
      {"B", "A"});

  const auto extended = left.extend(right);
  expect(extended.features() ==
             std::vector<std::string>({"x", "y", "z"}),
         "case extension should use the stable feature union");
  expect(extended.label().values().as_note().vector() ==
             std::vector<std::string_view>({"A", "C", "B", "A"}),
         "case extension should append the full case axes");
  expect(pluss::table::detail::validity_mask(extended.features("x"))->to_vec<bool>() ==
             std::vector<bool>({true, true, false, false}),
         "case extension should null-fill a missing right feature");
  expect(pluss::table::detail::validity_mask(extended.features("z"))->to_vec<bool>() ==
             std::vector<bool>({false, false, true, true}),
         "case extension should null-fill a missing left feature");

  const auto common =
      left.extend(right, FrameMatchRule::Common, true);
  expect(common.features() == std::vector<std::string>({"y"}),
         "common case extension should retain shared features only");
  expect(common.label().values().as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1, 2, 3}),
         "fresh case extension should create one contiguous case axis");

  expect_throw(
      [&]() { (void)left.extend(right, FrameMatchRule::Unite, false, true); },
      "case extension should optionally reject repeated output cases");
}

DataFrame make_unify_left() {
  return DataFrame({
      {"id", Slate(pluss::make_series(std::vector<std::int64_t>{1, 2}))},
      {"left_value", Slate(pluss::make_series(std::vector<double>{10.0, 20.0}))},
  });
}

DataFrame make_unify_right() {
  return DataFrame({
      {"id", Slate(pluss::make_series(std::vector<std::int64_t>{2, 3}))},
      {"right_value", Slate(pluss::make_series(std::vector<double>{200.0, 300.0}))},
  });
}

void test_unify_modes() {
  const auto left = make_unify_left();
  const auto right = make_unify_right();

  const auto common = left.unify("id", right, "", "common");
  expect(common.ncases() == 1 &&
             common.features("id").as_series().to_vec<std::int64_t>(0) == 2,
         "common unification should retain matching keys");

  const auto united = left.unify("id", right, "", "unite");
  expect(united.ncases() == 3,
         "unite mode should retain matched and one-sided keys");
  expect(united.features("id").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({1, 2, 3}),
         "unite mode should coalesce a shared key feature");
  expect(pluss::table::detail::validity_mask(united.features("left_value"))->to_vec<bool>() ==
             std::vector<bool>({true, true, false}),
         "unite mode should null-mark missing left payloads");
  expect(pluss::table::detail::validity_mask(united.features("right_value"))->to_vec<bool>() ==
             std::vector<bool>({false, true, true}),
         "unite mode should null-mark missing right payloads");

  const auto keep_right = left.unify("id", right, "", "keep_right");
  expect(keep_right.ncases() == 2 &&
             keep_right.features("id").as_series().to_vec<std::int64_t>() ==
                 std::vector<std::int64_t>({2, 3}),
         "keep_right mode should preserve right-key order");

  const auto left_only = left.unify("id", right, "", "left_only");
  expect(left_only.ncases() == 1 &&
             left_only.features("id").as_series().to_vec<std::int64_t>(0) == 1,
         "left_only mode should retain unmatched left keys");

  const auto right_only = left.unify("id", right, "", "right_only");
  expect(right_only.ncases() == 1 &&
             right_only.features("id").as_series().to_vec<std::int64_t>(0) == 3,
         "right_only mode should retain unmatched right keys");

  const auto crossed = left.unify("", right, "", "cross");
  expect(crossed.ncases() == 4,
         "cross mode should create the cartesian case product");
}

void test_vectorized_group_reductions() {
  const auto frame = DataFrame({
      {"group", Slate(pluss::make_note({"A", "A", "B", "B", "B"}))},
      {"value", Slate(pluss::make_series(
          std::vector<double>{1.0, 3.0, 2.0, 4.0, 6.0}))},
      {"label", Slate(pluss::make_note({"x", "y", "z", "z", "w"}))},
  });
  const std::vector<DataFrame::GroupSpec> specs{
      {"n", "", DataFrame::GroupOp::Count},
      {"distinct", "value", DataFrame::GroupOp::Ndistinct},
      {"total", "value", DataFrame::GroupOp::Sum},
      {"average", "value", DataFrame::GroupOp::Mean},
      {"middle", "value", DataFrame::GroupOp::Median},
      {"num_mode", "value", DataFrame::GroupOp::Mode},
      {"label_mode", "label", DataFrame::GroupOp::Mode},
      {"smallest", "value", DataFrame::GroupOp::Min},
      {"largest", "value", DataFrame::GroupOp::Max},
      {"spread", "value", DataFrame::GroupOp::Std},
      {"variance", "value", DataFrame::GroupOp::Var},
  };

  const auto grouped = frame.group({.dropnull = true}).by({"group"}).agg(specs);
  expect(grouped.ncases() == 2,
         "grouping should produce one case per distinct key");
  expect(grouped.features("group").as_note().vector() ==
             std::vector<std::string_view>({"A", "B"}),
         "grouping should preserve first-seen key order and key dtype");
  expect(grouped.features("n").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({2, 3}),
         "group count should use vectorized group ids");
  expect(grouped.features("distinct").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({2, 3}),
         "group distinct count should count unique observed values");
  expect(grouped.features("total").as_series().to_vec<double>() ==
             std::vector<double>({4.0, 12.0}),
         "group sum should reduce by group id");
  expect(grouped.features("average").as_series().to_vec<double>() ==
             std::vector<double>({2.0, 4.0}),
         "group mean should reduce by group id");
  expect(grouped.features("middle").as_series().to_vec<double>() ==
             std::vector<double>({2.0, 4.0}),
         "group median should reduce by group id");
  expect(grouped.features("num_mode").as_series().to_vec<double>() ==
             std::vector<double>({1.0, 2.0}),
         "group numeric mode should tie-break by first-seen row");
  expect(grouped.features("label_mode").as_note().vector() ==
             std::vector<std::string_view>({"x", "z"}),
         "group note mode should reduce by group id");
  expect(grouped.features("smallest").as_series().to_vec<double>() ==
             std::vector<double>({1.0, 2.0}),
         "group minimum should reduce by group id");
  expect(grouped.features("largest").as_series().to_vec<double>() ==
             std::vector<double>({3.0, 6.0}),
         "group maximum should reduce by group id");
  const auto std_values = grouped.features("spread").as_series().to_vec<double>();
  const auto var_values = grouped.features("variance").as_series().to_vec<double>();
  expect(std::abs(std_values[0] - std::sqrt(2.0)) < 1e-12 &&
             std::abs(std_values[1] - 2.0) < 1e-12,
         "group standard deviation should use sample degrees of freedom");
  expect(std::abs(var_values[0] - 2.0) < 1e-12 &&
             std::abs(var_values[1] - 4.0) < 1e-12,
         "group variance should use sample degrees of freedom");

  const auto pivot_frame = DataFrame({
      {"case", Slate(pluss::make_note({"r1", "r1", "r2", "r2"}))},
      {"feature", Slate(pluss::make_note({"c1", "c1", "c1", "c1"}))},
      {"value", Slate(pluss::make_series(std::vector<double>{1.0, 5.0, 2.0, 8.0}))},
  });
  const auto pivoted = pivot_frame.pivot_table(
      "case", "feature", "value", DataFrame::GroupOp::Median, true, false);
  expect(pivoted.features("c1").as_series().to_vec<double>() ==
             std::vector<double>({3.0, 5.0}),
         "pivot_table median should aggregate duplicate cells");

  const auto sparse_counts = DataFrame({
      {"case", Slate(pluss::make_note({"r1", "r2"}))},
      {"feature", Slate(pluss::make_note({"c1", "c2"}))},
      {"value", Slate(pluss::make_series(std::vector<double>{10.0, 20.0}))},
  }).pivot_table("case", "feature", "value", DataFrame::GroupOp::Count, true, false);
  expect(pluss::table::detail::validity_mask(sparse_counts.features("c1"))->to_vec<bool>() ==
             std::vector<bool>({true, false}),
         "pivot_table count should leave unobserved cells missing instead of materializing zeros");
  expect(pluss::table::detail::validity_mask(sparse_counts.features("c2"))->to_vec<bool>() ==
             std::vector<bool>({false, true}),
         "pivot_table count should preserve sparsity per pivoted feature");

  const auto feature_keys = Slate(
      pluss::make_note(std::vector<std::string>{"NULL", "unused"}),
      pluss::make_series(std::vector<bool>{true, false}));
  const auto collision_frame = DataFrame({
      {"case", Slate(pluss::make_note({"r1", "r2"}))},
      {"feature", feature_keys},
      {"value", Slate(pluss::make_series(std::vector<double>{1.0, 2.0}))},
  });
  const auto collision_pivot =
      collision_frame.pivot_table("case", "feature", "value", DataFrame::GroupOp::Count, false, false);
  expect(collision_pivot.get("NULL").has_value() && collision_pivot.get("<NULL>").has_value(),
         "pivot_table should distinguish literal 'NULL' feature keys from missing feature keys");
  expect(collision_pivot.features("NULL").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({1, 0}) &&
             pluss::table::detail::validity_mask(collision_pivot.features("NULL"))->to_vec<bool>() ==
                 std::vector<bool>({true, false}),
         "pivot_table should keep the literal 'NULL' key in its own output feature");
  expect(collision_pivot.features("<NULL>").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1}) &&
             pluss::table::detail::validity_mask(collision_pivot.features("<NULL>"))->to_vec<bool>() ==
                 std::vector<bool>({false, true}),
         "pivot_table should render missing feature keys as '<NULL>' on the feature axis");

  const auto pivot_std =
      pivot_frame.pivot_table("case", "feature", "value", DataFrame::GroupOp::Std, true, false);
  const auto pivot_var =
      pivot_frame.pivot_table("case", "feature", "value", DataFrame::GroupOp::Var, true, false);
  const auto std_col = pivot_std.features("c1").as_series().to_vec<double>();
  const auto var_col = pivot_var.features("c1").as_series().to_vec<double>();
  expect(std::abs(std_col[0] - std::sqrt(8.0)) < 1e-12 &&
             std::abs(std_col[1] - std::sqrt(18.0)) < 1e-12,
         "pivot_table(std) should support sample standard deviation per cell");
  expect(std::abs(var_col[0] - 8.0) < 1e-12 &&
             std::abs(var_col[1] - 18.0) < 1e-12,
         "pivot_table(var) should support sample variance per cell");

  const auto mode_frame = DataFrame({
      {"case", Slate(pluss::make_note({"r1", "r1", "r2", "r2"}))},
      {"feature", Slate(pluss::make_note({"c1", "c1", "c1", "c1"}))},
      {"value", Slate(pluss::make_note({"x", "x", "z", "z"}))},
  });
  const auto pivot_mode =
      mode_frame.pivot_table("case", "feature", "value", DataFrame::GroupOp::Mode, true, false);
  expect(pivot_mode.features("c1").as_note().vector() ==
             std::vector<std::string_view>({"x", "z"}),
         "pivot_table(mode) should support note-valued grouped modes");

  DataFrame::PivotTableOptions multi_options;
  multi_options.index = {"city", "quarter"};
  multi_options.columns = {"product"};
  multi_options.values = {"units", "revenue"};
  multi_options.aggfuncs = {DataFrame::GroupOp::Sum};
  auto sales = DataFrame({
      {"city", Slate(pluss::make_note({"Accra", "Accra", "Accra", "Tema", "Tema"}))},
      {"quarter", Slate(pluss::make_note({"Q1", "Q1", "Q2", "Q1", "Q2"}))},
      {"product", Slate(pluss::make_note({"Laptop", "Phone", "Laptop", "Laptop", "Phone"}))},
      {"units", Slate(pluss::make_series(std::vector<double>{2.0, 3.0, 4.0, 1.0, 5.0}))},
      {"revenue", Slate(pluss::make_series(std::vector<double>{20.0, 30.0, 40.0, 10.0, 50.0}))},
  });
  const auto multi_pivot = sales.pivot_table(multi_options);
  expect(multi_pivot.label().names() == std::vector<std::string>({"city", "quarter"}),
         "pivot_table(options) should promote multi-key rows into a structured case axis");
  expect(multi_pivot.label().part("city").as_note().vector() ==
             std::vector<std::string_view>({"Accra", "Accra", "Tema", "Tema"}) &&
             multi_pivot.label().part("quarter").as_note().vector() ==
                 std::vector<std::string_view>({"Q1", "Q2", "Q1", "Q2"}),
         "pivot_table(options) should preserve lexicographic row-key ordering");
  expect(multi_pivot.features() ==
             std::vector<std::string>({
                 "units_sum :: Laptop",
                 "units_sum :: Phone",
                 "revenue_sum :: Laptop",
                 "revenue_sum :: Phone",
             }),
         "pivot_table(options) should flatten multi-value outputs deterministically");
  const auto units_laptop = multi_pivot.features("units_sum :: Laptop").as_series().to_vec<double>();
  expect(std::abs(units_laptop[0] - 2.0) < 1e-12 &&
             std::abs(units_laptop[1] - 4.0) < 1e-12 &&
             std::abs(units_laptop[2] - 1.0) < 1e-12 &&
             pluss::table::detail::validity_mask(multi_pivot.features("units_sum :: Laptop"))->to_vec<bool>() ==
                 std::vector<bool>({true, true, true, false}),
         "pivot_table(options) should populate the requested value-column aggregates per pivoted column");

  multi_options.fill_value = pluss::table::SlateValue::HostAny::floating(0.0);
  multi_options.margins = true;
  const auto pivot_with_totals = sales.pivot_table(multi_options);
  expect(pivot_with_totals.get("units_sum :: All").has_value() &&
             pivot_with_totals.get("revenue_sum :: All").has_value(),
         "pivot_table(options) should add total columns when margins=true");
  expect(pivot_with_totals.label().part("city").as_note().vector().back() == "All" &&
             pivot_with_totals.label().part("quarter").as_note().vector().back() == "",
         "pivot_table(options) should append a bottom total row on a structured case axis");
  expect(pivot_with_totals.features("units_sum :: Phone").as_series().to_vec<double>() ==
             std::vector<double>({3.0, 0.0, 0.0, 5.0, 8.0}),
         "pivot_table(fill_value, margins) should fill missing cells before exposing the result");
  expect(pivot_with_totals.features("units_sum :: All").as_series().to_vec<double>() ==
             std::vector<double>({5.0, 4.0, 1.0, 5.0, 15.0}),
         "pivot_table(margins=true) should add row totals plus a grand total");
}

void test_group_phase5_contracts() {
  const auto frame = DataFrame({
      {"group", Slate(pluss::make_note({"A", "A", "B", "B", "B"}))},
      {"value", Slate(pluss::make_series(std::vector<double>{1.0, 3.0, 2.0, 4.0, 6.0}))},
      {"label", Slate(pluss::make_note({"x", "y", "z", "z", "w"}))},
  });

  const auto counted = frame.group().by({"group"}).count();
  expect(counted.features() ==
             std::vector<std::string>({"group", "value_count", "label_count"}),
         "group().by(...).count() should count non-key value features with stable default names");
  expect(counted.features("value_count").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({2, 3}) &&
             counted.features("label_count").as_series().to_vec<std::int64_t>() ==
                 std::vector<std::int64_t>({2, 3}),
         "grouped count convenience should count non-null values per remaining feature");

  const auto rows = frame.features({"group"}).group().by({"group"}).count();
  expect(rows.features() == std::vector<std::string>({"group", "count"}),
         "grouped count should fall back to row count when no value features remain");
  expect(rows.features("count").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({2, 3}),
         "grouped row-count fallback should count cases per group");

  const auto targeted_count = frame.group().by({"group"}).count({"value"});
  expect(targeted_count.features() ==
             std::vector<std::string>({"group", "value_count"}),
         "targeted grouped count should only emit the requested value feature");
  expect(targeted_count.features("value_count").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({2, 3}),
         "targeted grouped count should preserve non-null count semantics");

  const auto targeted_mean = frame.group().by({"group"}).mean({"value"});
  expect(targeted_mean.features() ==
             std::vector<std::string>({"group", "value_mean"}),
         "targeted grouped mean should only emit the requested value feature");
  expect(targeted_mean.features("value_mean").as_series().to_vec<double>() ==
             std::vector<double>({2.0, 4.0}),
         "targeted grouped mean should aggregate only the requested feature");

  const auto targeted_multi = frame.group().by({"group"}).count({"value", "label"});
  expect(targeted_multi.features() ==
             std::vector<std::string>({"group", "value_count", "label_count"}),
         "targeted grouped reducers should preserve the requested feature order");

  const auto unnamed = frame.group().by({"group"}).agg({
      {"", "", DataFrame::GroupOp::Count},
      {"", "value", DataFrame::GroupOp::Mean},
      {"", "label", DataFrame::GroupOp::Count},
  });
  expect(unnamed.features() ==
             std::vector<std::string>({"group", "count", "value_mean", "label_count"}),
         "omitted grouped output names should resolve deterministically");
  expect(unnamed.features("count").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({2, 3}) &&
             unnamed.features("value_mean").as_series().to_vec<double>() ==
                 std::vector<double>({2.0, 4.0}),
         "omitted grouped output naming should preserve the requested aggregations");

  expect_throw(
      [&]() {
        (void)frame.group().by({"group"}).agg({
            {"", "value", DataFrame::GroupOp::Mean},
            {"", "value", DataFrame::GroupOp::Mean},
        });
      },
      "duplicate default grouped output names should fail loudly");
  expect_throw(
      [&]() {
        (void)frame.group().by({"group"}).agg({
            {"count", "", DataFrame::GroupOp::Count},
            {"", "", DataFrame::GroupOp::Count},
        });
      },
      "explicit and implicit grouped output names should not be allowed to collide");
}

void test_group_transform_and_filter() {
  const auto groups = Slate(
      pluss::make_note({"A", "A", "B", "drop", "B"}),
      pluss::make_series(std::vector<bool>{true, true, true, false, true}));
  const auto frame = DataFrame({
      {"group", groups},
      {"value", Slate(pluss::make_series(std::vector<double>{1.0, 3.0, 2.0, 9.0, 6.0}))},
      {"label", Slate(pluss::make_note({"x", "y", "z", "skip", "w"}))},
  });

  const auto value_mean = frame.features("value")
                              .group({.dropnull = true})
                              .by(frame.features("group"))
                              .transform(pluss::table::GroupOp::Mean);
  expect(value_mean.as_series().to_vec<double>() ==
             std::vector<double>({2.0, 2.0, 4.0, 2.0, 4.0}),
         "SlateGrouped::transform should broadcast grouped reductions back to row space");
  expect(pluss::table::detail::validity_mask(value_mean)->to_vec<bool>() ==
             std::vector<bool>({true, true, true, false, true}),
         "SlateGrouped::transform should null out rows excluded by dropnull keys");

  const auto multi_value =
      frame.features("value")
          .group({.dropnull = true})
          .by(frame.features("group"))
          .transform({
              {"value_mean", pluss::table::GroupOp::Mean},
              {"value_count", pluss::table::GroupOp::Count},
          });
  expect(multi_value.ncases() == frame.ncases() &&
             multi_value.features() == std::vector<std::string>({"value_mean", "value_count"}),
         "SlateGrouped::transform(specs) should return broadcasted output features only");
  expect(multi_value.features("value_mean").as_series().to_vec<double>() ==
             std::vector<double>({2.0, 2.0, 4.0, 2.0, 4.0}) &&
             multi_value.features("value_count").as_series().to_vec<std::int64_t>() ==
                 std::vector<std::int64_t>({2, 2, 2, 2, 2}),
         "SlateGrouped::transform(specs) should broadcast multiple grouped reductions");
  expect(pluss::table::detail::validity_mask(multi_value.features("value_count"))->to_vec<bool>() ==
             std::vector<bool>({true, true, true, false, true}),
         "SlateGrouped::transform(specs) should preserve dropped-key nulls");

  const auto transformed = frame.group({.dropnull = true}).by({"group"}).transform({
      {"value_mean", "value", DataFrame::GroupOp::Mean},
      {"count", "", DataFrame::GroupOp::Count},
      {"label_mode", "label", DataFrame::GroupOp::Mode},
  });
  expect(transformed.ncases() == frame.ncases() &&
             transformed.features() == std::vector<std::string>({"value_mean", "count", "label_mode"}),
         "DataFrameGrouped::transform should return broadcasted output features only");
  expect(transformed.features("value_mean").as_series().to_vec<double>() ==
             std::vector<double>({2.0, 2.0, 4.0, 2.0, 4.0}) &&
             transformed.features("count").as_series().to_vec<std::int64_t>() ==
                 std::vector<std::int64_t>({2, 2, 2, 2, 2}) &&
             transformed.features("label_mode").as_note().vector() ==
                 std::vector<std::string_view>({"x", "x", "z", "x", "z"}),
         "DataFrameGrouped::transform should broadcast explicit grouped specs");
  expect(pluss::table::detail::validity_mask(transformed.features("count"))->to_vec<bool>() ==
             std::vector<bool>({true, true, true, false, true}),
         "DataFrameGrouped::transform should preserve nulls for dropped key rows");

  const auto keep = frame.group({.dropnull = true})
                        .by({"group"})
                        .agg({{"total", "value", DataFrame::GroupOp::Sum}})
                        .features("total")
                        .gt(4.0);
  const auto filtered = frame.group({.dropnull = true}).by({"group"}).filter(keep);
  expect(filtered.ncases() == 2 &&
             filtered.features("group").as_note().vector() == std::vector<std::string_view>({"B", "B"}) &&
             filtered.features("value").as_series().to_vec<double>() == std::vector<double>({2.0, 6.0}),
         "DataFrameGrouped::filter should keep whole groups selected by a grouped keep mask");
}

void test_group_yield_and_ndistinct() {
  const auto groups = Slate(
      pluss::make_note({"B", "A", "B", "A", "skip", "B"}),
      pluss::make_series(std::vector<bool>{true, true, true, true, false, true}));
  const auto frame = DataFrame(
      {
          {"group", groups},
          {"month_code", Slate(
                             pluss::make_series(std::vector<double>{2.0, 1.0, 2.0, 3.0, 9.0, 0.0}),
                             pluss::make_series(std::vector<bool>{true, true, true, true, false, false}))},
          {"amount", Slate(pluss::make_series(std::vector<double>{20.0, 10.0, 30.0, 40.0, 50.0, 60.0}))},
      },
      Slate(pluss::make_note({"r0", "r1", "r2", "r3", "r4", "r5"})));

  const auto yielded = frame.group({.dropnull = true, .sort = true}).by({"group"}).yield();
  expect(yielded.ncases() == 5,
         "group().yield() should drop rows excluded by null grouping keys");
  expect(yielded.features("group").as_note().vector() ==
             std::vector<std::string_view>({"A", "A", "B", "B", "B"}),
         "group().yield() should materialize rows in grouped key order");
  expect(yielded.features("amount").as_series().to_vec<double>() ==
             std::vector<double>({10.0, 40.0, 20.0, 30.0, 60.0}),
         "group().yield() should preserve original within-group row order");
  expect(yielded.label().values().as_note().vector() ==
             std::vector<std::string_view>({"r1", "r3", "r0", "r2", "r5"}),
         "group().yield() should carry matching case labels through regrouping");

  const auto grouped = frame.group({.dropnull = true}).by({"group"}).agg({
      {"active_months", "month_code", DataFrame::GroupOp::Ndistinct},
  });
  expect(grouped.features("active_months").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({1, 2}),
         "grouped ndistinct should count unique non-null values per group");

  const auto indexed = DataFrame({
      {"region", Slate(pluss::make_note({"West", "West", "East", "East"}))},
      {"account", Slate(pluss::make_series(std::vector<std::int64_t>{101, 102, 201, 202}))},
      {"sales", Slate(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0}))},
  }).label().set(
      DataFrame::SetLabelOptions{
          .features = {"region", "account"},
          .drop = false,
          .verify = true,
          .sort = false});
  const auto label_yield = indexed.group().by(DataFrame::LabelGroup{.label = "region"}).yield();
  expect(!label_yield.get("__pluss_case_level_region").has_value(),
         "group(...).by(LabelGroup).yield() should not leak internal grouping features");
  expect(label_yield.features("sales").as_series().to_vec<double>() ==
             std::vector<double>({10.0, 20.0, 30.0, 40.0}),
         "label-group yield should regroup rows by case-label keys");
}

void test_contingency_and_split() {
  const auto cont_frame = DataFrame(
      {
          {"city", Slate(pluss::make_note({"A", "A", "B", "B", "B"}))},
          {"channel", Slate(pluss::make_note({"Cash", "Card", "Cash", "Cash", "Card"}))},
          {"weekend", Slate(pluss::make_note({"no", "no", "no", "yes", "yes"}))},
          {"amount", Slate(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0, 50.0}))},
      },
      Slate(pluss::make_note({"r0", "r1", "r2", "r3", "r4"})));

  const auto counts_ct = cont_frame.contingency("city", "channel");
  expect(counts_ct.ndim() == 2,
         "contingency(city, channel) should retain two dimensions");
  expect(counts_ct.dims() == std::vector<std::string>({"city", "channel"}),
         "contingency(city, channel) should preserve dimension names");
  expect(counts_ct.shape() == std::vector<std::int64_t>({2, 2}),
         "contingency(city, channel) should expose the dense tensor shape");

  const auto count_array = counts_ct.to_array().to(pluss::codem::Platform::CPU).contiguous();
  expect(std::vector<std::int64_t>(count_array.sizes().begin(), count_array.sizes().end()) ==
             std::vector<std::int64_t>({2, 2}),
         "to_array() should keep the contingency tensor shape");
  const auto* count_raw = reinterpret_cast<const std::int64_t*>(count_array.raw_data());
  std::vector<std::int64_t> count_values(count_raw, count_raw + 4);
  std::sort(count_values.begin(), count_values.end());
  expect(count_values == std::vector<std::int64_t>({1, 1, 1, 2}),
         "to_array() should retain the full cell counts");

  const auto counts = counts_ct.to_frame();
  expect(counts.features() ==
             std::vector<std::string>({"city", "Card", "Cash"}),
         "to_frame() should widen column levels into output features");
  expect(counts.features("city").as_note().vector() ==
             std::vector<std::string_view>({"A", "B"}),
         "to_frame() should preserve the sorted row levels by default");
  expect(counts.features("Card").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({1, 1}),
         "to_frame() should materialize the count cells");
  expect(counts.features("Cash").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({1, 2}),
         "to_frame() should materialize the count cells");

  DataFrame::ContingencyOptions totals_options;
  totals_options.totals = true;
  const auto totals = counts_ct.to_frame({"city"}, {"channel"}, totals_options);
  expect(totals.features() ==
             std::vector<std::string>({"city", "Card", "Cash", "Total"}),
         "to_frame(..., totals=true) should append a total output feature");
  expect(totals.features("city").as_note().vector() ==
             std::vector<std::string_view>({"A", "B", "Total"}),
         "to_frame(..., totals=true) should append a total row marker");
  expect(totals.features("Total").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({2, 3, 5}),
         "to_frame(..., totals=true) should report row and grand totals");

  const auto shares = counts_ct.share().to_frame();
  expect(std::abs(shares.features("Card").as_series().to_vec<double>()[0] - 0.2) < 1e-12,
         "share() should normalize by the full table total");
  expect(std::abs(shares.features("Cash").as_series().to_vec<double>()[1] - 0.4) < 1e-12,
         "share() should normalize by the full table total");

  const auto conditional = counts_ct.condition({"city"}).to_frame();
  const auto conditional_card = conditional.features("Card").as_series().to_vec<double>();
  expect(std::abs(conditional_card[0] - 0.5) < 1e-12 &&
             std::abs(conditional_card[1] - (1.0 / 3.0)) < 1e-12,
         "condition({city}) should normalize within each city slice");
  expect(std::abs(conditional.features("Cash").as_series().to_vec<double>()[1] - (2.0 / 3.0)) < 1e-12,
         "condition({city}) should normalize across the complementary dimensions");

  const auto multi_ct = cont_frame.contingency({"city", "channel", "weekend"});
  const auto multi = multi_ct.to_frame({"city"}, {"channel"});
  expect(multi.features() ==
             std::vector<std::string>({
                 "city",
                 "channel=Card | weekend=no",
                 "channel=Card | weekend=yes",
                 "channel=Cash | weekend=no",
                 "channel=Cash | weekend=yes",
             }),
         "to_frame(row, col) should flatten extra column dimensions deterministically");

  const auto weekend_yes = multi_ct.slice({{"weekend", "yes"}}).to_frame({"city"}, {"channel"});
  expect(weekend_yes.features("Card").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1}),
         "slice(...) should fix a level before rendering");
  expect(weekend_yes.features("Cash").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1}),
         "slice(...) should preserve the remaining dimensions");

  const auto marginalized = multi_ct.marginalize({"weekend"}).to_frame({"city"}, {"channel"});
  expect(marginalized.features("Card").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({1, 1}),
         "marginalize(...) should sum out the requested dimensions");
  expect(marginalized.features("Cash").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({1, 2}),
         "marginalize(...) should sum out the requested dimensions");

  const auto facets = multi_ct.facet({"city"}, {"channel"});
  expect(facets.depth() == 2,
         "facet(row, col) should return one frame per remaining level combination");
  expect(facets.axis0_labels().has_value(),
         "facet(row, col) should label the facet axis");
  expect(facets.axis0_labels()->as_note().vector() ==
             std::vector<std::string_view>({"weekend=no", "weekend=yes"}),
         "facet(row, col) should label each rendered slice");
  expect(facets.frame(0).features("Cash").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({1, 1}),
         "facet(row, col) should render the first remaining level slice");
  expect(facets.frame(1).features("Card").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1}),
         "facet(row, col) should render the second remaining level slice");

  const auto split_frame = DataFrame(
      {
          {"city", Slate(pluss::make_note({"A", "A", "B", "B", "C"}))},
          {"channel", Slate(pluss::make_note({"Cash", "Card", "Cash", "Card", "Cash"}))},
          {"amount", Slate(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0, 50.0}))},
          {"score", Slate(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}))},
      },
      Slate(pluss::make_note({"r0", "r1", "r2", "r3", "r4"})));

  const auto ordered_case = split_frame.split(0.4);
  expect(ordered_case.first.ncases() == 3 && ordered_case.second.ncases() == 2,
         "split(frac) should allocate frac of rows to the second output");
  expect(ordered_case.first.label().values().as_note().vector() ==
             std::vector<std::string_view>({"r0", "r1", "r2"}),
         "ordered row split should preserve the leading rows in the first output");
  expect(ordered_case.second.label().values().as_note().vector() ==
             std::vector<std::string_view>({"r3", "r4"}),
         "ordered row split should preserve the trailing rows in the second output");

  const auto ordered_feature = split_frame.split(0.5, Axis::Feature, false);
  expect(ordered_feature.first.features() ==
             std::vector<std::string>({"city", "channel"}),
         "ordered feature split should keep the leading features in the first output");
  expect(ordered_feature.second.features() ==
             std::vector<std::string>({"amount", "score"}),
         "ordered feature split should keep the trailing features in the second output");

  const auto random_case = split_frame.split(0.4, 7);
  expect(random_case.first.ncases() == 3 && random_case.second.ncases() == 2,
         "randomized row split should preserve the requested row counts");
  std::vector<std::string_view> combined_labels = random_case.first.label().values().as_note().vector();
  const auto rhs_labels = random_case.second.label().values().as_note().vector();
  combined_labels.insert(combined_labels.end(), rhs_labels.begin(), rhs_labels.end());
  std::sort(combined_labels.begin(), combined_labels.end());
  expect(combined_labels ==
             std::vector<std::string_view>({"r0", "r1", "r2", "r3", "r4"}),
         "randomized row split should partition the original rows without loss");

  const auto nan_render = DataFrame({
      {"value", Slate(pluss::make_series(std::vector<double>{
          1.0,
          std::numeric_limits<double>::quiet_NaN(),
      }))},
  }).to_string();
  expect(nan_render.find("nan") == std::string::npos &&
             nan_render.find("NaN") == std::string::npos,
         "display rendering should not print literal nan for floating values");
  expect(nan_render.find("NULL") != std::string::npos,
         "display rendering should present floating NaN values as NULL");
}

void test_temporal_subtraction_and_sort_builder() {
  const auto left = Slate(pluss::make_timeline({
      "2024-01-03T00:00:00Z",
      "2024-01-05T00:00:00Z",
  }));
  const auto right = Slate(pluss::make_timeline({
      "2024-01-01T00:00:00Z",
      "2024-01-02T00:00:00Z",
  }));
  expect((left - right).as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({172800000000000LL, 259200000000000LL}),
         "Timeline - Timeline should delegate to the span kernel in nanosecond mode");
  expect((left - right).as_series().to_vec<std::int64_t>() ==
             right.tl().span(left, pluss::table::TimeUnit::Nanos).as_series().to_vec<std::int64_t>(),
         "Timeline - Timeline should preserve subtraction direction while matching tl().span(..., Nanos)");

  const auto idx = Slate(pluss::make_timeline({"2024-01-01", "2024-01-04"}));
  expect((left - idx).as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({172800000000000LL, 86400000000000LL}),
         "Timeline subtraction should produce nanosecond deltas for indexed axes");
  expect((left - idx).as_series().to_vec<std::int64_t>() ==
             idx.tl().span(left, pluss::table::TimeUnit::Nanos).as_series().to_vec<std::int64_t>(),
         "indexed Timeline subtraction should match tl().span(..., Nanos) with direction preserved");

  const auto values = Slate(pluss::make_series(std::vector<double>{3.0, 1.0, 2.0}));
  const Slate sorted_slate = values.sort();
  expect(sorted_slate.as_series().to_vec<double>() == std::vector<double>({1.0, 2.0, 3.0}),
         "Slate::sort() should remain usable through implicit conversion");

  const auto frame = DataFrame(
      {
          {"a", Slate(pluss::make_series(std::vector<std::int64_t>{2, 1, 2, 1}))},
          {"b", Slate(pluss::make_series(std::vector<std::int64_t>{2, 2, 1, 1}))},
          {"v", Slate(pluss::make_series(std::vector<double>{20.0, 10.0, 30.0, 40.0}))},
      },
      Slate(pluss::make_note({"r2", "r1", "r3", "r0"})));

  const auto by = frame.sort().by({"a", "b"}).yield();
  expect(by.features("v").as_series().to_vec<double>() ==
             std::vector<double>({40.0, 10.0, 30.0, 20.0}),
         "DataFrame::sort().by(features) should reuse arrange semantics");

  const auto chained = frame.sort().by("a").desc().then("b").asc().yield();
  expect(chained.features("v").as_series().to_vec<double>() ==
             std::vector<double>({30.0, 20.0, 40.0, 10.0}),
         "DataFrame::sort() should support per-key chained sort policies");

  const auto order = frame.sort().by("v").desc().order();
  expect(order.as_series().to_vec<std::int64_t>() == std::vector<std::int64_t>({3, 2, 0, 1}),
         "DataFrame::sort().order() should expose the reusable row permutation");
  const auto reordered = frame.take(order);
  expect(reordered.features("v").as_series().to_vec<double>() ==
             std::vector<double>({40.0, 30.0, 20.0, 10.0}),
         "DataFrame::take(order) should reuse a sort permutation directly");
  const auto top2 = frame.sort().by("v").desc().head(2);
  expect(top2.features("v").as_series().to_vec<double>() == std::vector<double>({40.0, 30.0}),
         "DataFrame::sort().head(n) should return the ranked prefix");
  const auto tail2 = frame.sort().by("v").desc().tail(2);
  expect(tail2.features("v").as_series().to_vec<double>() == std::vector<double>({20.0, 10.0}),
         "DataFrame::sort().tail(n) should return the ranked suffix");

  const auto grouped = DataFrame(
      {
          {"g", Slate(pluss::make_note({"B", "A", "B", "A", "C"}))},
          {"score", Slate(pluss::make_series(std::vector<std::int64_t>{3, 1, 2, 4, 0}))},
          {"v2", Slate(pluss::make_series(std::vector<std::int64_t>{30, 10, 20, 40, 0}))},
      },
      Slate(pluss::make_note({"b0", "a0", "b1", "a1", "c0"})));
  const auto within = grouped.sort().within("g").by("score").desc().yield();
  expect(within.features("g").as_note().vector() ==
             std::vector<std::string_view>({"B", "B", "A", "A", "C"}),
         "DataFrame::sort().within(keys) should keep first-seen group order");
  expect(within.features("score").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({3, 2, 4, 1, 0}),
         "DataFrame::sort().within(keys) should sort inside each group");
  const auto within_head3 = grouped.sort().within("g").by("score").desc().head(3);
  expect(within_head3.features("score").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({3, 2, 4}),
         "DataFrame::sort().within(keys).head(n) should rank across the grouped view without full materialization");
}

void test_bundelize_and_tabelize_interop() {
  const auto orders = make_frame(
      {
          {"id", Slate(pluss::make_series(std::vector<std::int64_t>{1, 2, 3}))},
          {"city", Slate(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
          {"sales", Slate(pluss::make_series(std::vector<double>{100.0, 150.0, 90.0}))},
      },
      {"o1", "o2", "o3"});

  const auto catalog = orders.kindelize();
  const auto rebuilt = catalog.tabelize();
  expect(rebuilt.features() == orders.features(),
         "Catalog::tabelize() should rebuild feature names from a kindelized frame");
  expect(rebuilt.label().values().as_note().vector() == orders.label().values().as_note().vector(),
         "Catalog::tabelize() should preserve case labels on a round-trip");

  const auto registry = orders.scribbelize();
  const auto from_registry = registry.tabelize();
  expect(from_registry.features() == orders.features(),
         "Registry::tabelize() should rebuild feature names from a bundelized frame");
  expect(from_registry.features("sales").as_series().to_vec<double>() ==
             std::vector<double>({100.0, 150.0, 90.0}),
         "Registry::tabelize() should preserve numeric values");

  const auto from_catalog_scribble = catalog.scribbelize();
  expect(from_catalog_scribble.fields() == std::vector<std::string>({"id", "city", "sales"}),
         "Catalog::scribbelize() should preserve key names");

  const auto atlas = Atlas({orders, orders.title("copy")});
  const auto archive = atlas.kindelize();
  const auto axel = atlas.scribbelize();
  const auto atlas_from_archive = archive.tabelize();
  const auto atlas_from_axel = axel.tabelize();
  expect(atlas_from_archive.depth() == 2 && atlas_from_axel.depth() == 2,
         "Archive::tabelize() and Axel::tabelize() should preserve depth");
}

void test_group_phase7_hardening() {
  const auto key_validity = pluss::make_series(
      std::vector<bool>{true, true, true, false, true, true});
  const auto value_validity = pluss::make_series(
      std::vector<bool>{true, true, true, true, false, true});
  const auto frame = DataFrame({
      {"k1", Slate(
                 pluss::make_note({"b", "a", "b", "drop", "a", "b"}),
                 key_validity)},
      {"k2", Slate(pluss::make_series(std::vector<std::int64_t>{2, 2, 1, 9, 1, 2}))},
      {"value", Slate(
                    pluss::make_series(std::vector<double>{5.0, 1.0, 3.0, 7.0, 9.0, 11.0}),
                    value_validity)},
      {"label", Slate(pluss::make_note({"u", "v", "u", "w", "x", "u"}))},
  });

  const auto first_seen = frame.group({.dropnull = true, .sort = false}).by({"k1", "k2"});
  const auto stable = first_seen.agg({
      {"n", "", DataFrame::GroupOp::Count},
      {"avg", "value", DataFrame::GroupOp::Mean},
  });
  expect(stable.features("k1").as_note().vector() ==
             std::vector<std::string_view>({"b", "a", "b", "a"}) &&
             stable.features("k2").as_series().to_vec<std::int64_t>() ==
                 std::vector<std::int64_t>({2, 2, 1, 1}),
         "sort=false should preserve first-seen multi-key order");
  expect(stable.features("n").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({2, 1, 1, 1}),
         "row-count grouping should ignore dropped-key rows while retaining valid keyed rows");
  expect(pluss::table::detail::validity_mask(stable.features("avg"))->to_vec<bool>() ==
             std::vector<bool>({true, true, true, false}),
         "numeric grouped results should null groups with no observed values");

  const auto sorted = frame.group({.dropnull = true, .sort = true}).by({"k1", "k2"}).count();
  expect(sorted.features("k1").as_note().vector() ==
             std::vector<std::string_view>({"a", "a", "b", "b"}) &&
             sorted.features("k2").as_series().to_vec<std::int64_t>() ==
                 std::vector<std::int64_t>({1, 2, 1, 2}),
         "sort=true should produce deterministic sorted multi-key order");

  const auto keep = first_seen.count().features("value_count").gt(1.0);
  const auto filtered = first_seen.filter(keep);
  expect(filtered.ncases() == 2 &&
             filtered.features("k1").as_note().vector() ==
                 std::vector<std::string_view>({"b", "b"}),
         "reusing the same grouped handle for count then filter should retain cached grouping semantics");

  const auto all_null = DataFrame({
      {"group", Slate(
                    pluss::make_note({"A", "B", "C"}),
                    pluss::make_series(std::vector<bool>{false, false, false}))},
      {"value", Slate(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}))},
  }).group({.dropnull = true}).by({"group"});
  const auto empty = all_null.agg({
      {"n", "", DataFrame::GroupOp::Count},
      {"total", "value", DataFrame::GroupOp::Sum},
  });
  expect(empty.ncases() == 0,
         "all-null keys with dropnull=true should produce zero output groups");
  const auto empty_tx = all_null.transform({
      {"n", "", DataFrame::GroupOp::Count},
      {"total", "value", DataFrame::GroupOp::Sum},
  });
  expect(pluss::table::detail::validity_mask(empty_tx.features("n"))->to_vec<bool>() ==
             std::vector<bool>({false, false, false}) &&
             pluss::table::detail::validity_mask(empty_tx.features("total"))->to_vec<bool>() ==
                 std::vector<bool>({false, false, false}),
         "all-null keys should broadcast all-invalid grouped transform results");
  const auto none = all_null.filter(Slate(pluss::Series::empty({0}, pluss::codem::ScalarType::Bool)));
  expect(none.ncases() == 0,
         "all-null keys should allow a zero-length keep mask and return no rows");

  std::vector<std::string> ids;
  ids.reserve(128);
  std::vector<double> values;
  values.reserve(128);
  for (int i = 0; i < 128; ++i) {
    ids.push_back("g" + std::to_string(i));
    values.push_back(static_cast<double>(i));
  }
  const auto high_cardinality = DataFrame({
      {"group", Slate(pluss::make_note(ids))},
      {"value", Slate(pluss::make_series(values))},
  }).group({.dropnull = true, .sort = false}).by({"group"}).count();
  expect(high_cardinality.ncases() == 128 &&
             high_cardinality.features("value_count").as_series().to_vec<std::int64_t>()[127] == 1,
         "high-cardinality grouping should preserve one output row per distinct key");
}

void test_lengthen_reshape() {
  const auto frame = DataFrame(
      {
          {"id", Slate(pluss::make_note({"r1", "r2"}))},
          {"a", Slate(pluss::make_series(std::vector<double>{1.0, 2.0}))},
          {"b", Slate(pluss::make_series(std::vector<double>{3.0, 4.0}))},
      },
      Slate(pluss::make_note({"C1", "C2"})));

  const auto long_form = frame.lengthen({"id"}, {"a", "b"});
  expect(long_form.features() ==
             std::vector<std::string>({"id", "feature", "value"}),
         "lengthen should emit identifiers, source feature, and value");
  expect(long_form.features("id").as_note().vector() ==
             std::vector<std::string_view>({"r1", "r2", "r1", "r2"}),
         "lengthen should tile identifier features by value feature");
  expect(long_form.features("feature").as_note().vector() ==
             std::vector<std::string_view>({"a", "a", "b", "b"}),
         "lengthen should identify each source value feature");
  expect(long_form.features("value").as_series().to_vec<double>() ==
             std::vector<double>({1.0, 2.0, 3.0, 4.0}),
         "lengthen should concatenate homogeneous value payloads");
  expect(long_form.label().values().as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1, 2, 3}),
         "lengthen should create fresh contiguous cases by default");

  const auto preserved =
      frame.lengthen({"id"}, {"a", "b"}, "source", "measure", false);
  expect(preserved.label().values().as_note().vector() ==
             std::vector<std::string_view>({"C1", "C2", "C1", "C2"}),
         "lengthen should tile original cases when requested");

  expect_throw(
      [&]() { (void)frame.lengthen({}, {"id", "a"}); },
      "lengthen should reject heterogeneous value payload kinds");
}

void test_hierarchical_case_axis_engine() {
  const auto frame = DataFrame({
      {"region", Slate(pluss::make_note({"West", "West", "East", "East"}))},
      {"account", Slate(pluss::make_series(std::vector<std::int64_t>{102, 101, 201, 202}))},
      {"quarter", Slate(pluss::make_note({"Q2", "Q1", "Q1", "Q2"}))},
      {"sales", Slate(pluss::make_series(std::vector<double>{20.0, 10.0, 30.0, 40.0}))},
      {"cost", Slate(pluss::make_series(std::vector<double>{8.0, 4.0, 12.0, 16.0}))},
  });

  const auto indexed = frame.label().set(
      DataFrame::SetLabelOptions{
          .features = {"region", "account"},
          .drop = false,
          .verify = true,
          .sort = false});
  expect(indexed.label().has(), "label().set should attach structured case labels");
  expect(indexed.label().names() == std::vector<std::string>({"region", "account"}),
         "label().set should preserve case-label names");
  expect(indexed.label().part("region").as_note().vector()[0] == "West",
         "label().part(name) should expose a typed case label");

  const auto selected = indexed.case_study().pick(
      DataFrame::CasePick{.label = "region", .value = "West"});
  expect(selected.ncases() == 2 &&
             selected.label().part("account").as_series().to_vec<std::int64_t>() ==
                 std::vector<std::int64_t>({102, 101}),
         "case_study(level,value) should perform partial hierarchical selection");

  const auto full_key = indexed.case_study().pick(
      DataFrame::CasePick{.key = {"East", "202"}});
  expect(full_key.ncases() == 1 &&
             full_key.features("sales").as_series().to_vec<double>() ==
                 std::vector<double>({40.0}),
         "case_study(key) should match a full structured key");

  const auto projected = indexed.features({"sales", "cost"});
  expect(projected.label().has() && projected.label().names() == indexed.label().names(),
         "feature selection should preserve hierarchical case metadata");

  const auto sorted = indexed.label().sort(
      DataFrame::SortLabelOptions{.names = {"region", "account"}});
  expect(sorted.label().part("region").as_note().vector() ==
             std::vector<std::string_view>({"East", "East", "West", "West"}) &&
             sorted.label().part("account").as_series().to_vec<std::int64_t>() ==
                 std::vector<std::int64_t>({201, 202, 101, 102}),
         "label().sort(names) should sort by typed case labels");

  const std::vector<DataFrame::GroupSpec> specs{
      {"total", "sales", DataFrame::GroupOp::Sum},
      {"n", "", DataFrame::GroupOp::Count},
  };
  const auto grouped =
      indexed.group().by(DataFrame::LabelGroup{.label = "region"}).agg(specs);
  expect(grouped.features("region").as_note().vector() ==
             std::vector<std::string_view>({"West", "East"}) &&
             grouped.features("total").as_series().to_vec<double>() ==
                 std::vector<double>({30.0, 70.0}),
         "group(...).by(LabelGroup).agg(...) should group by case levels without requiring key features");

  const auto transformed =
      indexed.group().by(DataFrame::LabelGroup{.label = "region"}).transform({
          {"region_total", "sales", DataFrame::GroupOp::Sum},
      });
  expect(transformed.features("region_total").as_series().to_vec<double>() ==
             std::vector<double>({30.0, 30.0, 70.0, 70.0}),
         "group(...).by(LabelGroup).transform(...) should broadcast case-level grouped values");

  const auto keep_east = grouped.features("total").gt(50.0);
  const auto east_only =
      indexed.group().by(DataFrame::LabelGroup{.label = "region"}).filter(keep_east);
  expect(east_only.ncases() == 2 &&
             east_only.features("sales").as_series().to_vec<double>() == std::vector<double>({30.0, 40.0}),
         "group(...).by(LabelGroup).filter(...) should keep full label groups");

  const auto renamed = indexed.label().rename({{"account", "acct"}});
  expect(renamed.label().names() == std::vector<std::string>({"region", "acct"}),
         "label().rename should rename a structured case label");
  const auto reordered = renamed.label().order({"acct", "region"});
  expect(reordered.label().names() == std::vector<std::string>({"acct", "region"}),
         "label().order should reorder structured case labels");
  const auto dropped = reordered.label().drop({"region"});
  expect(dropped.label().names() == std::vector<std::string>({"acct"}),
         "label().drop should remove selected structured case labels");
  const auto reset = indexed.label().reset(
      DataFrame::ResetLabelOptions{.drop = false, .names = {"region_case", "account_case"}});
  expect(reset.get("region_case").has_value() && reset.get("account_case").has_value() &&
             !reset.label().has(),
         "label().reset should materialize case labels as ordinary features and clear hierarchy");

  const auto collision_left = DataFrame({
      {"region", Slate(pluss::make_note({"A"}))},
      {"account", Slate(pluss::make_note(std::vector<std::string>{std::string("B") + '\x1F' + "C"}))},
      {"value", Slate(pluss::make_series(std::vector<double>{10.0}))},
  }).label().set({"region", "account"}, false, "\x1F")
    .features({"value"});
  const auto collision_right = DataFrame({
      {"region", Slate(pluss::make_note(std::vector<std::string>{std::string("A") + '\x1F' + "B"}))},
      {"account", Slate(pluss::make_note({"C"}))},
      {"value", Slate(pluss::make_series(std::vector<double>{1.0}))},
  }).label().set({"region", "account"}, false, "\x1F")
    .features({"value"});
  const auto collision = collision_left.add(
      collision_right,
      pluss::table::AlignOptions{.align = pluss::table::AlignBy::Both});
  expect(collision.ncases() == 2 &&
             !pluss::table::detail::valid_at(collision.features("value"), 0) &&
             !pluss::table::detail::valid_at(collision.features("value"), 1),
         "structured case alignment should use typed level tuples, not flattened display labels");
}

void test_stack_unstack_hierarchical_cases() {
  const auto frame = DataFrame({
      {"region", Slate(pluss::make_note({"West", "West", "East", "East"}))},
      {"account", Slate(pluss::make_series(std::vector<std::int64_t>{101, 101, 201, 201}))},
      {"quarter", Slate(pluss::make_note({"Q1", "Q2", "Q1", "Q2"}))},
      {"sales", Slate(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0}))},
  }).label().set({"region", "account", "quarter"}, true);

  const auto wide = frame.unstack("quarter");
  expect(wide.label().has() &&
             wide.label().names() == std::vector<std::string>({"region", "account"}) &&
             wide.features() == std::vector<std::string>({"Q1", "Q2"}) &&
             wide.features("Q1").as_series().to_vec<double>() ==
                 std::vector<double>({10.0, 30.0}) &&
             wide.features("Q2").as_series().to_vec<double>() ==
                 std::vector<double>({20.0, 40.0}),
         "unstack should move one case level into wide features");

  const auto long_again = wide.stack("quarter", "sales");
  expect(long_again.label().has() &&
             long_again.label().names() == std::vector<std::string>({"region", "account", "quarter"}) &&
             long_again.features("sales").as_series().to_vec<double>() ==
                 std::vector<double>({10.0, 30.0, 20.0, 40.0}),
         "stack should move features into the case hierarchy and keep typed values");

  const auto duplicate = DataFrame({
      {"region", Slate(pluss::make_note({"West", "West"}))},
      {"quarter", Slate(pluss::make_note({"Q1", "Q1"}))},
      {"sales", Slate(pluss::make_series(std::vector<double>{1.0, 2.0}))},
  }).label().set({"region", "quarter"}, true);
  expect_throw([&]() { (void)duplicate.unstack("quarter"); },
               "unstack should reject duplicate output cells");
}

void test_transform_axis_surface() {
  const auto frame = DataFrame({
      {"x", Slate(pluss::make_series(std::vector<double>{1.0, 2.0}))},
      {"y", Slate(pluss::make_series(std::vector<double>{3.0, 4.0}))},
  });
  const auto by_feature = frame.transform(
      [](const Slate& value) {
        return value + 10.0;
      },
      pluss::table::Axis::Feature);
  expect(by_feature.features("x").as_series().to_vec<double>() ==
             std::vector<double>({11.0, 12.0}) &&
             by_feature.features("y").as_series().to_vec<double>() ==
                 std::vector<double>({13.0, 14.0}),
         "transform(slate_fn, Axis::Feature) should apply column-wise");

  const auto slate_transformed = frame.features("x").transform(
      [](const pluss::table::SlateValue& value) {
        return pluss::table::SlateValue(value.slate() + 5.0);
      });
  expect(slate_transformed.as_series().to_vec<double>() ==
             std::vector<double>({6.0, 7.0}),
         "Slate::transform(value_fn) should apply element-wise");

  const auto whole_slate_default = frame.features("x").transform(
      [](const Slate& value) {
        return value + 20.0;
      });
  expect(whole_slate_default.as_series().to_vec<double>() ==
             std::vector<double>({21.0, 22.0}),
         "Slate::transform(slate_fn) should preserve length by default");

  const auto whole_slate_reduce = frame.features("x").transform(
      [](const Slate& value) {
        return Slate(pluss::make_series({value.sum().to_dbl()}));
      },
      pluss::table::ResultShape::Reduce);
  expect(whole_slate_reduce.as_series().to_vec<double>() ==
             std::vector<double>({3.0}),
         "Slate::transform(slate_fn, Reduce) should accept one-row output");

  expect_throw(
      [&]() {
        (void)frame.features("x").transform([](const Slate& value) {
          return Slate(pluss::make_series({value.sum().to_dbl()}));
        });
      },
      "Slate::transform(slate_fn) should reject reduction output under the Preserve default");
}

void test_category_factor() {
  const auto validity = pluss::make_series(
      std::vector<bool>{true, true, true, true, false});
  const auto frame = DataFrame({
      {"city", Slate(
          pluss::make_note({"Calgary", "Edmonton", "Calgary", "Banff", ""}),
          validity)},
  });

  const auto encoding = frame.factor("city");
  expect(encoding.levels.as_note().vector() ==
             std::vector<std::string_view>({"Calgary", "Edmonton", "Banff"}),
         "factor should preserve stable first-seen level order");
  expect(encoding.codes.as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1, 0, 2, -1}),
         "factor should encode levels and reserve -1 for missing values");
  expect(pluss::table::detail::validity_mask(encoding.codes)->to_vec<bool>() ==
             std::vector<bool>({true, true, true, true, false}),
         "factor should preserve source missingness");
  const auto decoded = encoding.decode();
  expect(decoded.as_note().vector()[0] == "Calgary" &&
             decoded.as_note().vector()[1] == "Edmonton" &&
             decoded.as_note().vector()[3] == "Banff",
         "category decoding should recover observed labels");
  expect(!pluss::table::detail::validity_mask(decoded)->to_vec<bool>()[4],
         "category decoding should recover missingness");

  const auto sorted = frame.factor("city", true, true);
  expect(sorted.ordered &&
             sorted.levels.as_note().vector() ==
                 std::vector<std::string_view>({"Banff", "Calgary", "Edmonton"}),
         "factor should support explicit ordered sorted levels");
}

void test_streamed_csv_case_batches() {
  const auto path =
      std::filesystem::temp_directory_path() /
      "plussmic_case_reader_test.csv";
  {
    std::ofstream output(path);
    output << "id,value\n"
           << "1,10\n"
           << "2,20\n"
           << "3,30\n"
           << "4,40\n"
           << "5,50\n";
  }

  pluss::loader::ReadCsvOptions options;
  options.ncases = 4;
  pluss::loader::CsvCaseReader reader(path.string(), 2, options);
  DataFrame batch;
  expect(reader.next(batch) && batch.ncases() == 2,
         "streamed CSV reader should return the first bounded case batch");
  expect(batch.features("value").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({10, 20}),
         "streamed CSV reader should parse the first batch values");
  expect(reader.next(batch) && batch.ncases() == 2,
         "streamed CSV reader should return the next bounded case batch");
  expect(batch.features("value").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({30, 40}),
         "streamed CSV reader should continue from its current file position");
  expect(!reader.next(batch) && reader.cases_read() == 4,
         "streamed CSV reader should honor the global case limit");
  std::filesystem::remove(path);
}

void test_case_aligned_set_and_reduction_axis_rules() {
  auto df = DataFrame(
      {
          {"student", Slate(pluss::make_note({"Alice", "Bob", "Carol", "David"}))},
          {"A1", Slate(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0}))},
          {"A2", Slate(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}))},
      },
      Slate(pluss::make_series(std::vector<std::int64_t>{1001, 1002, 1003, 1004})));

  expect(df.shape() == std::pair<std::int64_t, std::int64_t>{4, 3},
         "DataFrame::shape should report cases x features");
  expect(df.features("A1").ncases() == 4 &&
             df.features("A1").shape() == std::pair<std::int64_t, std::int64_t>{4, 1},
         "Slate::ncases/shape should report a 1D case vector");

  const auto row_totals = df.features({"A1", "A2"}).sum(Axis::Feature);
  expect(row_totals.as_series().to_vec<double>() ==
             std::vector<double>({11.0, 22.0, 33.0, 44.0}),
         "Axis::Feature reduction should reduce across features row-wise");
  df.set("Assignments") = row_totals;
  expect(df.features("Assignments").as_series().to_vec<double>() ==
             std::vector<double>({11.0, 22.0, 33.0, 44.0}),
         "case-length row-wise reduction should assign cleanly");

  const auto column_totals = df.features({"A1", "A2"}).sum();
  expect(column_totals.as_series().to_vec<double>() ==
             std::vector<double>({100.0, 10.0}),
         "default DataFrame reduction should reduce down cases column-wise");
  bool wrong_axis_default_threw = false;
  try {
    df.set("WrongAxis") = column_totals;
  } catch (const std::exception&) {
    wrong_axis_default_threw = true;
  }
  expect(wrong_axis_default_threw,
         "default assignment should reject feature-indexed reduction length mismatch");
  df.set("WrongAxis", {.align = pluss::table::AlignBy::Caselbl}) = column_totals;
  expect(pluss::table::detail::validity_mask(df.features("WrongAxis"))->to_vec<bool>() ==
             std::vector<bool>({false, false, false, false}),
         "explicit case-label assignment should make feature-indexed reduction null when labels do not match");

  const auto scholarship = Slate(pluss::make_series(
                               std::vector<double>{1500.0, 2500.0, 1000.0}))
                               .label().set(Slate(pluss::make_series(
                                   std::vector<std::int64_t>{1003, 1001, 1004})));
  bool scholarship_default_threw = false;
  try {
    df.set("scholarship") = scholarship;
  } catch (const std::exception&) {
    scholarship_default_threw = true;
  }
  expect(scholarship_default_threw,
         "default assignment should reject partial labeled rhs");
  df.set("scholarship", {.align = pluss::table::AlignBy::Caselbl}) = scholarship;
  const auto scholarship_values = df.features("scholarship").as_series().to_vec<double>();
  expect(scholarship_values[0] == 2500.0 &&
             scholarship_values[2] == 1500.0 &&
             scholarship_values[3] == 1000.0,
         "case-aligned assignment should gather matching rhs values");
  expect(pluss::table::detail::validity_mask(df.features("scholarship"))->to_vec<bool>() ==
             std::vector<bool>({true, false, true, true}),
         "case-aligned assignment should mark missing target cases null");

  df.set("flat") = pluss::make_series(7235.0);
  expect(df.features("flat").as_series().to_vec<double>() ==
             std::vector<double>({7235.0, 7235.0, 7235.0, 7235.0}),
         "scalar Series assignment should broadcast over established cases");

  bool one_row_default_threw = false;
  try {
    df.set("one_row_series") = Slate(pluss::make_series(std::vector<double>{7235.0}));
  } catch (const std::exception&) {
    one_row_default_threw = true;
  }
  expect(one_row_default_threw,
         "plain 1D length-one Slate assignment should not broadcast or align by default");
}

void test_align_reindex_equivalent() {
  const auto frame = make_frame(
      {
          {"A", Slate(pluss::make_series(std::vector<double>{10.0, 20.0}))},
          {"B", Slate(pluss::make_series(std::vector<double>{100.0, 200.0}))},
      },
      {"r1", "r2"});

  const auto cases = frame.label().align(Slate(pluss::make_note({"r2", "missing", "r1"})));
  expect(cases.label().values().as_note().vector() ==
             std::vector<std::string_view>({"r2", "missing", "r1"}),
         "align_caselbl should conform the case-label axis exactly");
  const auto case_values = cases.features("A").as_series().to_vec<double>();
  expect(case_values[0] == 20.0 && case_values[2] == 10.0,
         "align_caselbl should gather existing rows by case label");
  expect(pluss::table::detail::validity_mask(cases.features("A"))->to_vec<bool>() ==
             std::vector<bool>({true, false, true}),
         "align_caselbl should mark missing case labels null");

  const auto both = frame.align(DataFrame::AlignSpec{
      .label = Slate(pluss::make_note({"r2", "missing"})),
      .features = {"B", "C", "A"},
      .fill = 0.0,
  });
  expect(both.features() == std::vector<std::string>({"B", "C", "A"}),
         "align should conform feature labels exactly");
  expect(both.label().values().as_note().vector() ==
             std::vector<std::string_view>({"r2", "missing"}),
         "align should conform case labels exactly");
  expect(both.features("A").as_series().to_vec<double>() ==
             std::vector<double>({20.0, 0.0}),
         "align(fill) should fill missing numeric case-label rows");
  expect(both.features("C").as_series().to_vec<double>() ==
             std::vector<double>({0.0, 0.0}),
         "align(fill) should create filled numeric features for missing feature labels");
  expect(!pluss::table::detail::has_validity_mask(both.features("C")) ||
             pluss::table::detail::validity_mask(both.features("C"))->to_vec<bool>() ==
                 std::vector<bool>({true, true}),
         "align(fill) missing numeric features should be valid filled values");
}

void test_timeline_epoch_days_surface() {
  const auto timeline = pluss::make_timeline({
      "1970-01-01T00:00:00Z",
      "1970-01-02T00:00:00Z",
      "1970-01-03T00:00:00Z",
  });
  const auto slate_days =
      Slate(timeline).tl().epoch(pluss::table::TimeUnit::Days);
  expect(slate_days.as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1, 2}),
         "Slate::tl().epoch(TimeUnit::Days) should return epoch-day integers");

  const auto frame = DataFrame({
      {"when", Slate(timeline)},
      {"value", Slate(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0}))},
  });
  const auto frame_days = frame.features({"when"}).tl().epoch(pluss::table::TimeUnit::Days);
  expect(frame_days.features("when").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1, 2}),
         "DataFrame::tl().epoch(TimeUnit::Days) should delegate through Slate::tl()");
}

void test_timeline_span_surface() {
  const auto day = [](std::string_view iso) {
    return pluss::make_timeline(iso).epoch_days().to_vec<std::int64_t>(0);
  };
  const auto has_day = [](const pluss::table::BusinessCalendar& calendar, std::int64_t label) {
    return std::find(calendar.holidays.begin(), calendar.holidays.end(), label) != calendar.holidays.end();
  };

  const auto starts = Slate(pluss::make_timeline({
      "2024-01-31T00:00:00Z",
      "2024-02-29T00:00:00Z",
      "2024-01-01T00:00:00Z",
  }));
  const auto ends = Slate(pluss::make_timeline({
      "2024-02-01T00:00:00Z",
      "2025-02-28T00:00:00Z",
      "2024-01-05T00:00:00Z",
  }));

  expect(starts.tl().span(ends, pluss::table::TimeUnit::Days)
             .as_series()
             .to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({1, 365, 4}),
         "Slate::tl().span(Days) should use fixed UTC 24-hour spans");

  pluss::table::TimelineSpanOptions complete_months;
  complete_months.unit = pluss::table::TimeUnit::Months;
  expect(starts.tl().span(ends, complete_months)
             .as_series()
             .to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 11, 0}),
         "Slate::tl().span(Months, Complete) should count completed calendar months");

  pluss::table::TimelineSpanOptions boundary_years;
  boundary_years.unit = pluss::table::TimeUnit::Years;
  boundary_years.mode = pluss::table::TimelineSpanMode::Boundary;
  expect(starts.tl().span(ends, boundary_years)
             .as_series()
             .to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1, 0}),
         "Slate::tl().span(Years, Boundary) should count crossed year labels");

  pluss::table::TimelineSpanOptions month_end_months;
  month_end_months.unit = pluss::table::TimeUnit::Months;
  month_end_months.anchor = pluss::table::TimelineSpanAnchor::MonthEnd;
  expect(Slate(pluss::make_timeline({"2024-01-31T00:00:00Z"}))
             .tl()
             .span(Slate(pluss::make_timeline({"2024-02-29T00:00:00Z"})), month_end_months)
             .as_series()
             .to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({1}),
         "Slate::tl().span(Months, MonthEnd) should treat paired month ends as complete");

  pluss::table::TimelineSpanOptions month_end_years;
  month_end_years.unit = pluss::table::TimeUnit::Years;
  month_end_years.anchor = pluss::table::TimelineSpanAnchor::MonthEnd;
  expect(Slate(pluss::make_timeline({"2024-02-29T00:00:00Z"}))
             .tl()
             .span(Slate(pluss::make_timeline({"2025-02-28T00:00:00Z"})), month_end_years)
             .as_series()
             .to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({1}),
         "Slate::tl().span(Years, MonthEnd) should handle leap-day to February month-end");

  const auto holiday =
      pluss::make_timeline({"2024-01-03T00:00:00Z"}).epoch_days().to_vec<std::int64_t>(0);
  pluss::table::TimelineSpanOptions business;
  business.unit = pluss::table::TimeUnit::BusinessDays;
  business.calendar.holidays = {holiday};
  expect(starts.tl().span(ends, business).as_series().to_vec<std::int64_t>()[2] == 3,
         "Slate::tl().span(BusinessDays) should honor explicit holiday calendars");

  const auto business_start = Slate(pluss::make_timeline({"2024-01-01T00:00:00Z"}));
  const auto business_end = Slate(pluss::make_timeline({"2024-01-05T00:00:00Z"}));
  pluss::table::TimelineSpanOptions closed;
  closed.unit = pluss::table::TimeUnit::BusinessDays;
  closed.closed = pluss::table::TimelineSpanClosed::Right;
  expect(business_start.tl().span(business_end, closed).as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({4}),
         "Slate::tl().span(BusinessDays, Right) should use (start,end]");
  closed.closed = pluss::table::TimelineSpanClosed::Both;
  expect(business_start.tl().span(business_end, closed).as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({5}),
         "Slate::tl().span(BusinessDays, Both) should use [start,end]");
  closed.closed = pluss::table::TimelineSpanClosed::Neither;
  expect(business_start.tl().span(business_end, closed).as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({3}),
         "Slate::tl().span(BusinessDays, Neither) should use (start,end)");

  pluss::table::TimelineSpanOptions bad_weekmask;
  bad_weekmask.unit = pluss::table::TimeUnit::BusinessDays;
  bad_weekmask.calendar.weekmask = 0;
  expect_throw(
      [&] { (void)business_start.tl().span(business_end, bad_weekmask); },
      "Slate::tl().span(BusinessDays) should reject empty business weekmasks");

  pluss::table::TimelineSpanOptions bad_holiday;
  bad_holiday.unit = pluss::table::TimeUnit::BusinessDays;
  bad_holiday.calendar.holidays = {1'700'000'000'000'000'000LL};
  expect_throw(
      [&] { (void)business_start.tl().span(business_end, bad_holiday); },
      "Slate::tl().span(BusinessDays) should reject holiday values that look like epoch nanos");

  pluss::table::TimelineSpanOptions bad_calendar_tz;
  bad_calendar_tz.unit = pluss::table::TimeUnit::BusinessDays;
  bad_calendar_tz.calendar.timezone = "America/Edmonton";
  expect_throw(
      [&] { (void)business_start.tl().span(business_end, bad_calendar_tz); },
      "Slate::tl().span(BusinessDays) should reject calendar timezone mismatches");

  const auto edmonton_end = Slate(pluss::make_timeline({"2024-01-05T00:00:00Z"})
                                      .tz_convert("America/Edmonton"));
  expect_throw(
      [&] { (void)business_start.tl().span(edmonton_end, business); },
      "Slate::tl().span(BusinessDays) should reject timeline timezone mismatches");

  const auto monfri = pluss::table::BusinessCalendar::monfri();
  expect(monfri.weekmask == 0b0111110 && monfri.holidays.empty(),
         "BusinessCalendar::monfri should be Monday-Friday with no holidays");

  const auto us = pluss::table::BusinessCalendar::usa_federal(2026, 2026);
  expect(has_day(us, day("2026-07-03T00:00:00Z")) &&
             has_day(us, day("2026-10-12T00:00:00Z")) &&
             has_day(us, day("2026-11-11T00:00:00Z")),
         "BusinessCalendar::usa_federal should include observed Independence, Columbus, and Veterans holidays");

  const auto ca = pluss::table::BusinessCalendar::canada_federal(2026, 2026);
  expect(has_day(ca, day("2026-09-30T00:00:00Z")) &&
             has_day(ca, day("2026-12-28T00:00:00Z")),
         "BusinessCalendar::canada_federal should include Truth/Reconciliation and observed Boxing Day");

  const auto uk = pluss::table::BusinessCalendar::uk_bank(2026, 2026);
  expect(uk.timezone == "Europe/London" &&
             has_day(uk, day("2026-04-03T00:00:00Z")) &&
             has_day(uk, day("2026-04-06T00:00:00Z")) &&
             has_day(uk, day("2026-05-04T00:00:00Z")) &&
             has_day(uk, day("2026-08-31T00:00:00Z")),
         "BusinessCalendar::uk_bank should include England/Wales bank holiday rules");

  const auto target = pluss::table::BusinessCalendar::target2(2026, 2026);
  expect(target.timezone == "UTC" &&
             has_day(target, day("2026-05-01T00:00:00Z")) &&
             has_day(target, day("2026-12-26T00:00:00Z")),
         "BusinessCalendar::target2 should include TARGET closing days");

  const auto nyse = pluss::table::BusinessCalendar::nyse(2026, 2026);
  expect(nyse.timezone == "America/New_York" &&
             has_day(nyse, day("2026-04-03T00:00:00Z")) &&
             has_day(nyse, day("2026-06-19T00:00:00Z")) &&
             !has_day(nyse, day("2026-10-12T00:00:00Z")) &&
             !has_day(nyse, day("2026-11-11T00:00:00Z")),
         "BusinessCalendar::nyse should include exchange holidays and exclude Columbus/Veterans closures");

  pluss::table::TimelineSpanOptions nyse_span;
  nyse_span.unit = pluss::table::TimeUnit::BusinessDays;
  nyse_span.calendar = nyse;
  expect(Slate(pluss::make_timeline({"2026-04-02T00:00:00Z"}).tz_convert("America/New_York"))
             .tl()
             .span(Slate(pluss::make_timeline({"2026-04-06T00:00:00Z"}).tz_convert("America/New_York")), nyse_span)
             .as_series()
             .to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({1}),
         "BusinessCalendar::nyse should remove Good Friday from business-day spans");

  const auto ny = pluss::make_timeline({"2026-07-01T12:00:00Z", "2026-01-01T12:00:00Z"})
                      .tz_convert("America/New_York")
                      .hour()
                      .to_vec<std::int64_t>();
  expect(ny == std::vector<std::int64_t>({8, 7}),
         "TimeZone should support America/New_York DST and standard offsets");
  const auto london = pluss::make_timeline({"2026-07-01T12:00:00Z", "2026-01-01T12:00:00Z"})
                          .tz_convert("Europe/London")
                          .hour()
                          .to_vec<std::int64_t>();
  expect(london == std::vector<std::int64_t>({13, 12}),
         "TimeZone should support Europe/London DST and standard offsets");

  const auto frame = DataFrame({
      {"start", starts},
      {"end", ends},
  });
  const auto frame_months =
      frame.features({"start"}).tl().span(frame.features("end"), complete_months);
  expect(frame_months.features("start").as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 11, 0}),
         "DataFrame::tl().span should delegate through Slate::tl().span");
}

} // namespace

int main(int argc, char** argv) {
  using Test = std::pair<std::string, std::function<void()>>;
  const std::vector<Test> tests{
      {"union_matching_for_frame_math", test_union_matching_for_frame_math},
      {"explicit_common_matching", test_explicit_common_matching},
      {"repeated_caselbl_pair_in_order", test_repeated_caselbl_pair_in_order},
      {"matching_rejects_incompatible_case_types", test_matching_rejects_incompatible_case_types},
      {"case_aware_horizontal_composition", test_case_aware_horizontal_composition},
      {"vertical_case_extension", test_vertical_case_extension},
      {"unify_modes", test_unify_modes},
      {"vectorized_group_reductions", test_vectorized_group_reductions},
      {"group_phase5_contracts", test_group_phase5_contracts},
      {"group_transform_and_filter", test_group_transform_and_filter},
      {"group_yield_and_ndistinct", test_group_yield_and_ndistinct},
      {"contingency_and_split", test_contingency_and_split},
      {"temporal_subtraction_and_sort_builder", test_temporal_subtraction_and_sort_builder},
      {"bundelize_and_tabelize_interop", test_bundelize_and_tabelize_interop},
      {"group_phase7_hardening", test_group_phase7_hardening},
      {"lengthen_reshape", test_lengthen_reshape},
      {"hierarchical_case_axis_engine", test_hierarchical_case_axis_engine},
      {"stack_unstack_hierarchical_cases", test_stack_unstack_hierarchical_cases},
      {"transform_axis_surface", test_transform_axis_surface},
      {"category_factor", test_category_factor},
      {"streamed_csv_case_batches", test_streamed_csv_case_batches},
      {"case_aligned_set_and_reduction_axis_rules", test_case_aligned_set_and_reduction_axis_rules},
      {"align_reindex_equivalent", test_align_reindex_equivalent},
      {"timeline_epoch_days_surface", test_timeline_epoch_days_surface},
      {"timeline_span_surface", test_timeline_span_surface},
  };

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
    } catch (const std::exception& e) {
      std::cerr << "[FAIL] " << name << "\n" << e.what() << "\n";
      return 1;
    } catch (...) {
      std::cerr << "[FAIL] " << name << "\nunknown error\n";
      return 1;
    }
  }
  return 0;
}
