#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;



TEST_CASE("max 7D enforced", "[shape]") {
  // 7D should work
  REQUIRE_NOTHROW(Series::zeros({1,1,1,1,1,1,1}, ScalarType::Float32));
  // 8D should throw
  REQUIRE_THROWS_AS(Series::zeros({1,1,1,1,1,1,1,1}, ScalarType::Float32), pluss::codem::Error);
}
