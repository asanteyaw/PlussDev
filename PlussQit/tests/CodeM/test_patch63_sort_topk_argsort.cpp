#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("sort/argsort/topk basic parity", "[sort][topk]") {
  auto x = pluss::make_series(std::vector<std::int64_t>{3,1,2,0}, {4});
  auto vals = x.sort(/*dim=*/0, /*descending=*/false, /*stable=*/false);
  REQUIRE(vals.to_vec<std::int64_t>() == std::vector<std::int64_t>{0,1,2,3});

  auto aidx = x.argsort(0, false, false);
  REQUIRE(aidx.numel() == 4);

  auto tv = x.topk(2, 0, /*largest=*/true, /*sorted=*/true);
  REQUIRE(tv.numel() == 2);
}
