#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;


TEST_CASE("multidim bool mask wrong shape throws", "[indexing][fail]") {
  auto a = pluss::zeros({2,3,4}, pluss::ScalarType::Int64);
  auto m = pluss::full({3,5}, 0, pluss::ScalarType::Bool); // mismatch (3,5) vs (3,4)
  std::vector<pluss::Series> idx = { pluss::Series{}, m };
  auto v = pluss::full({2,2}, 1, pluss::ScalarType::Int64);
  REQUIRE_THROWS_AS(pluss::index_put(a, idx, v, false), pluss::codem::Error);
}

TEST_CASE("bool mask ndim block exceeding rank throws", "[indexing][fail]") {
  auto a = pluss::zeros({2,3}, pluss::ScalarType::Int64);
  auto m = pluss::full({2,3,1}, 0, pluss::ScalarType::Bool);
  std::vector<pluss::Series> idx = { m }; // rank3 > rank2, should fail
  auto v = pluss::full({0}, 1, pluss::ScalarType::Int64);
  REQUIRE_THROWS_AS(pluss::index_put(a, idx, v, false), pluss::codem::Error);
}

TEST_CASE("values wrong shape throws", "[indexing][fail]") {
  auto a = pluss::zeros({4,5,6}, pluss::ScalarType::Int64);
  auto i0 = pluss::make_series(std::vector<std::int64_t>{0,3}, {2,1});
  auto i2 = pluss::make_series(std::vector<std::int64_t>{1,2,4}, {1,3});
  std::vector<pluss::Series> idx = { i0, pluss::Series{}, i2 };
  auto v_bad = pluss::full({2,3,4}, 7, pluss::ScalarType::Int64); // tail should be 5 not 4
  REQUIRE_THROWS_AS(pluss::index_put(a, idx, v_bad, false), pluss::codem::Error);
}
