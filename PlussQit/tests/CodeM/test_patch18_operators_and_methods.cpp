#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>
#include <sstream>


TEST_CASE("operators", "[ops]") {
  Series a = pluss::full({4}, 2.0, ScalarType::Float32);
  Series b = pluss::full({4}, 3.0, ScalarType::Float32);

  Series c = a + b;
  auto* cp = reinterpret_cast<const float*>(c.raw_data());
  REQUIRE(cp[0] == 5.0f);

  a += b;
  auto* ap = reinterpret_cast<const float*>(a.raw_data());
  REQUIRE(ap[0] == 5.0f);

  Series m = (a > b);
  REQUIRE(m.scalar_type() == ScalarType::Bool);
}

TEST_CASE("ostream", "[print]") {
  Series a = pluss::arange(0, 5, 1, ScalarType::Int64);
  std::ostringstream oss;
  oss << a;
  REQUIRE(oss.str().find("pluss::Series") != std::string::npos);
  REQUIRE(oss.str().find("dtype=") != std::string::npos);
}
