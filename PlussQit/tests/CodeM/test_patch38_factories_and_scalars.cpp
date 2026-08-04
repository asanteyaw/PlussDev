#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;


TEST_CASE("linspace and eye factories", "[factories]") {
  auto x = pluss::series::linspace(0.0, 1.0, 5, pluss::config({{"dtype","float32"}}));
  REQUIRE(x.sizes()[0] == 5);
  REQUIRE(x.scalar_type() == pluss::ScalarType::Float32);

  auto I = pluss::series::eye(3, -1, pluss::config({{"dtype","float64"}}));
  REQUIRE(I.sizes()[0] == 3);
  REQUIRE(I.sizes()[1] == 3);
  REQUIRE(I.scalar_type() == pluss::ScalarType::Float64);
}

TEST_CASE("like factories", "[factories]") {
  auto a = pluss::series::zeros({2,3}, pluss::ScalarType::Int16);
  auto b = pluss::series::ones_like(a, pluss::config({{"dtype","int16"}}));
  REQUIRE(b.scalar_type() == pluss::ScalarType::Int16);
  REQUIRE(b.sizes()[0] == 2);
  REQUIRE(b.sizes()[1] == 3);
}

TEST_CASE("scalar overloads for add/sub/mul/div/pow and comparisons", "[methods]") {
  auto a = pluss::series::full({2,2}, 2.0, pluss::ScalarType::Float32);
  auto b = a.add(3);     // int scalar
  auto c = b.mul(2.5f);  // float scalar
  auto d = c.div(2u);    // unsigned scalar
  REQUIRE(d.scalar_type() == pluss::ScalarType::Float32);

  auto m = d.gt(1);      // scalar compare int
  REQUIRE(m.scalar_type() == pluss::ScalarType::Bool);
}
