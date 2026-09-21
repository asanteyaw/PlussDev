#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("mixed 1D bool mask + integer advanced indices", "[indexing]") {
  auto x = pluss::series::arange(0, 12, 1, pluss::config({{"dtype","float32"}})).reshape({3,4});
  auto mask = pluss::make_series(std::vector<bool>{true,false,true}, {3});
  auto idx  = pluss::make_series(std::vector<std::int64_t>{3,1}, {2});
  // x[mask, idx]: mask (shape (3,)) supplies an implicit nonzero-based
  // advanced index of shape (2,) (its True count); idx is itself shape (2,).
  // Two advanced indices broadcast ELEMENTWISE against each other (standard
  // NumPy/torch advanced-indexing rule), not against each other's *counts*
  // as separate axes -- so the result is a single (2,) vector pairing
  // (row 0, col 3) and (row 2, col 1), not a (2,2) outer-product block.
  // Verified against real torch: `x[mask, idx]` on this exact data gives
  // `tensor([3., 9.])`, shape (2,) -- confirmed this file's original
  // (2,2) expectation was simply wrong, not a Pluss bug, before touching it.
  auto y = x.index({mask, idx});
  REQUIRE(y.dim() == 1);
  REQUIRE(y.sizes()[0] == 2);
  REQUIRE(y.to_vec<float>() == std::vector<float>{3.0f, 9.0f});
}

TEST_CASE("index_put_ mixed bool/int uses torch-like path", "[indexing]") {
  auto x = pluss::series::zeros({3,4}, pluss::config({{"dtype","float32"}}));
  auto mask = pluss::make_series(std::vector<bool>{true,false,true}, {3});
  auto idx  = pluss::make_series(std::vector<std::int64_t>{1,2}, {2});
  // The broadcast target x[mask, idx] is a (2,) vector (see the test above),
  // so `vals` must be shape (2,) too, not (2,2) -- the original (2,2)/4-
  // element `vals` was the same broadcast misunderstanding as the test
  // above's (2,2) expectation. Verified against real torch:
  // x.index_put_((mask, idx), tensor([5.,5.])) sets x[0,1] and x[2,2] to 5,
  // leaving the rest 0.
  auto vals = pluss::series::full({2}, 5.0, pluss::config({{"dtype","float32"}}));
  x.index_put_({mask, idx}, vals, /*accumulate=*/false);
  auto y = x.index({mask, idx});
  REQUIRE(y.dim() == 1);
  REQUIRE(y.sizes()[0] == 2);
  REQUIRE(y.to_vec<float>() == std::vector<float>{5.0f, 5.0f});
  REQUIRE(x.pos(0, 1).to_vec<float>() == std::vector<float>{5.0f});
  REQUIRE(x.pos(2, 2).to_vec<float>() == std::vector<float>{5.0f});
  REQUIRE(x.sum().to_vec<float>() == std::vector<float>{10.0f});  // nothing else touched
}
