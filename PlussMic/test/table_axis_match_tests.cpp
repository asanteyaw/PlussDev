#include <cmath>
#include <filesystem>
#include <fstream>
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
using pluss::table::Thread;

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
    std::initializer_list<std::pair<std::string, Thread>> features,
    std::initializer_list<std::string_view> cases) {
  return DataFrame(
      features,
      Thread(pluss::make_note(cases)));
}

void test_union_matching_for_frame_math() {
  const auto left = make_frame(
      {
          {"x", Thread(pluss::make_series(std::vector<double>{1.0, 2.0}))},
          {"y", Thread(pluss::make_series(std::vector<double>{10.0, 20.0}))},
      },
      {"A", "C"});
  const auto right = make_frame(
      {
          {"y", Thread(pluss::make_series(std::vector<double>{100.0, 200.0}))},
          {"z", Thread(pluss::make_series(std::vector<double>{7.0, 8.0}))},
      },
      {"B", "A"});

  const auto positional = left.add(right);
  expect(positional.features() == std::vector<std::string>({"x", "y"}),
         "default frame arithmetic should preserve left feature labels under positional semantics");
  expect(positional.caselbl().as_note().vector() ==
             std::vector<std::string_view>({"A", "C"}),
         "default frame arithmetic should preserve left case labels under positional semantics");
  expect(positional.features("x").as_series().to_vec<double>() ==
             std::vector<double>({101.0, 202.0}),
         "default frame arithmetic should pair values by position");
  const auto result = left.add(right, {.align = pluss::table::AlignBy::Both});
  expect(result.features() == std::vector<std::string>({"x", "y", "z"}),
         "union matching should preserve left feature order and append right-only features");
  expect(result.caselbl().as_note().vector() ==
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
          {"x", Thread(pluss::make_series(std::vector<double>{1.0, 2.0}))},
          {"y", Thread(pluss::make_series(std::vector<double>{10.0, 20.0}))},
      },
      {"A", "C"});
  const auto right = make_frame(
      {
          {"y", Thread(pluss::make_series(std::vector<double>{100.0, 200.0}))},
          {"z", Thread(pluss::make_series(std::vector<double>{7.0, 8.0}))},
      },
      {"B", "A"});

  FrameMatchOptions options;
  options.cases = FrameMatchRule::Common;
  options.features = FrameMatchRule::Common;
  const auto matched = left.match(right, options);

  expect(matched.left.features() == std::vector<std::string>({"y"}),
         "common feature matching should retain shared features only");
  expect(matched.left.caselbl().as_note().vector() ==
             std::vector<std::string_view>({"A"}),
         "common case matching should retain shared cases only");
  expect(matched.left.features("y").as_series().to_vec<double>() ==
             std::vector<double>({10.0}),
         "common matching should gather the left value");
  expect(matched.right.features("y").as_series().to_vec<double>() ==
             std::vector<double>({200.0}),
         "common matching should gather the corresponding right value");
}

void test_repeated_cases_pair_in_order() {
  const auto left = make_frame(
      {{"x", Thread(pluss::make_series(std::vector<double>{1.0, 2.0}))}},
      {"A", "A"});
  const auto right = make_frame(
      {{"x", Thread(pluss::make_series(std::vector<double>{10.0}))}},
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
      {{"x", Thread(pluss::make_series(std::vector<double>{1.0}))}},
      {"A"});
  const auto numeric_cases = DataFrame(
      {{"x", Thread(pluss::make_series(std::vector<double>{2.0}))}},
      Thread(pluss::make_series(std::vector<std::int64_t>{0})));

  expect_throw(
      [&]() { (void)text_cases.match(numeric_cases); },
      "matching should reject case axes with different payload kinds");
}

void test_case_aware_horizontal_composition() {
  const auto left = make_frame(
      {{"x", Thread(pluss::make_series(std::vector<double>{1.0, 2.0}))}},
      {"A", "C"});
  const auto right = make_frame(
      {{"y", Thread(pluss::make_series(std::vector<double>{10.0, 20.0}))}},
      {"B", "A"});

  const auto combined = left.combine(right);
  expect(combined.caselbl().as_note().vector() ==
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
          {{"x", Thread(pluss::make_series(std::vector<double>{9.0}))}},
          {"A"}));
  expect(updated.caselbl().as_note().vector() ==
             std::vector<std::string_view>({"A", "C"}),
         "update should keep the left case axis");
  expect(updated.features("x").as_series().to_vec<double>() ==
             std::vector<double>({9.0, 2.0}),
         "update should preserve left values where the right frame is missing");
}

