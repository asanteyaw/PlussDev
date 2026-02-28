#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;


TEST_CASE("index_put_ non-leading + mixed bool/int", "[indexing]") {
  // a shape [2,3,4]
  pluss::Series a = pluss::arange(0, 24, 1, pluss::ScalarType::Int64).view(std::vector<std::int64_t>{2,3,4});
  // bool mask on dim 1 (size 3): select rows 0 and 2
  pluss::Series m = pluss::make_series(std::vector<bool>{true,false,true}, {3});
  // int index on dim 2: pick element 1 for all
  pluss::Series j = pluss::full({1}, 1, pluss::ScalarType::Int64);

  // indices: [slice, boolmask, int]
  std::vector<pluss::Series> idx = { pluss::Series{}, m, j };
  pluss::Series v = pluss::full({2,2,4}, 100, pluss::ScalarType::Int64); // broadcasted head_total=2 (mask nnz) and j has shape [1]
  // use free function wrapper: index_put (generated) should call internal
  pluss::Series out = pluss::index_put(a, idx, v, false);
  REQUIRE(out.sizes()[0] == 2);
  REQUIRE(out.sizes()[1] == 3);
  REQUIRE(out.sizes()[2] == 4);
}

TEST_CASE("scatter_reduce include_self/initial", "[scatter][reduce]") {
  pluss::Series self = pluss::full({2,3}, 5, pluss::ScalarType::Int64);
  pluss::Series src  = pluss::full({2,3}, 1, pluss::ScalarType::Int64);
  pluss::Series idx  = pluss::full({2,3}, 0, pluss::ScalarType::Int64);

  // include_self=false, initial=0 -> should yield sums only from src
  pluss::Series out = self.scatter_reduce_sum(1, idx, src, /*include_self=*/false, /*initial=*/0.0);
  const auto* p = reinterpret_cast<const std::int64_t*>(out.raw_data());
  REQUIRE(p[0] == 3);
  REQUIRE(p[3] == 3);
}
