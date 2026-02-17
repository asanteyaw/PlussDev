#include <catch2/catch_test_macros.hpp>
#include "CodeM/core/SeriesBody.h"


TEST_CASE("CodeM broadcast add", "[codem]") {
  auto a = Series::empty({2,3}, ScalarType::Float64);
  auto b = Series::zeros({1,3}, ScalarType::Float64);

  auto* ap = reinterpret_cast<double*>(a.raw_data());
  for (std::int64_t i = 0; i < a.numel(); ++i) ap[i] = double(i + 1);

  auto c = a + b;
  REQUIRE(c.sizes()[0] == 2);
  REQUIRE(c.sizes()[1] == 3);

  auto* cp = reinterpret_cast<const double*>(c.raw_data());
  REQUIRE(cp[0] == 1.0);
  REQUIRE(cp[5] == 6.0);
}
