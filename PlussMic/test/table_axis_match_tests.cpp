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

  const auto result = left.add(right);
  expect(result.feature_names() == std::vector<std::string>({"x", "y", "z"}),
         "union matching should preserve left feature order and append right-only features");
  expect(result.cases().as_note().vector() ==
             std::vector<std::string_view>({"A", "C", "B"}),
         "union matching should preserve left case order and append right-only cases");

  const auto y = result.features("y");
  expect(y.as_series().to_vec<double>().front() == 210.0,
         "matched arithmetic should pair values by case labels");
  expect(y.validity_mask()->to_vec<bool>() ==
             std::vector<bool>({true, false, false}),
         "matched arithmetic should mark one-sided values missing");
  expect(result.features("x").validity_mask()->to_vec<bool>() ==
             std::vector<bool>({false, false, false}),
         "left-only feature arithmetic should be entirely missing");
  expect(result.features("z").validity_mask()->to_vec<bool>() ==
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
  const auto matched = left.matched_with(right, options);

  expect(matched.left.feature_names() == std::vector<std::string>({"y"}),
         "common feature matching should retain shared features only");
  expect(matched.left.cases().as_note().vector() ==
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

  const auto result = left.add(right);
  expect(result.ncases() == 2, "repeated cases should pair by occurrence without expansion");
  expect(result.features("x").as_series().to_vec<double>().front() == 11.0,
         "the first repeated occurrence should pair with the first peer occurrence");
  expect(result.features("x").validity_mask()->to_vec<bool>() ==
             std::vector<bool>({true, false}),
         "an unmatched repeated occurrence should be missing");

  FrameMatchOptions reject;
  reject.repeated_cases = RepeatedCaseRule::Reject;
  expect_throw(
      [&]() { (void)left.matched_with(right, reject); },
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
      [&]() { (void)text_cases.matched_with(numeric_cases); },
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
  expect(combined.cases().as_note().vector() ==
             std::vector<std::string_view>({"A", "C", "B"}),
         "horizontal composition should match cases by label");
  expect(combined.features("x").validity_mask()->to_vec<bool>() ==
             std::vector<bool>({true, true, false}),
         "horizontal composition should null-mark missing left rows");
  expect(combined.features("y").validity_mask()->to_vec<bool>() ==
             std::vector<bool>({true, false, true}),
         "horizontal composition should null-mark missing right rows");
  const auto y = combined.features("y").as_series().to_vec<double>();
  expect(y[0] == 20.0 && y[2] == 10.0,
         "horizontal composition should gather right values by case label");

  const auto updated = left.update(
      make_frame(
          {{"x", Thread(pluss::make_series(std::vector<double>{9.0}))}},
          {"A"}));
  expect(updated.cases().as_note().vector() ==
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
  expect(extended.feature_names() ==
             std::vector<std::string>({"x", "y", "z"}),
         "case extension should use the stable feature union");
  expect(extended.cases().as_note().vector() ==
             std::vector<std::string_view>({"A", "C", "B", "A"}),
         "case extension should append the full case axes");
  expect(extended.features("x").validity_mask()->to_vec<bool>() ==
             std::vector<bool>({true, true, false, false}),
         "case extension should null-fill a missing right feature");
  expect(extended.features("z").validity_mask()->to_vec<bool>() ==
             std::vector<bool>({false, false, true, true}),
         "case extension should null-fill a missing left feature");

  const auto common =
      left.extend_cases(right, FrameMatchRule::Common, true);
  expect(common.feature_names() == std::vector<std::string>({"y"}),
         "common case extension should retain shared features only");
  expect(common.cases().as_series().to_vec<std::int64_t>() ==
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
  expect(united.features("left_value").validity_mask()->to_vec<bool>() ==
             std::vector<bool>({true, true, false}),
         "unite mode should null-mark missing left payloads");
  expect(united.features("right_value").validity_mask()->to_vec<bool>() ==
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
  expect(long_form.feature_names() ==
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
  expect(long_form.cases().as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1, 2, 3}),
         "lengthen should create fresh contiguous cases by default");

  const auto preserved =
      frame.lengthen({"id"}, {"a", "b"}, "source", "measure", false);
  expect(preserved.cases().as_note().vector() ==
             std::vector<std::string_view>({"C1", "C2", "C1", "C2"}),
         "lengthen should tile original cases when requested");

  expect_throw(
      [&]() { (void)frame.lengthen({}, {"id", "a"}); },
      "lengthen should reject heterogeneous value payload kinds");
}

void test_category_catalogue() {
  const auto validity = pluss::make_series(
      std::vector<bool>{true, true, true, true, false});
  const auto frame = DataFrame({
      {"city", Thread(
          pluss::make_note({"Calgary", "Edmonton", "Calgary", "Banff", ""}),
          validity)},
  });

  const auto encoding = frame.catalogue("city");
  expect(encoding.levels.as_note().vector() ==
             std::vector<std::string_view>({"Calgary", "Edmonton", "Banff"}),
         "catalogue should preserve stable first-seen level order");
  expect(encoding.codes.as_series().to_vec<std::int64_t>() ==
             std::vector<std::int64_t>({0, 1, 0, 2, -1}),
         "catalogue should encode levels and reserve -1 for missing values");
  expect(encoding.codes.validity_mask()->to_vec<bool>() ==
             std::vector<bool>({true, true, true, true, false}),
         "catalogue should preserve source missingness");
  const auto decoded = encoding.decode();
  expect(decoded.as_note().vector()[0] == "Calgary" &&
             decoded.as_note().vector()[1] == "Edmonton" &&
             decoded.as_note().vector()[3] == "Banff",
         "category decoding should recover observed labels");
  expect(!decoded.validity_mask()->to_vec<bool>()[4],
         "category decoding should recover missingness");

  const auto sorted = frame.catalogue("city", true, true);
  expect(sorted.ordered &&
             sorted.levels.as_note().vector() ==
                 std::vector<std::string_view>({"Banff", "Calgary", "Edmonton"}),
         "catalogue should support explicit ordered sorted levels");
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
  test_category_catalogue();
  test_streamed_csv_case_batches();
  return 0;
}