void test_vertical_case_extension() {
  const auto left = make_frame(
      {
          {"x", Thread(pluss::make_series(std::vector<double>{1.0, 2.0}))},
          {"y", Thread(pluss::make_series(std::vector<double>{3.0, 4.0}))},
      },
      {"A", "C"});
  const auto right = make_frame(
      {
          {"y", Thread(pluss::make_series(std::vector<double>{5.0, 6.0}))},
          {"z", Thread(pluss::make_series(std::vector<double>{7.0, 8.0}))},
      },
      {"B", "A"});

  const auto extended = left.extend_cases(right);
  expect(extended.features() ==
             std::vector<std::string>({"x", "y", "z"}),
         "case extension should use the stable feature union");
  expect(extended.caselbl().as_note().vector() ==
             std::vector<std::string_view>({"A", "C", "B", "A"}),
         "case extension should append the full case axes");
  expect(pluss::table::detail::validity_mask(extended.features("x"))->to_vec<bool>() ==
             std::vector<bool>({true, true, false, false}),
         "case extension should null-fill a missing right feature");
  expect(pluss::table::detail::validity_mask(extended.features("z"))->to_vec<bool>() ==
             std::vector<bool>({false, false, true, true}),
         "case extension should null-fill a missing left feature");

  const auto common =
      left.extend_cases(right, FrameMatchRule::Common, true);
  expect(common.features() == std::vector<std::string>({"y"}),
         "common case extension should retain shared features only");
  expect(common.caselbl().as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1, 2, 3}),
         "fresh case extension should create one contiguous case axis");

  expect_throw(
      [&]() { (void)left.extend_cases(right, FrameMatchRule::Unite, false, true); },
      "case extension should optionally reject repeated output cases");
}

DataFrame make_unify_left() {
  return DataFrame({
      {"id", Thread(pluss::make_series(std::vector<std::int64_t>{1, 2}))},
      {"left_value", Thread(pluss::make_series(std::vector<double>{10.0, 20.0}))},
  });
}

DataFrame make_unify_right() {
  return DataFrame({
      {"id", Thread(pluss::make_series(std::vector<std::int64_t>{2, 3}))},
      {"right_value", Thread(pluss::make_series(std::vector<double>{200.0, 300.0}))},
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
      {"group", Thread(pluss::make_note({"A", "A", "B", "B", "B"}))},
      {"value", Thread(pluss::make_series(
          std::vector<double>{1.0, 3.0, 2.0, 4.0, 6.0}))},
  });
  const std::vector<DataFrame::GroupSpec> specs{
      {"n", "", DataFrame::GroupOp::Count},
      {"distinct", "value", DataFrame::GroupOp::Nunique},
      {"total", "value", DataFrame::GroupOp::Sum},
      {"average", "value", DataFrame::GroupOp::Mean},
      {"smallest", "value", DataFrame::GroupOp::Min},
      {"largest", "value", DataFrame::GroupOp::Max},
      {"spread", "value", DataFrame::GroupOp::Std},
      {"variance", "value", DataFrame::GroupOp::Var},
  };

  const auto grouped = static_cast<DataFrame>(
      frame.group_by({"group"}, specs, true, false));
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
}

void test_lengthen_reshape() {
  const auto frame = DataFrame(
      {
          {"id", Thread(pluss::make_note({"r1", "r2"}))},
          {"a", Thread(pluss::make_series(std::vector<double>{1.0, 2.0}))},
          {"b", Thread(pluss::make_series(std::vector<double>{3.0, 4.0}))},
      },
      Thread(pluss::make_note({"C1", "C2"})));

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
  expect(long_form.caselbl().as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1, 2, 3}),
         "lengthen should create fresh contiguous cases by default");

  const auto preserved =
      frame.lengthen({"id"}, {"a", "b"}, "source", "measure", false);
  expect(preserved.caselbl().as_note().vector() ==
             std::vector<std::string_view>({"C1", "C2", "C1", "C2"}),
         "lengthen should tile original cases when requested");

  expect_throw(
      [&]() { (void)frame.lengthen({}, {"id", "a"}); },
      "lengthen should reject heterogeneous value payload kinds");
}

