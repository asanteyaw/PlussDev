#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("index_put_ multiple advanced tensors broadcast mixed shapes", "[indexing]") {
  pluss::Series a = pluss::zeros({4,5,6}, pluss::ScalarType::Int64);

  pluss::Series i0 = pluss::value(std::vector<std::int64_t>{0,3}, {2,1}); // -> [2,1]
  pluss::Series i2 = pluss::value(std::vector<std::int64_t>{1,2,4}, {1,3}); // -> [1,3]

  std::vector<pluss::Series> idx = { i0, pluss::Series{}, i2 };
  pluss::Series v = pluss::full({2,3,5}, 7, pluss::ScalarType::Int64);
  pluss::Series out = pluss::index_put(a, idx, v, false);

  REQUIRE(out.sizes()[0] == 4);
  REQUIRE(out.sizes()[1] == 5);
  REQUIRE(out.sizes()[2] == 6);
}

TEST_CASE("index_put_ multidim bool mask inside indices vector", "[indexing]") {
  pluss::Series a = pluss::zeros({2,3,4}, pluss::ScalarType::Int64);

  pluss::Series m = pluss::full({3,4}, 0, pluss::ScalarType::Bool);
  auto* mp = reinterpret_cast<bool*>(m.raw_data());
  mp[0] = true;
  mp[5] = true;

  std::vector<pluss::Series> idx = { pluss::Series{}, m };
  pluss::Series v = pluss::full({2,2}, 9, pluss::ScalarType::Int64); // head=2, tail dim0=2
  pluss::Series out = pluss::index_put(a, idx, v, false);

  REQUIRE(out.numel() == a.numel());
}

TEST_CASE("index_put_ overlap without accumulate throws", "[indexing][fail]") {
  pluss::Series a = pluss::zeros({5}, pluss::ScalarType::Int64);
  pluss::Series i = pluss::value(std::vector<std::int64_t>{1,1}, {2});
  std::vector<pluss::Series> idx = { i };
  pluss::Series v = pluss::full({2}, 3, pluss::ScalarType::Int64);
  REQUIRE_THROWS_AS(pluss::index_put(a, idx, v, false), pluss::codem::Error);
}

TEST_CASE("index_put_ out of bounds throws", "[indexing][fail]") {
  pluss::Series a = pluss::zeros({2}, pluss::ScalarType::Int64);
  pluss::Series i = pluss::value(std::vector<std::int64_t>{2}, {1});
  std::vector<pluss::Series> idx = { i };
  pluss::Series v = pluss::full({1}, 1, pluss::ScalarType::Int64);
  REQUIRE_THROWS_AS(pluss::index_put(a, idx, v, false), pluss::codem::Error);
}
