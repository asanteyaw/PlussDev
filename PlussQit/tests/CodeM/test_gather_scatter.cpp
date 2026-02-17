#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>


TEST_CASE("gather and scatter", "[indexing]") {
  auto x = Series::zeros({2,3}, ScalarType::Float64);
  auto* xp = reinterpret_cast<double*>(x.raw_data());
  // [[1,2,3],[4,5,6]]
  for (int i=0;i<6;++i) xp[i] = double(i+1);

  // gather along dim=1 with indices
  auto idx = Series::zeros({2,2}, ScalarType::Int64);
  auto* ip = reinterpret_cast<std::int64_t*>(idx.raw_data());
  // take cols [2,0] for each row
  ip[0]=2; ip[1]=0;
  ip[2]=2; ip[3]=0;

  auto g = gather(x, 1, idx);
  REQUIRE(g.sizes()[0] == 2);
  REQUIRE(g.sizes()[1] == 2);
  auto* gp = reinterpret_cast<const double*>(g.raw_data());
  // row0: [3,1], row1: [6,4]
  REQUIRE(gp[0] == 3.0);
  REQUIRE(gp[1] == 1.0);
  REQUIRE(gp[2] == 6.0);
  REQUIRE(gp[3] == 4.0);

  // scatter back into a copy of x using src=g, writing into cols [1,1] (overwrite)
  auto idx2 = Series::zeros({2,2}, ScalarType::Int64);
  auto* i2 = reinterpret_cast<std::int64_t*>(idx2.raw_data());
  i2[0]=1; i2[1]=1;
  i2[2]=1; i2[3]=1;

  auto y = scatter(x, 1, idx2, g);
  auto* yp = reinterpret_cast<const double*>(y.raw_data());
  // col 1 becomes last written value per position; since both map to same col, second write overwrites.
  // row0 col1 overwritten by g[0,1]=1 ; row1 col1 overwritten by g[1,1]=4
  REQUIRE(yp[0] == 1.0);
  REQUIRE(yp[1] == 1.0);
  REQUIRE(yp[2] == 3.0);
  REQUIRE(yp[3] == 4.0);
  REQUIRE(yp[4] == 4.0);
  REQUIRE(yp[5] == 6.0);
}
