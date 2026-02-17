#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("torch-compat invoke_any supports list/int/bool arguments", "[invoke]") {
  auto x = pluss::arange(0, 12, 1, {{"dtype","float32"}}).reshape({3,4});
  auto y = x.transpose(0,1);
  REQUIRE(y.sizes()[0] == 4);
  REQUIRE(y.sizes()[1] == 3);

  auto z = x.reshape({2,6});
  REQUIRE(z.sizes()[0] == 2);
  REQUIRE(z.sizes()[1] == 6);

  auto s = x.sum(1, true);
  REQUIRE(s.sizes()[0] == 3);
  REQUIRE(s.sizes()[1] == 1);
}
