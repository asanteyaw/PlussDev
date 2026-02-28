#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("invoke_any supports more multi-arg and binary ops", "[invoke][surface]") {
  auto x = pluss::arange(0, 12, 1, pluss::config({{"dtype","float32"}})).reshape({3,4});
  auto y = x.index_select(1, pluss::make_series(std::vector<std::int64_t>{1, 2}, {2})); // cols 1..2
  REQUIRE(y.sizes()[0] == 3);
  REQUIRE(y.sizes()[1] == 2);

  auto s = x.select(0, 1);
  REQUIRE(s.dim() == 1);
  REQUIRE(s.sizes()[0] == 4);

  auto a = pluss::full({3,4}, 2.0, pluss::config({{"dtype","float32"}}));
  auto m = x.maximum(a);
  REQUIRE(m.numel() == 12);

  auto r = x.remainder(a);
  REQUIRE(r.numel() == 12);

  auto t = x.atan2(a);
  REQUIRE(t.numel() == 12);

  auto c = x.clamp_min(1.0).clamp_max(10.0);
  REQUIRE(c.numel() == 12);
}
