#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("more generated methods route to real kernels", "[surface][kernels]") {
  auto x = pluss::linspace(0.0, 1.0, 10, {{"dtype","float32"}});
  auto y = x.asin().sin(); // asin then sin
  REQUIRE(y.numel() == 10);

  auto z = x.cosh().sinh();
  REQUIRE(z.numel() == 10);

  auto f = x.isfinite();
  REQUIRE(f.scalar_type() == pluss::ScalarType::Bool);

  auto b = pluss::value(std::vector<std::int64_t>{1,2,3,4}, {2,2});
  auto c = b.bitwise_not();
  REQUIRE(c.numel() == 4);

  auto s = x.softmax(0);
  REQUIRE(s.numel() == 10);

  auto t = x.clone();
  t.softplus_();
  REQUIRE(t.numel() == 10);
}
