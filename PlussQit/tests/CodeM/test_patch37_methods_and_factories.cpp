#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;


TEST_CASE("factory Config kv list works", "[factories]") {
  auto x = pluss::zeros({2,3}, {{"dtype","float16"}});
  REQUIRE(x.scalar_type() == pluss::ScalarType::Float16);

  auto y = pluss::full({2,3}, 1.0, {{"dtype","bfloat16"}});
  REQUIRE(y.scalar_type() == pluss::ScalarType::BFloat16);
}

TEST_CASE("pile/unite methods work", "[methods]") {
  auto a = pluss::ones({2,2}, pluss::ScalarType::Float32);
  auto b = pluss::zeros({2,2}, pluss::ScalarType::Float32);
  auto s = a.pile({b}, 0);
  REQUIRE(s.sizes()[0] == 2); // stacked along new dim: [2,2,2]
  REQUIRE(s.sizes()[1] == 2);
  REQUIRE(s.sizes()[2] == 2);

  auto c = a.unite({b}, 0);
  REQUIRE(c.sizes()[0] == 4);
  REQUIRE(c.sizes()[1] == 2);
}
