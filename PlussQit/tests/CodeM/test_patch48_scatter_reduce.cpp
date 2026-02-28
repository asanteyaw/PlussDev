#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("scatter_add basic", "[scatter]") {
  auto a = pluss::zeros({2,3}, pluss::config({{"dtype","float32"}}));
  auto index = pluss::make_series(std::vector<std::int64_t>{0,1,2,  2,1,0}, {2,3});
  auto src = pluss::make_series(std::vector<float>{1,2,3,  4,5,6}, {2,3});
  auto out = a.scatter_add(1, index, src);
  REQUIRE(out.sizes()[0] == 2);
  REQUIRE(out.sizes()[1] == 3);
}

TEST_CASE("scatter_reduce amax", "[scatter]") {
  auto a = pluss::zeros({1,4}, pluss::config({{"dtype","float32"}}));
  auto index = pluss::make_series(std::vector<std::int64_t>{0,0,1,1}, {1,4});
  auto src = pluss::make_series(std::vector<float>{1,7,3,5}, {1,4});
  auto out = a.scatter_reduce(1, index, src, "amax", false);
  REQUIRE(out.sizes()[1] == 4);
}
