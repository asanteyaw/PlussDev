#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>


TEST_CASE("masked_select basic", "[indexing]") {
  auto a = Series::zeros({2,3}, ScalarType::Float64);
  auto m = Series::zeros({2,3}, ScalarType::Bool);

  auto* ap = reinterpret_cast<double*>(a.raw_data());
  for (std::int64_t i=0;i<a.numel();++i) ap[i] = double(i+1);

  auto* mp = reinterpret_cast<std::uint8_t*>(m.raw_data());
  // select elements 2,4,6
  mp[1] = 1; mp[3] = 1; mp[5] = 1;

  auto out = masked_select(a, m);
  REQUIRE(out.dim() == 1);
  REQUIRE(out.numel() == 3);
  auto* op = reinterpret_cast<const double*>(out.raw_data());
  REQUIRE(op[0] == 2.0);
  REQUIRE(op[1] == 4.0);
  REQUIRE(op[2] == 6.0);
}
