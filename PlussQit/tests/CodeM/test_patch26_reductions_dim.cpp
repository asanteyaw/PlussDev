#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;



TEST_CASE("sum/mean dim reductions", "[reduce]") {
  Series x = pluss::arange(0, 6, 1, ScalarType::Float32).view(std::vector<std::int64_t>{2,3});
  auto s0 = x.sum(0);
  REQUIRE(s0.dim() == 1);
  auto* p0 = reinterpret_cast<const float*>(s0.raw_data());
  REQUIRE(p0[0] == 3.0f);

  auto s1 = x.sum(1);
  auto* p1 = reinterpret_cast<const float*>(s1.raw_data());
  REQUIRE(p1[0] == 3.0f);

  auto m1 = x.mean(1);
  auto* pm = reinterpret_cast<const float*>(m1.raw_data());
  REQUIRE(pm[0] == 1.0f);
}
