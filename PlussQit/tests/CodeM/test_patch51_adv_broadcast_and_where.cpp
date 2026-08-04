#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

using pluss::codem::Series;
using pluss::codem::ScalarType;


TEST_CASE("advanced index broadcast", "[indexing]") {
  auto x = pluss::series::zeros({3,4,5}, pluss::config({{"dtype","float32"}}));
  // Two advanced indices with broadcastable shapes: (2,1) and (1,3)
  auto i0 = pluss::make_series(std::vector<std::int64_t>{0,2}, {2,1});
  auto i1 = pluss::make_series(std::vector<std::int64_t>{1,2,3}, {1,3});
  auto y = x.index({i0, i1, pluss::Series{}});
  REQUIRE(y.dim() == 3);
}

TEST_CASE("where/clamp basics", "[eltwise]") {
  auto a = pluss::make_series(std::vector<float>{-1, 2, -3, 4}, {2,2});
  auto c = pluss::make_series(std::vector<bool>{true,false,true,false}, {2,2});
  auto b = pluss::series::zeros({2,2}, pluss::config({{"dtype","float32"}}));
  auto w = a.where(c, b);
  REQUIRE(w.numel() == 4);

  auto cl = a.clamp(-0.5, 1.5);
  REQUIRE(cl.numel() == 4);
}
