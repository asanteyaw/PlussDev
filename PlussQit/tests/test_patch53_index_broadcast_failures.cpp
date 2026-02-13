#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("advanced index tensors not broadcastable throws", "[indexing][fail]") {
  auto x = pluss::zeros({3,4,5}, {{"dtype","float32"}});
  auto i0 = pluss::value(std::vector<std::int64_t>{0,1}, {2,1});   // shape (2,1)
  auto i1 = pluss::value(std::vector<std::int64_t>{1,2}, {2,2});   // shape (2,2) not broadcastable with (2,1) for right align? actually is broadcastable -> result (2,2)
  // make a truly incompatible: (2,3)
  auto i2 = pluss::value(std::vector<std::int64_t>{1,2,3, 0,1,2}, {2,3});
  REQUIRE_THROWS(x.index({i0, i2, pluss::Series{}}));
}
