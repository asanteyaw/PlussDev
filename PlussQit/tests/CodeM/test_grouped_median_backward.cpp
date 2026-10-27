#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::Series;
using pluss::ScalarType;
using pluss::make_series;

// Series::grouped_median had no reverse-mode node at all (Series::grouped_median
// called straight into native::cpu::grouped_median, a raw nth_element buffer
// computation with nothing wired to the tape) -- caught by [backward-manifest]
// and left as a deliberately-deferred gap (see gen_series_backward_manifest.py
// history) until now. Fixed via a new read-only native helper,
// native::cpu::grouped_median_bounds, that independently recomputes the two
// raw order statistics grouped_median averages together for an even-sized
// group (equal to each other for an odd-sized group), then applies the same
// value-equality-mask technique grouped_mode/median_flat_impl use, once per
// bound at half weight each. There is no libtorch equivalent to check
// against (grouped_median is a Pluss-specific extension), so this pins the
// gradient down with hand-derived exact values instead of an oracle
// comparison.
TEST_CASE("Series::grouped_median reverse: odd-sized group routes full gradient to the single median element",
          "[autodiff][structural][grouped]") {
  // Group 0 = {5.0, 1.0, 3.0} (n=3, odd) -> sorted {1,3,5}, median = 3.0,
  // which is the single element at index 2 (untied) -- full gradient there.
  // Group 1 = {9.0, 9.0, 2.0} (n=3, odd) -> sorted {2,9,9}, median = 9.0,
  // tied at indices 3 and 4 -- gradient split evenly between them.
  Series values = make_series(std::vector<double>{5.0, 1.0, 3.0, 9.0, 9.0, 2.0}, {6});
  Series group_ids = make_series(std::vector<std::int64_t>{0, 0, 0, 1, 1, 1}, {6}).to(ScalarType::Int64);
  values.tracks_slope_(true);

  const Series out = values.grouped_median(group_ids, 2);
  REQUIRE(out.host<double>() == std::vector<double>{3.0, 9.0});

  // loss = sum(out^2) -> d(loss)/d(out) = 2*out = {6.0, 18.0}.
  out.mul(out).sum().reverse();
  REQUIRE(values.has_sensitivity());
  const auto g = values.sensitivity().host<double>();
  const std::vector<double> expected{0.0, 0.0, 6.0, 9.0, 9.0, 0.0};
  REQUIRE(g.size() == expected.size());
  for (std::size_t i = 0; i < g.size(); ++i) {
    INFO("index " << i);
    REQUIRE(g[i] == Catch::Approx(expected[i]).margin(1e-12));
  }
}

TEST_CASE("Series::grouped_median reverse: even-sized group splits gradient across both middle elements",
          "[autodiff][structural][grouped]") {
  // Group 0 = {1.0, 3.0, 5.0, 7.0} (n=4, even) -> sorted {1,3,5,7}; lower
  // middle = 3.0 (index 1), upper middle = 5.0 (index 2); median = 4.0.
  // Neither bound is tied with anything else in the group.
  Series values = make_series(std::vector<double>{1.0, 3.0, 5.0, 7.0}, {4});
  Series group_ids = make_series(std::vector<std::int64_t>{0, 0, 0, 0}, {4}).to(ScalarType::Int64);
  values.tracks_slope_(true);

  const Series out = values.grouped_median(group_ids, 1);
  REQUIRE(out.host<double>() == std::vector<double>{4.0});

  // loss = 10 * out -> d(loss)/d(out) = 10, split 5.0/5.0 between the lower
  // (index 1) and upper (index 2) middle elements; the two outer elements
  // (indices 0 and 3) get exactly zero.
  out.mul(10.0).sum().reverse();
  REQUIRE(values.has_sensitivity());
  const auto g = values.sensitivity().host<double>();
  const std::vector<double> expected{0.0, 5.0, 5.0, 0.0};
  REQUIRE(g.size() == expected.size());
  for (std::size_t i = 0; i < g.size(); ++i) {
    INFO("index " << i);
    REQUIRE(g[i] == Catch::Approx(expected[i]).margin(1e-12));
  }
}

TEST_CASE("Series::grouped_median reverse: repeated-value even group splits gradient across all four",
          "[autodiff][structural][grouped]") {
  // Group 0 = {2.0, 2.0, 2.0, 2.0} (n=4, even) -> lower == upper == 2.0, and
  // ALL four elements match both bounds -- the full gradient must still
  // split evenly across all four (0.5*g/4 + 0.5*g/4 = g/4 each), not
  // concentrate anywhere.
  Series values = make_series(std::vector<double>{2.0, 2.0, 2.0, 2.0}, {4});
  Series group_ids = make_series(std::vector<std::int64_t>{0, 0, 0, 0}, {4}).to(ScalarType::Int64);
  values.tracks_slope_(true);

  const Series out = values.grouped_median(group_ids, 1);
  REQUIRE(out.host<double>() == std::vector<double>{2.0});

  out.mul(8.0).sum().reverse();  // d(loss)/d(out) = 8.0
  REQUIRE(values.has_sensitivity());
  const auto g = values.sensitivity().host<double>();
  for (double v : g) {
    REQUIRE(v == Catch::Approx(2.0).margin(1e-12));  // 8.0 / 4 elements
  }
}
