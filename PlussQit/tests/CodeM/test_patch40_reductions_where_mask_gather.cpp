#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("reductions dim variants work", "[reduce]") {
  auto a = pluss::ones({2,3,4}, pluss::ScalarType::Float32);
  auto s0 = a.sum(0);
  REQUIRE(s0.sizes()[0] == 3);
  REQUIRE(s0.sizes()[1] == 4);

  auto m1 = a.mean(1, true);
  REQUIRE(m1.sizes()[0] == 2);
  REQUIRE(m1.sizes()[1] == 1);
  REQUIRE(m1.sizes()[2] == 4);

  auto mx = a.amax(2);
  REQUIRE(mx.sizes()[0] == 2);
  REQUIRE(mx.sizes()[1] == 3);
}

TEST_CASE("where and masked_fill methods", "[where][mask]") {
  auto cond = pluss::make_series(std::vector<std::uint8_t>{1,0,1,0}, {2,2}).to(pluss::ScalarType::Bool);
  auto a = pluss::full({2,2}, 3.0, pluss::ScalarType::Float32);
  auto b = pluss::zeros({2,2}, pluss::ScalarType::Float32);
  auto out = cond.where(a,b);
  REQUIRE(out.scalar_type() == pluss::ScalarType::Float32);

  auto m = pluss::make_series(std::vector<std::uint8_t>{0,1,0,1}, {2,2}).to(pluss::ScalarType::Bool);
  auto c = out.masked_fill(m, 7.0);
  REQUIRE(c.scalar_type() == pluss::ScalarType::Float32);
}

TEST_CASE("gather method", "[gather]") {
  auto x = pluss::make_series(std::vector<std::int64_t>{10,11,12,13,20,21,22,23}, {2,4});
  auto idx = pluss::make_series(std::vector<std::int64_t>{3,0,1,2, 0,1,2,3}, {2,4});
  auto y = x.gather(1, idx);
  REQUIRE(y.sizes()[0] == 2);
  REQUIRE(y.sizes()[1] == 4);
}
