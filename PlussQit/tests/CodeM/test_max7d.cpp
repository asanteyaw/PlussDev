#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#include <cstdint>
#include <vector>

using pluss::codem::Series;
using pluss::codem::ScalarType;

// The rank limit is 64 (SeriesImpl.h kMaxSeriesRank), which higher-order
// derivative tensors rely on.
TEST_CASE("max rank is enforced", "[shape]") {
  REQUIRE_NOTHROW(Series::zeros(std::vector<std::int64_t>(7, 1), ScalarType::Float32));
  REQUIRE_NOTHROW(Series::zeros(std::vector<std::int64_t>(64, 1), ScalarType::Float32));
  REQUIRE_THROWS_AS(Series::zeros(std::vector<std::int64_t>(65, 1), ScalarType::Float32), pluss::Error);
}
