#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

#ifdef PLUSS_ENABLE_TORCH
#include <pluss/codem/torch_interop.h>
#endif

TEST_CASE("torch interop header compiles", "[interop]") {
  REQUIRE(true);
}
