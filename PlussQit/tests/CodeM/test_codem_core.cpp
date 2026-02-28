#include <catch2/catch_test_macros.hpp>
#include "CodeM/core/SeriesBody.h"

using pluss::codem::Series;
using pluss::codem::ScalarType;



TEST_CASE("CodeM factories + views + contiguous", "[codem]") {
  auto a = Series::zeros({2,3}, ScalarType::Float64);
  REQUIRE(a.defined());
  REQUIRE(a.dim() == 2);
  REQUIRE(a.is_contiguous());

  auto t = a.transpose(0,1);
  REQUIRE_FALSE(t.is_contiguous());

  auto c = t.contiguous();
  REQUIRE(c.is_contiguous());
  REQUIRE(c.sizes()[0] == 3);
  REQUIRE(c.sizes()[1] == 2);
}
