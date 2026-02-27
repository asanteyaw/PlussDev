#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("scatter_reduce supports include_self and initial", "[scatter][reduce]") {
  // shape (1,4)
  pluss::Series self = pluss::make_series(std::vector<float>{1,2,3,4}, {1,4});
  pluss::Series idx  = pluss::make_series(std::vector<std::int64_t>{0,0,1,1}, {1,4});
  pluss::Series src  = pluss::make_series(std::vector<float>{10,10,10,10}, {1,4});

  // include_self=false, initial=5 => base=5 then add src into buckets
  auto out = self.scatter_reduce(1, idx, src, "sum", /*include_self=*/false, /*has_initial=*/true, 5.0);
  REQUIRE(out.sizes()[0] == 1);
  REQUIRE(out.sizes()[1] == 4);

  // in-place variant
  auto a = self.clone();
  a.scatter_reduce_(1, idx, src, "sum", /*include_self=*/false, /*has_initial=*/true, 0.0);
  REQUIRE(a.numel() == 4);
}
