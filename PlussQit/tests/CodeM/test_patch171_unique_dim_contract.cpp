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

TEST_CASE("unique(dim) returns expected rows for dim=0", "[unique][dim]") {
  const auto x = pluss::make_series(
      std::vector<std::int64_t>{
          3, 4,
          1, 2,
          3, 4,
          5, 6},
      {4, 2});

  const auto vals_unsorted = x.unique(/*dim=*/0, /*sorted=*/false).to_vec<std::int64_t>();
  require_equal_i64_(vals_unsorted, {3, 4, 1, 2, 5, 6});

  const auto vals_sorted = x.unique(/*dim=*/0, /*sorted=*/true).to_vec<std::int64_t>();
  require_equal_i64_(vals_sorted, {1, 2, 3, 4, 5, 6});
}

TEST_CASE("unique(dim) works for dim=1 on ND tensors", "[unique][dim][nd]") {
  const auto x = pluss::make_series(
      std::vector<std::int64_t>{
          3, 1, 3, 5,
          4, 2, 4, 6},
      {2, 4});

  const auto vals = x.unique(/*dim=*/1, /*sorted=*/true).to_vec<std::int64_t>();
  require_equal_i64_(vals, {1, 3, 5, 2, 4, 6});
}
