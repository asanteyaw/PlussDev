// Regression coverage for the ordered-axis audit: every PlussMic time-series
// kernel whose algorithm assumes a nondecreasing axis (join_asof's
// two-pointer merge, resample_bins' bucket assignment) must now throw on an
// unsorted axis instead of silently producing wrong matches. Exact-mode
// alignment is the one path that genuinely does not need order (it is a
// hash-map join on raw epoch-nanos ticks) and must keep working on an
// unsorted axis.

#include "table/DataFrame.h"

#include <array>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using pluss::table::DataFrame;
using pluss::table::Thread;
using pluss::table::TsAlignMode;
using pluss::table::TsOptions;

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

template <typename Fn>
void expect_no_throw(Fn&& fn, const std::string& message) {
  try {
    fn();
  } catch (const std::exception& e) {
    throw std::runtime_error(message + " (threw: " + e.what() + ")");
  }
}

pluss::Timeline make_timeline(std::span<const std::string_view> iso) {
  const std::array<std::int64_t, 1> shape{static_cast<std::int64_t>(iso.size())};
  return pluss::codeh::ops_internal::timeline_from_utf8(
      iso, std::span<const std::int64_t>(shape.data(), shape.size()));
}

// 2024-01-03, 2024-01-01, 2024-01-02 -- out of order.
Thread unsorted_axis() {
  const std::array<std::string_view, 3> iso{
      "2024-01-03T00:00:00Z",
      "2024-01-01T00:00:00Z",
      "2024-01-02T00:00:00Z",
  };
  return Thread(make_timeline(std::span<const std::string_view>(iso.data(), iso.size())));
}

Thread sorted_axis() {
  const std::array<std::string_view, 3> iso{
      "2024-01-01T00:00:00Z",
      "2024-01-02T00:00:00Z",
      "2024-01-03T00:00:00Z",
  };
  return Thread(make_timeline(std::span<const std::string_view>(iso.data(), iso.size())));
}

Thread values_1_2_3() {
  return Thread(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}));
}

void test_align_to_ordered_modes_reject_unsorted_axis() {
  const auto source = values_1_2_3();
  const auto unsorted = unsorted_axis();
  const auto sorted = sorted_axis();

  // Either side being unsorted must fail for every order-dependent mode.
  expect_throw(
      [&] { (void)source.ts().align_to(unsorted, sorted, TsAlignMode::Backward); },
      "align_to(Backward) must reject an unsorted source axis");
  expect_throw(
      [&] { (void)source.ts().align_to(sorted, unsorted, TsAlignMode::Backward); },
      "align_to(Backward) must reject an unsorted target axis");
  expect_throw(
      [&] { (void)source.ts().align_to(unsorted, sorted, TsAlignMode::Forward); },
      "align_to(Forward) must reject an unsorted source axis");
  expect_throw(
      [&] { (void)source.ts().align_to(unsorted, sorted, TsAlignMode::Nearest); },
      "align_to(Nearest) must reject an unsorted source axis");

  // The actual failure must be about sortedness, not something incidental.
  try {
    (void)source.ts().align_to(unsorted, sorted, TsAlignMode::Backward);
    expect(false, "unreachable");
  } catch (const std::exception& e) {
    const std::string what = e.what();
    expect(what.find("sorted") != std::string::npos || what.find("decreasing") != std::string::npos,
           "align_to(Backward)'s failure on an unsorted axis should name sortedness, got: " + what);
  }
}

void test_align_to_exact_mode_tolerates_unsorted_axis() {
  // Exact alignment is a hash-map join on raw epoch-nanos ticks -- it must
  // keep working on an unsorted axis, since the audit's point is that
  // *order-dependent* kernels need checking, not every ordered-axis kernel.
  const auto source = values_1_2_3();
  const auto unsorted = unsorted_axis();
  const auto sorted = sorted_axis();

  expect_no_throw(
      [&] { (void)source.ts().align_to(unsorted, sorted, TsAlignMode::Exact); },
      "align_to(Exact) must not require a sorted source axis");
  expect_no_throw(
      [&] { (void)source.ts().align_to(sorted, unsorted, TsAlignMode::Exact); },
      "align_to(Exact) must not require a sorted target axis");

  // And it must still be correct -- matching rows by epoch tick, not position.
  const auto out = source.ts().align_to(sorted, unsorted, TsAlignMode::Exact);
  // unsorted axis is [Jan 3, Jan 1, Jan 2]; sorted source values are
  // [1.0 @ Jan1, 2.0 @ Jan2, 3.0 @ Jan3], so the exact join onto the
  // unsorted target axis order should read back [3.0, 1.0, 2.0].
  const auto vals = out.as_series().to(pluss::codem::ScalarType::Float64).to_vec<double>();
  expect(vals.size() == 3 && vals[0] == 3.0 && vals[1] == 1.0 && vals[2] == 2.0,
         "align_to(Exact) must match by epoch tick regardless of either axis's order");
}

