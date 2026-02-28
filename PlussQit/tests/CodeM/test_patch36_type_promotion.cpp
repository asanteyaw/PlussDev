#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;


TEST_CASE("type promotion: floats and ints", "[promotion]") {
  REQUIRE(pluss::codem::promote_types(pluss::ScalarType::Int32, pluss::ScalarType::Float32) == pluss::ScalarType::Float32);
  REQUIRE(pluss::codem::promote_types(pluss::ScalarType::Float16, pluss::ScalarType::BFloat16) == pluss::ScalarType::Float32);
  REQUIRE(pluss::codem::promote_types(pluss::ScalarType::Float8E4M3FN, pluss::ScalarType::Float16) == pluss::ScalarType::Float16);
  REQUIRE(pluss::codem::promote_types(pluss::ScalarType::Float8E4M3FN, pluss::ScalarType::BFloat16) == pluss::ScalarType::Float32);
}

TEST_CASE("type promotion: complex", "[promotion]") {
  REQUIRE(pluss::codem::promote_types(pluss::ScalarType::Complex64, pluss::ScalarType::Float32) == pluss::ScalarType::Complex64);
  REQUIRE(pluss::codem::promote_types(pluss::ScalarType::Complex64, pluss::ScalarType::Float64) == pluss::ScalarType::Complex128);
  REQUIRE(pluss::codem::promote_types(pluss::ScalarType::Complex128, pluss::ScalarType::Int64) == pluss::ScalarType::Complex128);
}

TEST_CASE("type promotion: mixed signed/unsigned", "[promotion]") {
  REQUIRE(pluss::codem::promote_types(pluss::ScalarType::Int32, pluss::ScalarType::UInt32) == pluss::ScalarType::Int64);
  REQUIRE(pluss::codem::promote_types(pluss::ScalarType::UInt8, pluss::ScalarType::Int16) == pluss::ScalarType::Int16);
}
