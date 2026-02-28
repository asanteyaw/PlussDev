#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;


TEST_CASE("non-contiguous advanced indices: moved to front, no permute bug", "[indexing]") {
  auto x = pluss::zeros({2,3,4}, {{"dtype","float32"}});
  auto i0 = pluss::make_series(std::vector<std::int64_t>{0,1}, {2,1}); // adv dim 0
  auto i2 = pluss::make_series(std::vector<std::int64_t>{1,2,3}, {1,3}); // adv dim 2 (non-contig)
  auto y = x.index({i0, pluss::Series{}, i2});
  // Torch-ish: advanced dims moved to front => (2,3, dim1)?? here adv broadcast is (2,3), remaining dim1 size=3 => (2,3,3)
  REQUIRE(y.dim() == 3);
  REQUIRE(y.sizes()[0] == 2);
  REQUIRE(y.sizes()[1] == 3);
  REQUIRE(y.sizes()[2] == 3);
}
