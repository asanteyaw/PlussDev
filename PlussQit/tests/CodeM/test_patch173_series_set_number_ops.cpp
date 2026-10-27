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

TEST_CASE("series comp complement surface", "[setops][series][comp]") {
  const auto a = pluss::make_series(std::vector<std::int64_t>{1, 2, 2, 4}, {4});
  const auto universe = pluss::make_series(std::vector<std::int64_t>{0, 1, 2, 3, 4}, {5});

  require_equal_i64_(a.comp(universe).host<std::int64_t>(), {0, 3});
  require_equal_i64_(a.comp(universe, /*sorted=*/true).host<std::int64_t>(), {0, 3});

  const auto universe_dup = pluss::make_series(std::vector<std::int64_t>{0, 0, 3, 3, 4}, {5});
  require_equal_i64_(
      a.comp(universe_dup, /*sorted=*/false, /*assume_unique=*/false).host<std::int64_t>(),
      {0, 3});
}

TEST_CASE("series cartprod surface", "[setops][series][cartprod]") {
  const auto a = pluss::make_series(std::vector<std::int64_t>{1, 2}, {2});
  const auto b = pluss::make_series(std::vector<std::int64_t>{3, 4, 5}, {3});

  const auto pair = a.cartprod(b);
  REQUIRE(pair.sizes() == std::vector<std::int64_t>{6, 2});
  require_equal_i64_(pair.host<std::int64_t>().array().data, {1, 3, 1, 4, 1, 5, 2, 3, 2, 4, 2, 5});

  const auto c = pluss::make_series(std::vector<std::int64_t>{9}, {1});
  std::vector<pluss::Series> others{b, c};
  const auto tri = a.cartprod(pluss::msharp::RangeView::Of<pluss::Series>(others));
  REQUIRE(tri.sizes() == std::vector<std::int64_t>{6, 3});
  require_equal_i64_(tri.host<std::int64_t>().array().data, {1, 3, 9, 1, 4, 9, 1, 5, 9, 2, 3, 9, 2, 4, 9, 2, 5, 9});

  const auto bad_dtype = pluss::make_series(std::vector<double>{1.0, 2.0}, {2});
  REQUIRE_THROWS(a.cartprod(bad_dtype));
}

TEST_CASE("series number-theory surface", "[number][series]") {
  const auto x = pluss::make_series(std::vector<std::int64_t>{-2, -1, 0, 1, 2, 3, 4, 17}, {8});
  const auto prime_mask = x.is_prime().to(pluss::ScalarType::Int64);
  require_equal_i64_(prime_mask.host<std::int64_t>(), {0, 0, 0, 0, 1, 1, 0, 1});

  const auto n = pluss::make_series(std::vector<std::int64_t>{1, 2, 3, 5}, {4});
  require_equal_i64_(n.prime().host<std::int64_t>(), {2, 3, 5, 11});

  const auto y = pluss::make_series(std::vector<std::int64_t>{0, 1, 2, 12, 49, -15}, {6});
  require_equal_i64_(y.factor().host<std::int64_t>(), {0, 1, 2, 2, 7, 3});

  const auto bad_n = pluss::make_series(std::vector<std::int64_t>{0}, {1});
  REQUIRE_THROWS(bad_n.prime());
}
