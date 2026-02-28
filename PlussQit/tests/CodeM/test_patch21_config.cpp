#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;
using pluss::Config;



TEST_CASE("Config factories dtype", "[config]") {
  Config cfg = Config{}.with_dtype(ScalarType::Int64);
  Series a = pluss::zeros({3,2}, cfg);
  REQUIRE(a.scalar_type() == ScalarType::Int64);
}
