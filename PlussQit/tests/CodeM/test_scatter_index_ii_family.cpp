#include <pluss/pluss.h>

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdint>
#include <vector>

namespace {

void require_close(
    const std::vector<double>& actual,
    const std::vector<double>& expected,
    double tolerance = 1e-9) {
  REQUIRE(actual.size() == expected.size());
  for (std::size_t i = 0; i < actual.size(); ++i) {
    REQUIRE(std::abs(actual[i] - expected[i]) <= tolerance);
  }
}

} // namespace

TEST_CASE("index axis operations support full rank and negative dimensions", "[scatter-index-ii]") {
  const auto base = pluss::make_series({{1.0, 2.0, 3.0},
                                        {4.0, 5.0, 6.0},
                                        {7.0, 8.0, 9.0}});
  const auto rows = pluss::make_series({2, 0, 2});
  const auto additions = pluss::make_series({{10.0, 20.0, 30.0},
                                             {40.0, 50.0, 60.0},
                                             {1.0, 2.0, 3.0}});
  require_close(base.index_add(0, rows, additions).to_vec<double>(),
                {41.0, 52.0, 63.0, 4.0, 5.0, 6.0, 18.0, 30.0, 42.0});
  require_close(base.index_add(0, rows, additions, 0.5).to_vec<double>(),
                {21.0, 27.0, 33.0, 4.0, 5.0, 6.0, 12.5, 19.0, 25.5});

  const auto copy_base = pluss::make_series({{1, 2, 3}, {4, 5, 6}});
  const auto columns = pluss::make_series({2, 0});
  const auto replacements = pluss::make_series({{30, 10}, {60, 40}});
  REQUIRE(copy_base.index_copy(1, columns, replacements).to_vec<std::int64_t>() ==
          std::vector<std::int64_t>{10, 2, 30, 40, 5, 60});
  REQUIRE(copy_base.index_fill(-1, pluss::make_series({0, 2}), -1.0).to_vec<std::int64_t>() ==
          std::vector<std::int64_t>{-1, 2, -1, -1, 5, -1});
}

TEST_CASE("index add reverse gathers destination weights back to source slices",
          "[scatter-index-ii][autodiff]") {
  auto base = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}});
  auto source = pluss::make_series({{10.0, 20.0}, {30.0, 40.0}});
  base.tracks_slope_(true);
  source.tracks_slope_(true);

  const auto weights = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}});
  base.index_add(0, pluss::make_series({2, 0}), source, 2.0)
      .mul(weights)
      .sum()
      .reverse();

  require_close(base.sensitivity().to_vec<double>(), {1.0, 2.0, 3.0, 4.0, 5.0, 6.0});
  require_close(source.sensitivity().to_vec<double>(), {10.0, 12.0, 2.0, 4.0});
}

TEST_CASE("index fill reverse removes overwritten receiver slices",
          "[scatter-index-ii][autodiff]") {
  auto base = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}});
  base.tracks_slope_(true);
  base.index_fill(0, pluss::make_series({1}), 0.0).sum().reverse();
  require_close(base.sensitivity().to_vec<double>(), {1.0, 1.0, 0.0, 0.0, 1.0, 1.0});
}

TEST_CASE("index_reduce implements product mean and extrema inclusion", "[scatter-index-ii]") {
  const auto base = pluss::make_series({10.0, 20.0, 30.0, 40.0});
  const auto index = pluss::make_series({0, 0, 2});
  const auto source = pluss::make_series({2.0, 3.0, 5.0});

  require_close(base.index_reduce(0, index, source, "prod", true).to_vec<double>(),
                {60.0, 20.0, 150.0, 40.0});
  require_close(base.index_reduce(0, index, source, "prod", false).to_vec<double>(),
                {6.0, 20.0, 5.0, 40.0});
  require_close(base.index_reduce(0, index, source, "mean", true).to_vec<double>(),
                {5.0, 20.0, 17.5, 40.0});
  require_close(base.index_reduce(0, index, source, "mean", false).to_vec<double>(),
                {2.5, 20.0, 5.0, 40.0});
  require_close(base.index_reduce(0, index, source, "amax", false).to_vec<double>(),
                {3.0, 20.0, 5.0, 40.0});
  require_close(base.index_reduce(0, index, source, "amin", true).to_vec<double>(),
                {2.0, 20.0, 5.0, 40.0});
}

TEST_CASE("index_reduce product reverse handles ordinary and zero factors", "[scatter-index-ii][autodiff]") {
  auto base = pluss::make_series({2.0, 3.0});
  auto source = pluss::make_series({4.0, 5.0});
  base.tracks_slope_(true);
  source.tracks_slope_(true);
  base.index_reduce(0, pluss::make_series({0, 0}), source, "prod").sum().reverse();
  require_close(base.sensitivity().to_vec<double>(), {20.0, 1.0});
  require_close(source.sensitivity().to_vec<double>(), {10.0, 8.0});

  auto zero_base = pluss::make_series({0.0});
  auto zero_source = pluss::make_series({4.0, 5.0});
  zero_base.tracks_slope_(true);
  zero_source.tracks_slope_(true);
  zero_base.index_reduce(0, pluss::make_series({0, 0}), zero_source, "prod").sum().reverse();
  require_close(zero_base.sensitivity().to_vec<double>(), {20.0});
  require_close(zero_source.sensitivity().to_vec<double>(), {0.0, 0.0});
}

