#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>
#include <cmath>

using pluss::codem::Series;
using pluss::codem::ScalarType;


TEST_CASE("pluss::value overloads: scalar and nested initializer_list up to 7D", "[factories][value]") {
  auto a = pluss::make_series(2); // int -> Int64 scalar
  REQUIRE(a.dim() == 0);
  REQUIRE(a.scalar_type() == pluss::ScalarType::Int64);

  auto b = pluss::make_series(2.0); // double -> Float32 scalar (default)
  REQUIRE(b.dim() == 0);

  auto v1 = pluss::make_series({1,2,3}, pluss::ScalarType::Int64);
  REQUIRE(v1.dim() == 1);
  REQUIRE(v1.numel() == 3);

  auto m2 = pluss::make_series({{1,2},{3,4}}, pluss::ScalarType::Int64);
  REQUIRE(m2.dim() == 2);
  REQUIRE(m2.sizes()[0] == 2);
  REQUIRE(m2.sizes()[1] == 2);
}

TEST_CASE("bincount/histc/unique basic", "[count][hist][unique]") {
  auto x = pluss::make_series({1,1,2,3,3,3}, pluss::ScalarType::Int64);
  auto bc = x.bincount();
  REQUIRE(bc.numel() >= 4);

  auto u = x.unique(true);
  REQUIRE(u.numel() <= x.numel());

  auto f = pluss::linspace(0.0, 1.0, 100, pluss::config({{"dtype","float32"}}));
  auto h = f.histc(10, 0.0, 1.0);
  REQUIRE(h.numel() == 10);
}