void test_core_index_guards_ordered_kernels() {
  const auto key = pluss::timeline::make("2024-01-02T00:00:00Z");
  const auto unchecked = unsorted_axis().as_timeline().index(false);

  expect_throw([&] { (void)unchecked.searchsorted(key); },
               "unchecked searchsorted must validate ordering");
  expect_throw([&] { (void)unchecked.asof(key); },
               "unchecked asof must validate ordering");
  expect_throw([&] { (void)unchecked.slice(key, key); },
               "unchecked slice must validate ordering");

  const auto ordered = sorted_axis().as_timeline().index(false);
  expect(ordered.searchsorted(key) == 1,
         "ordered kernels must lazily validate a sorted unchecked accessor");
  expect(ordered.asof(key) == 1,
         "the validated state must remain reusable across ordered kernels");
}

void test_thread_resample_rejects_unsorted_axis() {
  const auto values = values_1_2_3();
  const auto unsorted = unsorted_axis();

  expect_throw(
      [&] {
        (void)values.ts()
            .resample(unsorted, "D", pluss::subsp::stats::ts::ResampleOp::Mean)
            .values;
      },
      "TimeSeriesAccessor::resample must reject an unsorted axis");

  expect_throw(
      [&] { (void)values.ts().resample(unsorted, "D").mean(); },
      "TimeSeriesResampler::mean (via resample_bins) must reject an unsorted axis");
}

void test_thread_resampler_terminal_ops_reject_unsorted_axis() {
  const auto values = values_1_2_3();
  const auto unsorted = unsorted_axis();

  expect_throw([&] { (void)values.ts().resample(unsorted, "D").asfreq(); },
               "TimeSeriesResampler::asfreq must reject an unsorted axis");
  expect_throw([&] { (void)values.ts().resample(unsorted, "D").asfreq(0.0); },
               "TimeSeriesResampler::asfreq(fill_value) must reject an unsorted axis");
  expect_throw([&] { (void)values.ts().resample(unsorted, "D").ffill(); },
               "TimeSeriesResampler::ffill must reject an unsorted axis");
  expect_throw([&] { (void)values.ts().resample(unsorted, "D").bfill(); },
               "TimeSeriesResampler::bfill must reject an unsorted axis");
  expect_throw([&] { (void)values.ts().resample(unsorted, "D").nearest(); },
               "TimeSeriesResampler::nearest must reject an unsorted axis");
}

void test_dataframe_resample_rejects_unsorted_axis() {
  DataFrame df({
      {"value", values_1_2_3()},
      {"date", unsorted_axis()},
  });
  const auto ts_opts = TsOptions{.axis = df.features("date")};

  expect_throw(
      [&] { (void)df.features({"value"}).ts(ts_opts).resample("D", pluss::subsp::stats::ts::ResampleOp::Mean); },
      "DataFrameTsAccessor::resample must reject an unsorted axis");
  expect_throw(
      [&] { (void)df.features({"value"}).ts(ts_opts).resample("D").asfreq(); },
      "DataFrameTsResampler::asfreq must reject an unsorted axis");
  expect_throw(
      [&] { (void)df.features({"value"}).ts(ts_opts).resample("D").ffill(); },
      "DataFrameTsResampler::ffill must reject an unsorted axis");
}

void test_dataframe_resample_accepts_sorted_axis() {
  // Positive control: the same shape, sorted, must keep working -- the fix
  // must not have made resample universally stricter than before.
  DataFrame df({
      {"value", values_1_2_3()},
      {"date", sorted_axis()},
  });
  const auto ts_opts = TsOptions{.axis = df.features("date")};
  expect_no_throw(
      [&] { (void)df.features({"value"}).ts(ts_opts).resample("D", pluss::subsp::stats::ts::ResampleOp::Mean); },
      "DataFrameTsAccessor::resample must still accept a sorted axis");
}

} // namespace

int main() {
  try {
    test_align_to_ordered_modes_reject_unsorted_axis();
    test_align_to_exact_mode_tolerates_unsorted_axis();
    test_core_index_guards_ordered_kernels();
    test_thread_resample_rejects_unsorted_axis();
    test_thread_resampler_terminal_ops_reject_unsorted_axis();
    test_dataframe_resample_rejects_unsorted_axis();
    test_dataframe_resample_accepts_sorted_axis();
    std::cout << "table_ts_unsorted_axis_tests passed\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "table_ts_unsorted_axis_tests failed: " << e.what() << '\n';
    return 1;
  }
}
