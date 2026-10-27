#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;


TEST_CASE("index_put_ multiple advanced tensors broadcast mixed shapes", "[indexing]") {
  pluss::Series a = pluss::series::zeros({4,5,6}, pluss::ScalarType::Int64);

  pluss::Series i0 = pluss::make_series(std::vector<std::int64_t>{0,3}, {2,1}); // -> [2,1]
  pluss::Series i2 = pluss::make_series(std::vector<std::int64_t>{1,2,4}, {1,3}); // -> [1,3]

  std::vector<pluss::Series> idx = {i0, pluss::series::undefined(), i2};
  pluss::Series v = pluss::series::full({2,3,5}, 7, pluss::ScalarType::Int64);
  pluss::Series out = pluss::index_put(a, idx, v, false);

  REQUIRE(out.sizes()[0] == 4);
  REQUIRE(out.sizes()[1] == 5);
  REQUIRE(out.sizes()[2] == 6);
}

TEST_CASE("index_put_ multidim bool mask inside indices vector", "[indexing]") {
  pluss::Series a = pluss::series::zeros({2,3,4}, pluss::ScalarType::Int64);

  pluss::Series m = pluss::series::full({3,4}, 0, pluss::ScalarType::Bool);
  auto* mp = reinterpret_cast<bool*>(m.raw_data());
  mp[0] = true;
  mp[5] = true;

  std::vector<pluss::Series> idx = {pluss::series::undefined(), m};
  pluss::Series v = pluss::series::full({2,2}, 9, pluss::ScalarType::Int64); // head=2, tail dim0=2
  pluss::Series out = pluss::index_put(a, idx, v, false);

  REQUIRE(out.numel() == a.numel());
}

TEST_CASE("index_put_ overlap without accumulate is last-write-wins, matching torch", "[indexing]") {
  // Renamed from "... throws": libtorch itself does NOT throw on an
  // overlapping non-accumulate index_put_ (verified live against this
  // repo's own torch build) -- it silently resolves to whichever write
  // landed last, and so does Pluss's CPU path (see the "preserve CPU
  // last-write semantics" comment at OpsInternal.cpp's
  // has_duplicate_linear_indices_ call site, which exists to route AROUND
  // the MPS scatter primitive specifically because ITS duplicate-write
  // order is undefined, falling back to this same deterministic CPU path).
  // The original test's "throws" expectation never matched any real
  // behavior on either side -- a test-authoring bug, not a Pluss gap.
  pluss::Series a = pluss::series::zeros({5}, pluss::ScalarType::Int64);
  pluss::Series i = pluss::make_series(std::vector<std::int64_t>{1,1}, {2});
  std::vector<pluss::Series> idx = { i };
  pluss::Series v = pluss::make_series(std::vector<std::int64_t>{3,7}, {2});
  pluss::Series out = pluss::index_put(a, idx, v, false);
  // Verified against real torch: a.index_put_((i,), v, accumulate=False)
  // on this exact data resolves index 1 to 7 (the later write in the index
  // array), leaving every other position untouched.
  REQUIRE(out.host<std::int64_t>() == std::vector<std::int64_t>{0, 7, 0, 0, 0});
}

TEST_CASE("index_put_ out of bounds throws", "[indexing][fail]") {
  pluss::Series a = pluss::series::zeros({2}, pluss::ScalarType::Int64);
  pluss::Series i = pluss::make_series(std::vector<std::int64_t>{2}, {1});
  std::vector<pluss::Series> idx = { i };
  pluss::Series v = pluss::series::full({1}, 1, pluss::ScalarType::Int64);
  REQUIRE_THROWS_AS(pluss::index_put(a, idx, v, false), pluss::Error);
}
