#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>
#include <type_traits>

using pluss::codem::Series;
using pluss::codem::ScalarType;


// Ensure factories are in pluss namespace: global ::value should not be found by ADL here.
// We can't directly test absence of ::value easily, but we can at least ensure pluss::value exists
// and returns expected dtype for integral/bool initlists.

TEST_CASE("pluss::value dtype deduction for initializer_list", "[factories][dtype]") {
  auto a = pluss::make_series({{1,2},{3,4}}); // all ints -> Int64 by policy
  REQUIRE(a.scalar_type() == pluss::ScalarType::Int64);

  auto b = pluss::make_series({true, false, true}); // bools -> Bool
  REQUIRE(b.scalar_type() == pluss::ScalarType::Bool);

  auto c = pluss::make_series({1.0, 2.0, 3.0}); // doubles -> Float64
  REQUIRE(c.scalar_type() == pluss::ScalarType::Float64);

  auto d = pluss::make_series({1.0f, 2.0f}); // floats -> Float32
  REQUIRE(d.scalar_type() == pluss::ScalarType::Float32);

  // explicit override should promote/cast
  auto e = pluss::make_series({{1,2},{3,4}}, pluss::ScalarType::Float32);
  REQUIRE(e.scalar_type() == pluss::ScalarType::Float32);
}

TEST_CASE("where/nonzero basic routing", "[where][nonzero]") {
  auto cond = pluss::make_series({true,false,true});
  auto x = pluss::make_series({1,2,3}, pluss::ScalarType::Int64);
  auto y = pluss::make_series({10,20,30}, pluss::ScalarType::Int64);
  auto out = cond.where(x, y);
  REQUIRE(out.numel() == 3);

  auto nz = out.nonzero();
  REQUIRE(nz.scalar_type() == pluss::ScalarType::Int64);
}
