#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;


TEST_CASE("advanced index tensors not broadcastable throws", "[indexing][fail]") {
  auto x = pluss::series::zeros({3,4,5}, pluss::config({{"dtype","float32"}}));
  // (2,2) and (2,3) are not broadcast-compatible.
  auto i0 = pluss::make_series(std::vector<std::int64_t>{0,1,1,2}, {2,2});
  auto i2 = pluss::make_series(std::vector<std::int64_t>{1,2,3, 0,1,2}, {2,3});
  REQUIRE_THROWS(x.index({i0, i2, pluss::Series{}}));
}
