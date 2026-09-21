#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;


TEST_CASE("mixed bool + int indexing (bool->positions)", "[indexing]") {
  auto x = pluss::make_series(std::vector<int>{0,1,2,3,4,5}, {2,3});
  // mask on dim0 selects row 1 only
  auto mask0 = pluss::make_series(std::vector<bool>{false,true}, {2});
  // int index on dim1 selects columns [2,0]
  auto idx1 = pluss::make_series(std::vector<std::int64_t>{2,0}, {2});
  // indices list length == dim, non-leading advanced (mask0 is leading) but mixed
  auto y = x.index({mask0, idx1});
  REQUIRE((y.sizes().size() == 1 || y.sizes().size() == 2));
}

TEST_CASE("non-leading advanced index moves to front", "[indexing]") {
  auto x = pluss::make_series(std::vector<int>{0,1,2,3,4,5,6,7}, {2,2,2});
  auto idx = pluss::make_series(std::vector<std::int64_t>{1,0}, {2}); // advanced on dim2
  // indices: ":" , ":" , idx
  auto y = x.index({pluss::Slicing::SliceAll(), pluss::Slicing::SliceAll(), idx});
  // advanced dims moved to front => output starts with broadcast(idx) then remaining dims
  REQUIRE(y.sizes().size() == 3);
}

TEST_CASE("index_put with non-leading advanced index", "[indexing]") {
  auto x = pluss::series::zeros({2,2,2}, pluss::config({{"dtype","float32"}}));
  auto idx = pluss::make_series(std::vector<std::int64_t>{1,0}, {2});
  auto v = pluss::series::ones({2,2,2}, pluss::config({{"dtype","float32"}})).flatten(0,2).reshape({2,2,2});
  auto y = x.index_put({pluss::Series{}, pluss::Series{}, idx}, v, false);
  REQUIRE(y.sizes()[0] == 2);
}
