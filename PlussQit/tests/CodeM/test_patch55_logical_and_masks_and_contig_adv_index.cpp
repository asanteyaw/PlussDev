#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;


TEST_CASE("logical_and/or/xor broadcasting", "[logical]") {
  pluss::Series a = pluss::make_series(std::vector<bool>{true,false,true,false}, {2,2});
  pluss::Series b = pluss::make_series(std::vector<bool>{true,true,false,false}, {2,2});
  auto aand = a.logical_and(b);
  auto aor  = a.logical_or(b);
  auto axor = a.logical_xor(b);
  REQUIRE(aand.scalar_type() == pluss::ScalarType::Bool);
  REQUIRE(aor.scalar_type() == pluss::ScalarType::Bool);
  REQUIRE(axor.scalar_type() == pluss::ScalarType::Bool);
  REQUIRE(aand.numel() == 4);
}

TEST_CASE("masked_select and masked_fill", "[mask]") {
  pluss::Series x = pluss::make_series(std::vector<int>{1,2,3,4,5,6}, {2,3});
  pluss::Series m = pluss::make_series(std::vector<bool>{true,false,true,false,false,true}, {2,3});
  auto sel = x.masked_select(m);
  REQUIRE(sel.dim() == 1);

  auto y = x.masked_fill(m, 9.0);
  REQUIRE(y.numel() == 6);

  x.masked_fill_(m, 7.0);
  REQUIRE(x.numel() == 6);
}

TEST_CASE("contiguous advanced indices keep position (Torch-ish)", "[indexing]") {
  auto x = pluss::zeros({2,3,4}, {{"dtype","float32"}});
  // advanced block on dims 1 and 2 (contiguous), broadcast to (2,3)
  auto i1 = pluss::make_series(std::vector<std::int64_t>{0,1}, {2,1});
  auto i2 = pluss::make_series(std::vector<std::int64_t>{1,2,3}, {1,3});
  // indices: : , i1, i2
  auto y = x.index({pluss::Series{}, i1, i2});
  // Torch-ish: output dims should be [dim0, adv_shape] => (2,2,3)
  REQUIRE(y.dim() == 3);
  REQUIRE(y.sizes()[0] == 2);
  REQUIRE(y.sizes()[1] == 2);
  REQUIRE(y.sizes()[2] == 3);
}