void test_hierarchical_case_axis_engine() {
  const auto frame = DataFrame({
      {"region", Thread(pluss::make_note({"West", "West", "East", "East"}))},
      {"account", Thread(pluss::make_series(std::vector<std::int64_t>{102, 101, 201, 202}))},
      {"quarter", Thread(pluss::make_note({"Q2", "Q1", "Q1", "Q2"}))},
      {"sales", Thread(pluss::make_series(std::vector<double>{20.0, 10.0, 30.0, 40.0}))},
      {"cost", Thread(pluss::make_series(std::vector<double>{8.0, 4.0, 12.0, 16.0}))},
  });

  const auto indexed = frame.set_cases(
      DataFrame::SetCasesOptions{
          .features = {"region", "account"},
          .drop = false,
          .verify = true,
          .sort = false});
  expect(indexed.has_levels(), "set_cases should attach structured case levels");
  expect(indexed.levels() == std::vector<std::string>({"region", "account"}),
         "set_cases should preserve level names");
  expect(indexed.level("region").as_note().vector()[0] == "West",
         "level(name) should expose a typed case level");

  const auto selected = indexed.case_study(
      DataFrame::CasePick{.level = "region", .value = "West"});
  expect(selected.ncases() == 2 &&
             selected.level("account").as_series().to_vec<std::int64_t>() ==
                 std::vector<std::int64_t>({102, 101}),
         "case_study(level,value) should perform partial hierarchical selection");

  const auto full_key = indexed.case_study(
      DataFrame::CasePick{.key = {"East", "202"}});
  expect(full_key.ncases() == 1 &&
             full_key.features("sales").as_series().to_vec<double>() ==
                 std::vector<double>({40.0}),
         "case_study(key) should match a full structured key");

  const auto projected = indexed.features({"sales", "cost"});
  expect(projected.has_levels() && projected.levels() == indexed.levels(),
         "feature selection should preserve hierarchical case metadata");

  const auto sorted = indexed.sort_cases(
      DataFrame::SortCasesOptions{.levels = {"region", "account"}});
  expect(sorted.level("region").as_note().vector() ==
             std::vector<std::string_view>({"East", "East", "West", "West"}) &&
             sorted.level("account").as_series().to_vec<std::int64_t>() ==
                 std::vector<std::int64_t>({201, 202, 101, 102}),
         "sort_cases(levels) should sort by typed case levels");

  const std::vector<DataFrame::GroupSpec> specs{
      {"total", "sales", DataFrame::GroupOp::Sum},
      {"n", "", DataFrame::GroupOp::Count},
  };
  const auto grouped = static_cast<DataFrame>(
      indexed.group_by(DataFrame::CaseGroup{.level = "region"}, specs));
  expect(grouped.features("region").as_note().vector() ==
             std::vector<std::string_view>({"West", "East"}) &&
             grouped.features("total").as_series().to_vec<double>() ==
                 std::vector<double>({30.0, 70.0}),
         "group_by(CaseGroup) should group by case levels without requiring key features");

  const auto renamed = indexed.rename_levels({{"account", "acct"}});
  expect(renamed.levels() == std::vector<std::string>({"region", "acct"}),
         "rename_levels should rename a structured case level");
  const auto reordered = renamed.reorder_levels({"acct", "region"});
  expect(reordered.levels() == std::vector<std::string>({"acct", "region"}),
         "reorder_levels should reorder structured case levels");
  const auto dropped = reordered.drop_levels({"region"});
  expect(dropped.levels() == std::vector<std::string>({"acct"}),
         "drop_levels should remove selected structured case levels");
  const auto reset = indexed.reset_cases(
      DataFrame::ResetCasesOptions{.drop = false, .names = {"region_case", "account_case"}});
  expect(reset.has_feature("region_case") && reset.has_feature("account_case") &&
             !reset.has_levels(),
         "reset_cases should materialize levels as ordinary features and clear hierarchy");
}

