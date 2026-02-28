#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;



TEST_CASE("int8/int16/uint16 casting and arithmetic", "[dtype]") {
  auto a = Series::full({4}, 2.0, ScalarType::Int16);
  auto b = Series::full({4}, 3.0, ScalarType::Int16);
  auto c = a.add(b);
  REQUIRE(c.scalar_type() == ScalarType::Int16);
  auto* cp = reinterpret_cast<const std::int16_t*>(c.raw_data());
  REQUIRE(cp[0] == 5);

  auto u = c.to(ScalarType::UInt16);
  REQUIRE(u.scalar_type() == ScalarType::UInt16);
}

TEST_CASE("bool casting", "[dtype]") {
  auto a = Series::full({3}, 0.0, ScalarType::Float32);
  auto b = a.to(ScalarType::Bool);
  auto* bp = reinterpret_cast<const std::uint8_t*>(b.raw_data());
  REQUIRE(bp[0] == 0);
  auto c = Series::full({3}, 2.0, ScalarType::Float32).to(ScalarType::Bool);
  auto* cp = reinterpret_cast<const std::uint8_t*>(c.raw_data());
  REQUIRE(cp[1] == 1);
}

TEST_CASE("in-place add_/clamp_", "[inplace]") {
  auto a = Series::full({2,2}, 1.0, ScalarType::Float32);
  auto b = Series::full({1,2}, 2.0, ScalarType::Float32);
  a.add_(b); // broadcast rhs
  auto* ap = reinterpret_cast<const float*>(a.raw_data());
  REQUIRE(ap[0] == 3.0f);
  REQUIRE(ap[3] == 3.0f);

  a.clamp_(0.0, 2.5);
  ap = reinterpret_cast<const float*>(a.raw_data());
  REQUIRE(ap[0] == 2.5f);
}