// With include_self=false the receiver's value still counts as a tie when it equals the
// result (libtorch divides the gradient among all of them and then zeroes the
// receiver's own share), so two source ties plus an equal receiver give 1/3 each.
// Pinned against libtorch in test_patch177_cpu_torch_autograd_parity.cpp.
TEST_CASE("index_reduce extrema ties divide among receiver and sources", "[scatter-index-ii][autodiff]") {
  const auto base = pluss::make_series({3.0});
  auto source = pluss::make_series({3.0, 3.0});
  source.tracks_slope_(true);
  base.index_reduce(0, pluss::make_series({0, 0}), source, "amax", false).sum().reverse();
  require_close(source.sensitivity().to_vec<double>(), {1.0 / 3.0, 1.0 / 3.0});
}

TEST_CASE("masked_fill broadcasts and routes both reverse paths", "[scatter-index-ii][autodiff]") {
  auto values = pluss::make_series({{1.0, 2.0, 3.0}});
  auto fill = pluss::make_series(9.0);
  const auto mask = pluss::make_series({{true, false, true}, {false, true, false}});
  values.tracks_slope_(true);
  fill.tracks_slope_(true);

  const auto result = values.masked_fill(mask, fill);
  REQUIRE(std::vector<std::int64_t>(result.sizes().begin(), result.sizes().end()) ==
          std::vector<std::int64_t>{2, 3});
  require_close(result.to_vec<double>(), {9.0, 2.0, 9.0, 1.0, 9.0, 3.0});
  result.sum().reverse();
  require_close(values.sensitivity().to_vec<double>(), {1.0, 1.0, 1.0});
  require_close(fill.sensitivity().to_vec<double>(), {3.0});
}

TEST_CASE("slice and select scatter replace exact views and reverse cleanly", "[scatter-index-ii][autodiff]") {
  auto base = pluss::make_series({{0.0, 1.0, 2.0, 3.0, 4.0, 5.0},
                                  {6.0, 7.0, 8.0, 9.0, 10.0, 11.0}});
  auto source = pluss::make_series({{100.0, 200.0}, {300.0, 400.0}});
  base.tracks_slope_(true);
  source.tracks_slope_(true);
  const auto sliced = base.slice_scatter(source, 1, 1, 5, 2);
  require_close(sliced.to_vec<double>(),
                {0.0, 100.0, 2.0, 200.0, 4.0, 5.0,
                 6.0, 300.0, 8.0, 400.0, 10.0, 11.0});
  sliced.sum().reverse();
  require_close(base.sensitivity().to_vec<double>(),
                {1.0, 0.0, 1.0, 0.0, 1.0, 1.0,
                 1.0, 0.0, 1.0, 0.0, 1.0, 1.0});
  require_close(source.sensitivity().to_vec<double>(), {1.0, 1.0, 1.0, 1.0});

  const auto selected = pluss::make_series({{1, 2, 3}, {4, 5, 6}})
      .select_scatter(pluss::make_series({30, 60}), 1, -1);
  REQUIRE(selected.to_vec<std::int64_t>() ==
          std::vector<std::int64_t>{1, 2, 30, 4, 5, 60});
}

TEST_CASE("index copy reverse separates overwritten and source slices", "[scatter-index-ii][autodiff]") {
  auto base = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
  auto source = pluss::make_series({{30.0, 10.0}, {60.0, 40.0}});
  base.tracks_slope_(true);
  source.tracks_slope_(true);
  base.index_copy(1, pluss::make_series({2, 0}), source).sum().reverse();
  require_close(base.sensitivity().to_vec<double>(), {0.0, 1.0, 0.0, 0.0, 1.0, 0.0});
  require_close(source.sensitivity().to_vec<double>(), {1.0, 1.0, 1.0, 1.0});
}

TEST_CASE("select scatter reverse separates retained cells from the source slice",
          "[scatter-index-ii][autodiff]") {
  auto base = pluss::make_series({{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
  auto source = pluss::make_series({20.0, 50.0});
  base.tracks_slope_(true);
  source.tracks_slope_(true);
  base.select_scatter(source, 1, 1).sum().reverse();
  require_close(base.sensitivity().to_vec<double>(), {1.0, 0.0, 1.0, 1.0, 0.0, 1.0});
  require_close(source.sensitivity().to_vec<double>(), {1.0, 1.0});
}

TEST_CASE("scatter index II operations reject malformed contracts", "[scatter-index-ii]") {
  const auto base = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
  REQUIRE_THROWS(base.index_add(0, pluss::make_series({{0, 1}}), base));
  REQUIRE_THROWS(base.index_copy(
      0, pluss::make_series({0}), pluss::make_series({1.0}).reshape({1, 1})));
  REQUIRE_THROWS(base.index_reduce(0, pluss::make_series({0, 1}), base, "sum"));
  REQUIRE_THROWS(base.select_scatter(pluss::make_series({9.0, 8.0}).reshape({2, 1}), 1, 0));
  REQUIRE_THROWS(base.slice_scatter(pluss::make_series({9.0, 8.0}).reshape({2, 1}), 1, 0, 2));
  REQUIRE_THROWS(base.masked_fill(pluss::make_series({{1, 0}, {0, 1}}), 9.0));
  REQUIRE_THROWS(base.masked_fill(pluss::make_series({{true, false}, {false, true}}),
                                  pluss::make_series({9.0})));
}

TEST_CASE("in-place index operations preserve receiver storage", "[scatter-index-ii][inplace]") {
  auto value = pluss::make_series({{1.0, 2.0}, {3.0, 4.0}});
  const void* storage = value.raw_data();
  value.index_add_(0, pluss::make_series({1}), pluss::make_series({{10.0, 20.0}}));
  REQUIRE(value.raw_data() == storage);
  require_close(value.to_vec<double>(), {1.0, 2.0, 13.0, 24.0});
}
