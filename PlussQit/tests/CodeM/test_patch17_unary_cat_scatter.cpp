#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;



TEST_CASE("unary math op and inplace", "[unary]") {
  Series x = pluss::full({4}, 1.0, ScalarType::Float32);
  Series y = x.exp();
  REQUIRE(y.scalar_type() == ScalarType::Float32);
  x.exp_();
  auto* xp = reinterpret_cast<const float*>(x.raw_data());
  REQUIRE(xp[0] > 2.7f);
}

TEST_CASE("cat/stack methods", "[catstack]") {
  Series a = pluss::arange(0, 3, 1, ScalarType::Int64); // [0,1,2]
  Series b = pluss::arange(3, 6, 1, ScalarType::Int64); // [3,4,5]
  auto c = a.unite({b}, 0);
  REQUIRE(c.numel() == 6);

  auto s = a.pile({b}, 0);
  REQUIRE(s.dim() == 2);
  REQUIRE(s.sizes()[0] == 2);
  REQUIRE(s.sizes()[1] == 3);
}

TEST_CASE("scatter_add inplace", "[scatter]") {
  Series base = pluss::zeros({5}, ScalarType::Float32);
  Series idx = pluss::zeros({4}, ScalarType::Int64);
  auto* ip = reinterpret_cast<std::int64_t*>(idx.raw_data());
  ip[0]=0; ip[1]=2; ip[2]=2; ip[3]=4;
  Series src = pluss::full({4}, 1.0, ScalarType::Float32);
  base = base.scatter_add(0, idx, src);
  auto* bp = reinterpret_cast<const float*>(base.raw_data());
  REQUIRE(bp[0] == 1.0f);
  REQUIRE(bp[2] == 2.0f);
  REQUIRE(bp[4] == 1.0f);
}
