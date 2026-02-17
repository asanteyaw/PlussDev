#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>


TEST_CASE("matmul 2D", "[matmul]") {
  auto a = Series::zeros({2,3}, ScalarType::Float64);
  auto b = Series::zeros({3,2}, ScalarType::Float64);

  // a = [[1,2,3],[4,5,6]]
  auto* ap = reinterpret_cast<double*>(a.raw_data());
  ap[0]=1; ap[1]=2; ap[2]=3;
  ap[3]=4; ap[4]=5; ap[5]=6;

  // b = [[7,8],[9,10],[11,12]]
  auto* bp = reinterpret_cast<double*>(b.raw_data());
  bp[0]=7; bp[1]=8;
  bp[2]=9; bp[3]=10;
  bp[4]=11; bp[5]=12;

  auto c = matmul(a, b);
  REQUIRE(c.sizes()[0] == 2);
  REQUIRE(c.sizes()[1] == 2);
  auto* cp = reinterpret_cast<const double*>(c.raw_data());
  // expected [[58,64],[139,154]]
  REQUIRE(cp[0] == 58.0);
  REQUIRE(cp[1] == 64.0);
  REQUIRE(cp[2] == 139.0);
  REQUIRE(cp[3] == 154.0);
}