void test_stack_unstack_hierarchical_cases() {
  const auto frame = DataFrame({
      {"region", Thread(pluss::make_note({"West", "West", "East", "East"}))},
      {"account", Thread(pluss::make_series(std::vector<std::int64_t>{101, 101, 201, 201}))},
      {"quarter", Thread(pluss::make_note({"Q1", "Q2", "Q1", "Q2"}))},
      {"sales", Thread(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0}))},
  }).set_cases({"region", "account", "quarter"}, true);

  const auto wide = frame.unstack("quarter");
  expect(wide.has_levels() &&
             wide.levels() == std::vector<std::string>({"region", "account"}) &&
             wide.features() == std::vector<std::string>({"Q1", "Q2"}) &&
             wide.features("Q1").as_series().to_vec<double>() ==
                 std::vector<double>({10.0, 30.0}) &&
             wide.features("Q2").as_series().to_vec<double>() ==
                 std::vector<double>({20.0, 40.0}),
         "unstack should move one case level into wide features");

  const auto long_again = wide.stack("quarter", "sales");
  expect(long_again.has_levels() &&
             long_again.levels() == std::vector<std::string>({"region", "account", "quarter"}) &&
             long_again.features("sales").as_series().to_vec<double>() ==
                 std::vector<double>({10.0, 30.0, 20.0, 40.0}),
         "stack should move features into the case hierarchy and keep typed values");

  const auto duplicate = DataFrame({
      {"region", Thread(pluss::make_note({"West", "West"}))},
      {"quarter", Thread(pluss::make_note({"Q1", "Q1"}))},
      {"sales", Thread(pluss::make_series(std::vector<double>{1.0, 2.0}))},
  }).set_cases({"region", "quarter"}, true);
  expect_throw([&]() { (void)duplicate.unstack("quarter"); },
               "unstack should reject duplicate output cells");
}

void test_transform_axis_surface() {
  const auto frame = DataFrame({
      {"x", Thread(pluss::make_series(std::vector<double>{1.0, 2.0}))},
      {"y", Thread(pluss::make_series(std::vector<double>{3.0, 4.0}))},
  });
  const auto by_feature = frame.transform(
      [](const Thread& value) {
        return value + 10.0;
      },
      pluss::table::Axis::Feature);
  expect(by_feature.features("x").as_series().to_vec<double>() ==
             std::vector<double>({11.0, 12.0}) &&
             by_feature.features("y").as_series().to_vec<double>() ==
                 std::vector<double>({13.0, 14.0}),
         "transform(thread_fn, Axis::Feature) should apply column-wise");

  const auto thread_transformed = frame.features("x").transform(
      [](const pluss::table::ThreadValue& value) {
        return pluss::table::ThreadValue(value.thread() + 5.0);
      });
  expect(thread_transformed.as_series().to_vec<double>() ==
             std::vector<double>({6.0, 7.0}),
         "Thread::transform(value_fn) should apply element-wise");
}

void test_category_factor() {
  const auto validity = pluss::make_series(
      std::vector<bool>{true, true, true, true, false});
  const auto frame = DataFrame({
      {"city", Thread(
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
          {"student", Thread(pluss::make_note({"Alice", "Bob", "Carol", "David"}))},
          {"A1", Thread(pluss::make_series(std::vector<double>{10.0, 20.0, 30.0, 40.0}))},
          {"A2", Thread(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}))},
      },
      Thread(pluss::make_series(std::vector<std::int64_t>{1001, 1002, 1003, 1004})));

  expect(df.shape() == std::pair<std::int64_t, std::int64_t>{4, 3},
         "DataFrame::shape should report cases x features");
  expect(df.features("A1").ncases() == 4 &&
             df.features("A1").shape() == std::pair<std::int64_t, std::int64_t>{4, 1},
         "Thread::ncases/shape should report a 1D case vector");

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

  const auto scholarship = Thread(pluss::make_series(
                               std::vector<double>{1500.0, 2500.0, 1000.0}))
                               .caselbl(Thread(pluss::make_series(
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
    df.set("one_row_series") = Thread(pluss::make_series(std::vector<double>{7235.0}));
  } catch (const std::exception&) {
    one_row_default_threw = true;
  }
  expect(one_row_default_threw,
         "plain 1D length-one Thread assignment should not broadcast or align by default");
}

} // namespace

int main() {
  test_union_matching_for_frame_math();
  test_explicit_common_matching();
  test_repeated_cases_pair_in_order();
  test_matching_rejects_incompatible_case_types();
  test_case_aware_horizontal_composition();
  test_vertical_case_extension();
  test_unify_modes();
  test_vectorized_group_reductions();
  test_lengthen_reshape();
  test_hierarchical_case_axis_engine();
  test_stack_unstack_hierarchical_cases();
  test_transform_axis_surface();
  test_category_factor();
  test_streamed_csv_case_batches();
  test_case_aligned_set_and_reduction_axis_rules();
  return 0;
}
