#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cstdint>
#include <vector>

namespace {

void require_equal_i64_(const std::vector<std::int64_t>& got, const std::vector<std::int64_t>& want) {
  REQUIRE(got.size() == want.size());
  for (std::size_t i = 0; i < got.size(); ++i) {
    REQUIRE(got[i] == want[i]);
  }
}

} // namespace

TEST_CASE("series set ops 1d contracts", "[setops][series]") {
  const auto a = pluss::make_series(std::vector<std::int64_t>{3, 1, 3, 2}, {4});
  const auto b = pluss::make_series(std::vector<std::int64_t>{2, 4, 1}, {3});

  require_equal_i64_(a.union1d(b, /*sorted=*/false).to_vec<std::int64_t>(), {3, 1, 2, 4});
  require_equal_i64_(a.union1d(b, /*sorted=*/true).to_vec<std::int64_t>(), {1, 2, 3, 4});

  require_equal_i64_(a.intersect1d(b, /*sorted=*/false).to_vec<std::int64_t>(), {1, 2});
  require_equal_i64_(a.intersect1d(b, /*sorted=*/true).to_vec<std::int64_t>(), {1, 2});

  require_equal_i64_(a.setdiff1d(b, /*sorted=*/false).to_vec<std::int64_t>(), {3});
  require_equal_i64_(a.setdiff1d(b, /*sorted=*/true).to_vec<std::int64_t>(), {3});

  require_equal_i64_(a.setxor1d(b, /*sorted=*/false).to_vec<std::int64_t>(), {3, 4});
  require_equal_i64_(a.setxor1d(b, /*sorted=*/true).to_vec<std::int64_t>(), {3, 4});
}

TEST_CASE("series general set union/intersection delegate to 1d", "[setops][series][general]") {
  const auto a = pluss::make_series(std::vector<std::int64_t>{3, 1, 3, 2}, {4});
  const auto b = pluss::make_series(std::vector<std::int64_t>{2, 4, 1}, {3});
  const auto c = pluss::make_series(std::vector<std::int64_t>{4, 2, 9}, {3});

  std::vector<pluss::Series> empty_others{};
  require_equal_i64_(
      a.unionize(pluss::msharp::RangeView::Of<pluss::Series>(empty_others), /*sorted=*/false).to_vec<std::int64_t>(),
      {3, 1, 2});

  std::vector<pluss::Series> others{b, c};
  require_equal_i64_(
      a.unionize(pluss::msharp::RangeView::Of<pluss::Series>(others), /*sorted=*/false).to_vec<std::int64_t>(),
      {3, 1, 2, 4, 9});

  require_equal_i64_(
      a.intersection(pluss::msharp::RangeView::Of<pluss::Series>(others), /*sorted=*/false, /*assume_unique=*/false)
          .to_vec<std::int64_t>(),
      {2});
}

TEST_CASE("series set ops edge contracts + failures", "[setops][series][edge]") {
  const auto empty = pluss::make_series(std::vector<std::int64_t>{}, {0});
  const auto vals = pluss::make_series(std::vector<std::int64_t>{2, 2, 3}, {3});

  require_equal_i64_(empty.union1d(vals, /*sorted=*/false).to_vec<std::int64_t>(), {2, 3});
  require_equal_i64_(empty.intersect1d(vals, /*sorted=*/false).to_vec<std::int64_t>(), {});
  require_equal_i64_(empty.setdiff1d(vals, /*sorted=*/false).to_vec<std::int64_t>(), {});
  require_equal_i64_(vals.setdiff1d(empty, /*sorted=*/false).to_vec<std::int64_t>(), {2, 3});
  require_equal_i64_(empty.setxor1d(vals, /*sorted=*/false).to_vec<std::int64_t>(), {2, 3});

  const auto d = pluss::make_series(std::vector<std::int64_t>{1, 1, 1}, {3});
  const auto e = pluss::make_series(std::vector<std::int64_t>{1, 1}, {2});
  require_equal_i64_(d.intersect1d(e, /*sorted=*/false).to_vec<std::int64_t>(), {1});

  const auto f = pluss::make_series(std::vector<double>{1.0, 2.0}, {2});
  REQUIRE_THROWS(f.union1d(vals, /*sorted=*/false));
}

