#include <catch2/catch_test_macros.hpp>
#include <pluss/pluss.h>

TEST_CASE("permute/unsqueeze/squeeze/flatten basics", "[views]") {
  auto x = pluss::zeros({2,3,4}, {{"dtype","float32"}});
  auto y = x.permute(std::array<std::int64_t,3>{2,0,1});
  REQUIRE(y.sizes()[0] == 4);
  REQUIRE(y.sizes()[1] == 2);
  REQUIRE(y.sizes()[2] == 3);

  auto u = x.unsqueeze(1);
  REQUIRE(u.sizes().size() == 4);
  REQUIRE(u.sizes()[1] == 1);

  auto s = u.squeeze(1);
  REQUIRE(s.sizes().size() == 3);

  auto f = x.flatten(1,2);
  REQUIRE(f.sizes().size() == 2);
  REQUIRE(f.sizes()[0] == 2);
  REQUIRE(f.sizes()[1] == 12);
}

TEST_CASE("norm p=1/2", "[norm]") {
  auto x = pluss::value(std::vector<float>{3,4}, {2});
  auto n2 = x.norm(2.0);
  REQUIRE(n2.numel() == 1);
  auto n1 = x.norm(1.0);
  REQUIRE(n1.numel() == 1);
}
