#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("Generated methods do not throw NotImplemented (fallback is allowed)", "[invoke][no-throw]") {
  auto x = pluss::linspace(0.0, 1.0, 8, pluss::config({{"dtype","float32"}}));
  // These may route via generated wrappers; even if not fully implemented yet,
  // they should not throw.
  REQUIRE_NOTHROW(x.log1p());
  REQUIRE_NOTHROW(x.expm1());
  REQUIRE_NOTHROW(x.sinc());
}