TEST_CASE("series set ops along dim contracts", "[setops][series][along]") {
  const auto a = pluss::make_series(
      std::vector<std::int64_t>{
          1, 2, 3,
          4, 5, 6},
      {2, 3});
  const auto b0 = pluss::make_series(
      std::vector<std::int64_t>{
          4, 5, 6,
          7, 8, 9},
      {2, 3});

  require_equal_i64_(
      a.unionize_along(b0, /*dim=*/0, /*sorted=*/false).to_vec<std::int64_t>(),
      {1, 2, 3, 4, 5, 6, 7, 8, 9});
  require_equal_i64_(
      a.intersection_along(b0, /*dim=*/0, /*sorted=*/false, /*assume_unique=*/false).to_vec<std::int64_t>(),
      {4, 5, 6});
  require_equal_i64_(
      a.setdiff_along(b0, /*dim=*/0, /*sorted=*/false, /*assume_unique=*/false).to_vec<std::int64_t>(),
      {1, 2, 3});
  require_equal_i64_(
      a.setxor_along(b0, /*dim=*/0, /*sorted=*/false, /*assume_unique=*/false).to_vec<std::int64_t>(),
      {1, 2, 3, 7, 8, 9});

  const auto b1 = pluss::make_series(
      std::vector<std::int64_t>{
          2, 7, 8,
          5, 9, 10},
      {2, 3});
  require_equal_i64_(
      a.unionize_along(b1, /*dim=*/1, /*sorted=*/false).to_vec<std::int64_t>(),
      {1, 2, 3, 7, 8, 4, 5, 6, 9, 10});
  require_equal_i64_(
      a.intersection_along(b1, /*dim=*/1, /*sorted=*/false, /*assume_unique=*/false).to_vec<std::int64_t>(),
      {2, 5});
  require_equal_i64_(
      a.setdiff_along(b1, /*dim=*/1, /*sorted=*/false, /*assume_unique=*/false).to_vec<std::int64_t>(),
      {1, 3, 4, 6});
  require_equal_i64_(
      a.setxor_along(b1, /*dim=*/1, /*sorted=*/false, /*assume_unique=*/false).to_vec<std::int64_t>(),
      {1, 3, 7, 8, 4, 6, 9, 10});

  const auto bad_shape = pluss::make_series(std::vector<std::int64_t>{1, 2, 3, 4}, {2, 2});
  REQUIRE_THROWS(a.unionize_along(bad_shape, /*dim=*/0, /*sorted=*/false));
  REQUIRE_THROWS(a.unionize_along(b0, /*dim=*/3, /*sorted=*/false));
}

TEST_CASE("series set ops along dim ND + negative dim", "[setops][series][along][nd]") {
  const auto a = pluss::make_series(
      std::vector<std::int64_t>{
          1, 2, 3,
          4, 5, 6},
      {1, 2, 3});
  const auto b = pluss::make_series(
      std::vector<std::int64_t>{
          3, 2, 7,
          4, 5, 6},
      {1, 2, 3});

  const auto u = a.unionize_along(b, /*dim=*/-1, /*sorted=*/false);
  REQUIRE(u.sizes() == std::vector<std::int64_t>{1, 2, 5});
  require_equal_i64_(u.to_vec<std::int64_t>(), {1, 2, 3, 3, 7, 4, 5, 6, 4, 6});

  const auto i = a.intersection_along(b, /*dim=*/2, /*sorted=*/false, /*assume_unique=*/false);
  REQUIRE(i.sizes() == std::vector<std::int64_t>{1, 2, 1});
  require_equal_i64_(i.to_vec<std::int64_t>(), {2, 5});

  const auto d = a.setdiff_along(b, /*dim=*/2, /*sorted=*/false, /*assume_unique=*/false);
  REQUIRE(d.sizes() == std::vector<std::int64_t>{1, 2, 2});
  require_equal_i64_(d.to_vec<std::int64_t>(), {1, 3, 4, 6});

  const auto x = a.setxor_along(b, /*dim=*/2, /*sorted=*/false, /*assume_unique=*/false);
  REQUIRE(x.sizes() == std::vector<std::int64_t>{1, 2, 4});
  require_equal_i64_(x.to_vec<std::int64_t>(), {1, 3, 3, 7, 4, 6, 4, 6});
}

TEST_CASE("series isin shape + invert", "[setops][series][isin]") {
  const auto elements = pluss::make_series(
      std::vector<std::int64_t>{1, 2, 3, 4, 5, 2},
      {2, 3});
  const auto tests = pluss::make_series(std::vector<std::int64_t>{2, 5}, {2});

  const auto mask = elements.isin(tests);
  const auto mask_i64 = mask.to(pluss::ScalarType::Int64).to_vec<std::int64_t>();
  require_equal_i64_(mask_i64, {0, 1, 0, 0, 1, 1});

  const auto inv_mask = elements.isin(tests, /*assume_unique=*/false, /*invert=*/true);
  const auto inv_i64 = inv_mask.to(pluss::ScalarType::Int64).to_vec<std::int64_t>();
  require_equal_i64_(inv_i64, {1, 0, 1, 1, 0, 0});
}
