#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("scatter_reduce allows different size along dim", "[scatter]") {
  auto x = pluss::zeros({2,5}, pluss::config({{"dtype","float32"}}));
  // index has same outer dim but smaller along dim=1
  auto index = pluss::make_series(std::vector<std::int64_t>{0,1,2,  2,1,0}, {2,3});
  auto src = pluss::make_series(std::vector<float>{1,2,3,  4,5,6}, {2,3});
  auto out = x.scatter_add(1, index, src);
  REQUIRE(out.sizes()[0] == 2);
  REQUIRE(out.sizes()[1] == 5);
}

TEST_CASE("scatter supports broadcasting src", "[scatter]") {
  auto x = pluss::zeros({2,4}, pluss::config({{"dtype","float32"}}));
  auto index = pluss::make_series(std::vector<std::int64_t>{0,1,2,3,  0,1,2,3}, {2,4});
  // src broadcast: shape {1,4}
  auto src = pluss::make_series(std::vector<float>{1,2,3,4}, {1,4});
  auto out = x.scatter_add(1, index, src);
  REQUIRE(out.sizes()[0] == 2);
  REQUIRE(out.sizes()[1] == 4);
}
