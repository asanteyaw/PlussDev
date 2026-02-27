#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("mixed 1D bool mask + integer advanced indices", "[indexing]") {
  auto x = pluss::arange(0, 12, 1, {{"dtype","float32"}}).reshape({3,4});
  auto mask = pluss::make_series(std::vector<bool>{true,false,true}, {3});
  auto idx  = pluss::make_series(std::vector<std::int64_t>{3,1}, {2});
  // x[mask, idx] => mask selects rows (2 rows), idx selects cols (2) => broadcast => (2,2)
  auto y = x.index({mask, idx});
  REQUIRE(y.dim() == 2);
  REQUIRE(y.sizes()[0] == 2);
  REQUIRE(y.sizes()[1] == 2);
}

TEST_CASE("index_put_ mixed bool/int uses torch-like path", "[indexing]") {
  auto x = pluss::zeros({3,4}, {{"dtype","float32"}});
  auto mask = pluss::make_series(std::vector<bool>{true,false,true}, {3});
  auto idx  = pluss::make_series(std::vector<std::int64_t>{1,2}, {2});
  auto vals = pluss::full({2,2}, 5.0, {{"dtype","float32"}});
  x.index_put_({mask, idx}, vals, /*accumulate=*/false);
  auto y = x.index({mask, idx});
  REQUIRE(y.numel() == 4);
}
