#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;


TEST_CASE("type promotion: float16 + bfloat16 => float32", "[promo]") {
  auto a = pluss::zeros({2}, pluss::config({{"dtype","float16"}}));
  auto b = pluss::zeros({2}, pluss::config({{"dtype","bfloat16"}}));
  auto c = a.add(b);
  REQUIRE(c.scalar_type() == pluss::ScalarType::Float32);
}

TEST_CASE("type promotion: complex + half => complex", "[promo]") {
  auto a = pluss::zeros({2}, pluss::config({{"dtype","complex64"}}));
  auto b = pluss::zeros({2}, pluss::config({{"dtype","float16"}}));
  auto c = a.mul(b);
  REQUIRE(c.scalar_type() == pluss::ScalarType::Complex64);
}
