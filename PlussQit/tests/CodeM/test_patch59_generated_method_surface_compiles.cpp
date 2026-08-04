#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("generated torch method wrapper compiles", "[gen][surface]") {
  auto x = pluss::series::zeros({2,2}, pluss::config({{"dtype","float32"}}));
  // exists in Torch YAML as a method name, may throw at runtime if not implemented.
  // We just ensure it compiles and links.
  REQUIRE(x.abs().numel() == 4);
}
